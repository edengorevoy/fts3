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

#ifndef CLI_JOBSTATUS_H_
#define CLI_JOBSTATUS_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <time.h>

#include <tuple>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

#include <boost/optional.hpp>
#include <boost/bind/bind.hpp>

#include <boost/property_tree/json_parser.hpp>

namespace fts3
{
namespace cli
{

namespace pt = boost::property_tree;

class FileInfo
{
    friend class MsgPrinter;
    friend class JsonOutput;

public:

    FileInfo(tns3__FileTransferStatus const * f) :
        src(*f->sourceSURL), dst (*f->destSURL), state(*f->transferFileState),
        reason(*f->reason), duration (f->duration), nbFailures(f->numFailures),
        stagingDuration(-1)
    {
        std::transform(
            f->retries.begin(),
            f->retries.end(),
            std::back_inserter(retries),
            boost::bind(&tns3__FileTransferRetry::reason, _1)
        );

        if (f->staging)
            stagingDuration = *f->staging;
    }

    FileInfo(pt::ptree const & t) :
        src(t.get<std::string>("source_surl")), dst(t.get<std::string>("dest_surl")), state(t.get<std::string>("file_state")),
        reason(t.get<std::string>("reason")), duration(0), nbFailures(t.get<int>("retry")),
        stagingDuration(0)
    {
        pt::ptree const & r = t.get_child("retries");

        pt::ptree::const_iterator it;
        for (it = r.begin(); it != r.end(); ++it)
            {
                retries.push_back(it->first);
            }

        std::string finish_time = t.get<std::string>("finish_time");
        std::string start_time = t.get<std::string>("start_time");

        tm time;
        strptime(finish_time.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
        time_t finish = mktime(&time);

        strptime(start_time.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
        time_t start = mktime(&time);

        duration = (long)difftime(finish, start);

        std::string staging_start = t.get<std::string>("staging_start");
        std::string staging_finished = t.get<std::string>("staging_finished");

        if (strptime(staging_start.c_str(), "%Y-%m-%dT%H:%M:%S", &time) != NULL)
            {
                time_t staging_start_time = mktime(&time);
                time_t staging_finished_time = ::time(NULL);

                if (strptime(staging_finished.c_str(), "%Y-%m-%dT%H:%M:%S", &time) != NULL)
                    staging_finished_time = mktime(&time);

                stagingDuration = staging_finished_time - staging_start_time;
            }
    }

    std::string getState() const
    {
        return state;
    }

    std::string getSource() const
    {
        return src;
    }

    std::string getDestination() const
    {
        return dst;
    }

private:
    std::string src;
    std::string dst;
    std::string state;
    std::string reason;
    long duration;
    int nbFailures;
    std::vector<std::string> retries;
    long stagingDuration;
};

class DetailedFileStatus
{
    friend class MsgPrinter;
    friend class JsonOutput;

public:
    DetailedFileStatus(tns3__DetailedFileStatus const * df) :
        jobId(df->jobId), src(df->sourceSurl), dst(df->destSurl), fileId(df->fileId), state(df->fileState)
    {

    }

    DetailedFileStatus(pt::ptree const & t) :
        jobId(t.get<std::string>("job_id")), src(t.get<std::string>("source_surl")), dst(t.get<std::string>("dest_surl")),
        fileId(t.get<int>("file_id")), state(t.get<std::string>("file_state"))
    {

    }

private:
    std::string jobId;
    std::string src;
    std::string dst;
    int fileId;
    std::string state;
};

class JobStatus
{
    friend class MsgPrinter;
    friend class JsonOutput;

public:

    typedef std::tuple<int, int, int, int, int , int, int, int, int> JobSummary;

    JobStatus(std::string const & jobId, std::string const & status, std::string const & dn, std::string const & reason,
              std::string const & vo, std::string const & submitTime, int nbFiles, int priority,
              boost::optional<JobSummary> summary = boost::optional<JobSummary>()) :
        jobId(jobId), status(status), dn(dn), reason(reason), vo (vo), submitTime(submitTime),
        nbFiles(nbFiles), priority(priority), summary(summary)
    {

    }

    virtual ~JobStatus() {}

    void addFile(FileInfo const & file)
    {
        files.push_back(file);
    }

    std::string getStatus()
    {
        return status;
    }

private:

    std::string jobId;
    std::string status;
    std::string dn;
    std::string reason;
    std::string vo;
    std::string submitTime;
    int nbFiles;
    int priority;

    boost::optional<JobSummary> summary;

    std::vector<FileInfo> files;
};



} /* namespace cli */
} /* namespace fts3 */

#endif /* CLI_JOBSTATUS_H_ */
