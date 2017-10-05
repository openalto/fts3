/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TransfersService.h"
#include "VoShares.h"

#include "config/ServerConfig.h"
#include "common/DaemonTools.h"
#include "common/ThreadPool.h"

#include "cred/DelegCred.h"

#include "db/generic/TransferFile.h"

#include "server/DrainMode.h"

#include "TransferFileHandler.h"
#include "FileTransferExecutor.h"

#include <msg-bus/producer.h>

using namespace fts3::common;


namespace fts3 {
namespace server {

extern time_t retrieveRecords;


TransfersService::TransfersService(): BaseService("TransfersService")
{
    cmd = "fts_url_copy";

    logDir = config::ServerConfig::instance().get<std::string>("TransferLogDirectory");
    msgDir = config::ServerConfig::instance().get<std::string>("MessagingDirectory");
    execPoolSize = config::ServerConfig::instance().get<int>("InternalThreadPool");
    ftsHostName = config::ServerConfig::instance().get<std::string>("Alias");
    infosys = config::ServerConfig::instance().get<std::string>("Infosys");

    monitoringMessages = config::ServerConfig::instance().get<bool>("MonitoringMessaging");
    schedulingInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("SchedulingInterval");
}


TransfersService::~TransfersService()
{
}


void TransfersService::runService()
{
    while (!boost::this_thread::interruption_requested())
    {
        retrieveRecords = time(0);

        try
        {
            boost::this_thread::sleep(schedulingInterval);

            if (DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            executeUrlcopy();
        }
        catch (boost::thread_interrupted&)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested" << commit;
            break;
        }
        catch (std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService " << e.what() << commit;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
        }
    }
}


void TransfersService::getFiles(const std::vector<QueueId>& queues)
{
    ThreadPool<FileTransferExecutor> execPool(execPoolSize);

    try
    {
        if (queues.empty())
            return;

        //now get files to be scheduled
        std::map<std::string, std::list<TransferFile> > voQueues;
        DBSingleton::instance().getDBObjectInstance()->getReadyTransfers(queues, voQueues);

        if (voQueues.empty())
            return;

        int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
        int urlCopyCount = countProcessesWithName("fts_url_copy");

        // create transfer-file handler
        TransferFileHandler tfh(voQueues);

        std::map<std::pair<std::string, std::string>, std::string> proxies;

        // loop until all files have been served
        int initial_size = tfh.size();

        while (!tfh.empty())
        {
            // iterate over all VOs
            for (auto it_vo = tfh.begin(); it_vo != tfh.end(); it_vo++)
            {
                if (boost::this_thread::interruption_requested())
                {
                    execPool.interrupt();
                    return;
                }

                boost::optional<TransferFile> opt_tf = tfh.get(*it_vo);
                // if this VO has no more files to process just continue
                if (!opt_tf)
                    continue;

                TransferFile & tf = *opt_tf;

                // just to be sure
                if (tf.fileId == 0 || tf.userDn.empty() || tf.credId.empty())
                    continue;

                std::pair<std::string, std::string> proxy_key(tf.credId, tf.userDn);

                if (proxies.find(proxy_key) == proxies.end())
                {
                    proxies[proxy_key] = DelegCred::getProxyFile(tf.userDn, tf.credId);
                }

                if (maxUrlCopy > 0 && urlCopyCount > maxUrlCopy) {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                        << "Reached limitation of MaxUrlCopyProcesses"
                        << commit;
                    break;
                } else {
                    FileTransferExecutor *exec = new FileTransferExecutor(tf,
                        tfh, monitoringMessages, infosys, ftsHostName,
                        proxies[proxy_key], logDir, msgDir);

                    execPool.start(exec);
                    ++urlCopyCount;
                }
            }
        }

        // wait for all the workers to finish
        execPool.join();
        FTS3_COMMON_LOGGER_NEWLOG(INFO) <<"Threadpool processed: " << initial_size
                << " files (" << execPool.reduce(std::plus<int>())
                << " have been scheduled)" << commit;

    }
    catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TransfersService:getFiles" << commit;
        execPool.interrupt();
        execPool.join();
    }
    catch (std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService:getFiles " << e.what() << commit;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
    }
}


/**
 * Transfers in uneschedulable queues must be set to fail
 */
static void failUnschedulable(const std::vector<QueueId> &unschedulable)
{
    Producer producer(config::ServerConfig::instance().get<std::string>("MessagingDirectory"));

    std::map<std::string, std::list<TransferFile> > voQueues;
    DBSingleton::instance().getDBObjectInstance()->getReadyTransfers(unschedulable, voQueues);

    for (auto iterList = voQueues.begin(); iterList != voQueues.end(); ++iterList) {
        const std::list<TransferFile> &transferList = iterList->second;
        for (auto iterTransfer = transferList.begin(); iterTransfer != transferList.end(); ++iterTransfer) {
            events::Message status;

            status.set_transfer_status("FAILED");
            status.set_timestamp(millisecondsSinceEpoch());
            status.set_process_id(0);
            status.set_job_id(iterTransfer->jobId);
            status.set_file_id(iterTransfer->fileId);
            status.set_source_se(iterTransfer->sourceSe);
            status.set_dest_se(iterTransfer->destSe);
            status.set_transfer_message("No share configured for this VO");
            status.set_retry(false);
            status.set_errcode(EPERM);

            producer.runProducerStatus(status);
        }
    }
}


void TransfersService::executeUrlcopy()
{
    std::vector<QueueId> queues, unschedulable;
    boost::thread_group g;

    try {
        DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);
        // Breaking determinism. See FTS-704 for an explanation.
        std::random_shuffle(queues.begin(), queues.end());
        // Apply VO shares at this level. Basically, if more than one VO is used the same link,
        // pick one each time according to their respective weights
        queues = applyVoShares(queues, unschedulable);
        // Fail all that are unschedulable
        failUnschedulable(unschedulable);

        if (queues.empty()) {
            return;
        }
        else if (1 == queues.size()) {
            getFiles(queues);
        }
        else {
            std::size_t const half_size1 = queues.size() / 2;
            std::vector<QueueId> split_1(queues.begin(), queues.begin() + half_size1);
            std::vector<QueueId> split_2(queues.begin() + half_size1, queues.end());

            std::size_t const half_size2 = split_1.size() / 2;
            std::vector<QueueId> split_11(split_1.begin(), split_1.begin() + half_size2);
            std::vector<QueueId> split_21(split_1.begin() + half_size2, split_1.end());

            std::size_t const half_size3 = split_2.size() / 2;
            std::vector<QueueId> split_12(split_2.begin(), split_2.begin() + half_size3);
            std::vector<QueueId> split_22(split_2.begin() + half_size3, split_2.end());

            // create threads only when needed
            if (!split_11.empty()) {
                g.create_thread(boost::bind(&TransfersService::getFiles, this, boost::ref(split_11)));
            }
            if (!split_21.empty()) {
                g.create_thread(boost::bind(&TransfersService::getFiles, this, boost::ref(split_21)));
            }
            if (!split_12.empty()) {
                g.create_thread(boost::bind(&TransfersService::getFiles, this, boost::ref(split_12)));
            }
            if (!split_22.empty()) {
                g.create_thread(boost::bind(&TransfersService::getFiles, this, boost::ref(split_22)));
            }

            // wait for them
            g.join_all();
        }
    }
    catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TransfersService" << commit;
        g.interrupt_all();
        g.join_all();
        throw;
    }
    catch (std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService " << e.what() << commit;
        throw;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
        throw;
    }
}

} // end namespace server
} // end namespace fts3
