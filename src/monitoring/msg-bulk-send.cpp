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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <decaf/lang/Long.h>
#include <decaf/util/Date.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>
#include <activemq/library/ActiveMQCPP.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <errno.h>
#include <iostream>
#include "MsgPipe.h"
#include "MsgProducer.h"
#include "utility_routines.h"
#include "common/name_to_uid.h"
#include <execinfo.h>

#include "common/ConcurrentQueue.h"
#include "common/error.h"
#include "common/Logger.h"

using namespace std;


int proc_find()
{
    DIR* dir=NULL;
    struct dirent* ent=NULL;
    char* endptr=NULL;
    char buf[512]= {0};
    unsigned count = 0;
    const char* name = "fts_msg_bulk";

    if (!(dir = opendir("/proc")))
        {
            return -1;
        }


    while((ent = readdir(dir)) != NULL)
        {
            /* if endptr is not a null character, the directory is not
             * entirely numeric, so ignore it */
            long lpid = strtol(ent->d_name, &endptr, 10);
            if (*endptr != '\0')
                {
                    continue;
                }

            /* try to open the cmdline file */
            snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
            FILE* fp = fopen(buf, "r");

            if (fp)
                {
                    if (fgets(buf, sizeof(buf), fp) != NULL)
                        {
                            /* check the first token in the file, the program name */
                            char* first = NULL;
                            first = strtok(buf, " ");

                            if (first && strstr(first, name))
                                {
                                    fclose(fp);
                                    fp = NULL;
                                    ++count;
                                    continue;
                                }
                        }
                    if(fp)
                        fclose(fp);
                }

        }
    closedir(dir);
    return count;
}



/*
	There is a parent process which checks/monitors the status of the child.
	If the child hangs or for any reason die, transparently it's restarting it and restoring all mesgs
*/

void DoServer() throw()
{
    if (!get_mon_cfg_file()) {
        FTS3_COMMON_LOGGER_LOG(CRIT, "Could not open the monitoring configuration file");
        return;
    }

    fts3::common::theLogger().open(getLOGFILEDIR() + "" + getLOGFILENAME());

    std::string errorMessage;
    try
        {
            activemq::library::ActiveMQCPP::initializeLibrary();

            //initialize here to avoid race conditions
            ConcurrentQueue::getInstance();

            MsgPipe pipeMsg1;
            MsgProducer producer;

            // Start the pipe thread.
            Thread pipeThread(&pipeMsg1);
            pipeThread.start();

            // Start the producer thread.
            Thread producerThread(&producer);
            producerThread.start();

            FTS3_COMMON_LOGGER_LOG(INFO, "Threads started");

            // Wait for the threads to complete.
            pipeThread.join();
            producerThread.join();

            pipeMsg1.cleanup();
            producer.cleanup();

            activemq::library::ActiveMQCPP::shutdownLibrary();
        }
    catch (CMSException& e)
        {
            errorMessage = "PROCESS_ERROR " + e.getStackTraceString();
            FTS3_COMMON_LOGGER_LOG(ERR, errorMessage);
        }
    catch (const std::exception& e)
        {
            errorMessage = "PROCESS_ERROR " + std::string(e.what());
            FTS3_COMMON_LOGGER_LOG(ERR, errorMessage);
        }
    catch (...)
        {
            errorMessage = "PROCESS_ERROR Unknown exception";
            FTS3_COMMON_LOGGER_LOG(ERR, errorMessage);
        }
}


int main(int argc,  char** /*argv*/)
{
    // Do not even try if already running
    int n_running = proc_find();
    if (n_running < 0)
        {
            std::cerr << "Could not check if FTS3 is already running" << std::endl;
            return EXIT_FAILURE;
        }
    else if (n_running > 1)
        {
            std::cerr << "Only 1 instance of FTS3 messaging daemon can run at a time" << std::endl;
            return EXIT_FAILURE;
        }

    //switch to non-priviledged user to avoid reading the hostcert
    uid_t pw_uid = name_to_uid();
    setuid(pw_uid);
    seteuid(pw_uid);

    //if any param is provided stay attached to terminal
    if(argc <= 1)
        {
            int d =  daemon(0,0);
            if(d < 0)
                {
                    std::cerr << "Can't set daemon, will continue attached to tty" << std::endl;
                    return EXIT_FAILURE;
                }

            // Detach stdout
            freopen("/dev/null", "a", stdout);
        }

    DoServer();

    return 0;
}
