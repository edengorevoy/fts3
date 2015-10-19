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

#include <activemq/library/ActiveMQCPP.h>

#include "common/ConcurrentQueue.h"
#include "common/DaemonTools.h"
#include "common/Logger.h"

#include "UtilityRoutines.h"
#include "MsgPipe.h"
#include "MsgProducer.h"

using namespace fts3::common;

const char *USER_NAME = "fts3";


static void DoServer(bool isDaemon) throw()
{
    if (!get_mon_cfg_file()) {
        FTS3_COMMON_LOGGER_LOG(CRIT, "Could not open the monitoring configuration file");
        return;
    }

    std::string logFile = getLOGFILEDIR() + "" + getLOGFILENAME();
    if (isDaemon) {
        if (theLogger().redirect(logFile, logFile) != 0) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Could not open the log file");
            return;
        }
    }

    try {
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
    catch (CMSException& e) {
        std::string errorMessage = "PROCESS_ERROR " + e.getStackTraceString();
        FTS3_COMMON_LOGGER_LOG(ERR, errorMessage);
    }
    catch (const std::exception& e) {
        std::string  errorMessage = "PROCESS_ERROR " + std::string(e.what());
        FTS3_COMMON_LOGGER_LOG(ERR, errorMessage);
    }
    catch (...) {
        FTS3_COMMON_LOGGER_LOG(ERR, "PROCESS_ERROR Unknown exception");
    }
}


static void dropPrivileges()
{
    uid_t pw_uid = getUserUid(USER_NAME);
    setuid(pw_uid);
    seteuid(pw_uid);
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Process UID changed to " << pw_uid << commit;
}


int main(int argc, char** /*argv*/)
{
    // Do not even try if already running
    int n_running = countProcessesWithName("fts_msg_bulk");
    if (n_running < 0) {
        std::cerr << "Could not check if FTS3 is already running" << std::endl;
        return EXIT_FAILURE;
    }
    else if (n_running > 1) {
        std::cerr << "Only 1 instance of FTS3 messaging daemon can run at a time" << std::endl;
        return EXIT_FAILURE;
    }

    dropPrivileges();

    //if any param is provided stay attached to terminal
    if (argc <= 1) {
        int d = daemon(0, 0);
        if (d < 0) {
            std::cerr << "Can't set daemon, will continue attached to tty" << std::endl;
            return EXIT_FAILURE;
        }
    }

    DoServer(argc <= 1);

    return 0;
}
