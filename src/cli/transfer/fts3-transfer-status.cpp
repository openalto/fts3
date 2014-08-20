/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "GSoapContextAdapter.h"
#include "JobStatus.h"
#include "ui/TransferStatusCli.h"

#include "rest/ResponseParser.h"
#include "rest/HttpRequest.h"
#include "exception/bad_option.h"

#include "exception/cli_exception.h"
#include "JsonOutput.h"

#include <algorithm>
#include <vector>
#include <string>
#include <iterator>

#include <boost/assign.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

using namespace fts3::cli;

static bool isTransferFailed(const std::string& state)
{
    return state == "CANCELED" || state == "FAILED";
}

/**
 * This is the entry point for the fts3-transfer-status command line tool.
 */
int main(int ac, char* av[])
{
    static const int DEFAULT_LIMIT = 100;

    try
        {
            TransferStatusCli cli;
            // create and initialise the command line utility
            cli.parse(ac, av);
            if (!cli.validate()) return 1;

            if (cli.rest())
                {
                    std::vector<std::string> const jobIds = cli.getJobIds();
                    std::vector<std::string>::const_iterator itr;

                    for (itr = jobIds.begin(); itr != jobIds.end(); ++itr)
                        {
                            std::string url = cli.getService() + "/jobs/" + *itr;

                            std::stringstream ss;
                            HttpRequest http (url, cli.capath(), cli.proxy(), ss);
                            http.get();
                            ResponseParser respons(ss);

                            std::cout << respons.get("job_state") << std::endl;
                        }
                    return 0;
                }

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter ctx (cli.getService());
            cli.printApiDetails(ctx);

            if (cli.p())
                {
                    std::vector<std::string> ids = cli.getJobIds();
                    std::vector<std::string>::const_iterator it;

                    for (it = ids.begin(); it != ids.end(); ++it)
                        {
                            tns3__DetailedJobStatus* ptr = ctx.getDetailedJobStatus(*it);
                            MsgPrinter::instance().print(*it, ptr->transferStatus);
                        }

                    return 0;
                }

            // archived content?
            bool archive = cli.queryArchived();
            // get job IDs that have to be check
            std::vector<std::string> jobIds = cli.getJobIds();

            // iterate over job IDs
            std::vector<std::string>::iterator it;
            for (it = jobIds.begin(); it < jobIds.end(); it++)
                {

                    std::string jobId = *it;

                    std::ofstream failedFiles;
                    if (cli.dumpFailed())
                        {
                            failedFiles.open(jobId.c_str(), ios_base::out);
                            if (failedFiles.fail())
                                throw bad_option("dump-failed", strerror(errno));
                        }

                    JobStatus status = cli.isVerbose() ?
                            ctx.getTransferJobSummary(jobId, archive)
                            :
                            ctx.getTransferJobStatus(jobId, archive)
                            ;

                    // If a list is requested, or dumping the failed transfers,
                    // get the transfers
                    if (cli.list() || cli.dumpFailed())
                        {
                            int offset = 0;
                            int cnt = 0;

                            do
                                {
                                    // do the request
                                    impltns__getFileStatusResponse resp;
                                    cnt = ctx.getFileStatus(jobId, archive, offset, DEFAULT_LIMIT, cli.detailed(), resp);

                                    if (cnt > 0 && resp._getFileStatusReturn)
                                        {

                                            std::vector<tns3__FileTransferStatus * > const & vect = resp._getFileStatusReturn->item;
                                            std::vector<tns3__FileTransferStatus * >::const_iterator it;
                                            // print the response
                                            for (it = vect.begin(); it < vect.end(); it++)
                                                {
                                                    tns3__FileTransferStatus* stat = *it;

                                                    if (cli.list())
                                                        {

                                                            status.addFile(*stat);
                                                        }
                                                    else if (cli.dumpFailed() && isTransferFailed(*stat->transferFileState))
                                                        {
                                                            failedFiles << *stat->sourceSURL << " "
                                                                        << *stat->destSURL
                                                                        << std::endl;
                                                        }
                                                }
                                        }

                                    offset += cnt;
                                }
                            while(cnt == DEFAULT_LIMIT);
                        }

                    MsgPrinter::instance().print(status);
                }

        }
    catch(cli_exception const & ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(std::exception& ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(...)
        {
            MsgPrinter::instance().print("error", "exception of unknown type!");
            return 1;
        }

    return 0;
}
