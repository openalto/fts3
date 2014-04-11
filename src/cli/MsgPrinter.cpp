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
 *
 * MsgPrinter.cpp
 *
 *  Created on: Dec 11, 2012
 *      Author: simonm
 */

#include "MsgPrinter.h"

#include "common/JobStatusHandler.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>

#include <utility>
#include <sstream>
#include <algorithm>

namespace fts3
{
namespace cli
{

using namespace boost;
using namespace boost::assign;
using namespace fts3::common;

void MsgPrinter::delegation_request_duration(long int h, long int m)
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Requesting delegated proxy for " << h << " hours and " << m << " minutes." << endl;
            return;
        }

    json_out.put("delegation.request.duration", lexical_cast<string>(h) + ":" + lexical_cast<string>(m));
}

void MsgPrinter::delegation_request_retry()
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Retrying!" << endl;
            return;
        }

    json_out.put("delegation.request.retry", true);
}

void MsgPrinter::delegation_request_error(string error)
{

//	if (!verbose) return;

    if (!json)
        {
            cout << "delegation: " << error << endl;
            return;
        }

    json_out.put("delegation.request.error", error);
}

void MsgPrinter::delegation_request_success(bool b)
{

    if (!verbose) return;

    if (!json)
        {
            if (b) cout << "Credential has been successfully delegated to the service." << endl;
            return;
        }

    json_out.put("delegation.request.delegated_successfully", b);
}

void MsgPrinter::delegation_local_expiration(long int h, long int m)
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Remaining time for the local proxy is " << h << "hours and " << m << " minutes." << endl;
            return;
        }

    json_out.put("delegation.expiration_time.local", lexical_cast<string>(h) + ":" + lexical_cast<string>(m));
}

void MsgPrinter::delegation_service_proxy(long int h, long int m)
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Remaining time for the proxy on the server side is " << h << " hours and " << m << " minutes." << endl;
            return;
        }

    json_out.put("delegation.expiration_time.service", lexical_cast<string>(h) + ":" + lexical_cast<string>(m));
}

void MsgPrinter::delegation_msg(string msg)
{

    if (!verbose) return;

    if (!json)
        {
            cout << msg << endl;
            return;
        }

    json_out.put("delegation.message", msg);
}

void MsgPrinter::endpoint(string endpoint)
{

    if (!json)
        {
            cout << "# Using endpoint: " << endpoint << endl;
            return;
        }

    json_out.put("endpoint", endpoint);
}

void MsgPrinter::service_version(string version)
{

    if (!json)
        {
            cout << "# Service version: " << version << endl;
            return;
        }

    json_out.put("service_version", version);
}

void MsgPrinter::service_interface(string interface)
{

    if (!json)
        {
            cout << "# Interface version: " << interface << endl;
            return;
        }

    json_out.put("service_interface", interface);
}

void MsgPrinter::service_schema(string schema)
{

    if (!json)
        {
            cout << "# Schema version: " << schema << endl;
            return;
        }

    json_out.put("service_schema", schema);
}

void MsgPrinter::service_metadata(string metadata)
{

    if (!json)
        {
            cout << "# Service features: " << metadata << endl;
            return;
        }

    json_out.put("service_metadata", metadata);
}

void MsgPrinter::client_version(string version)
{

    if (!json)
        {
            cout << "# Client version: " << version << endl;
            return;
        }

    json_out.put("client_version", version);
}

void MsgPrinter::client_interface(string interface)
{

    if (!json)
        {
            cout << "# Client interface version: " << interface << endl;
            return;
        }

    json_out.put("client_interface", interface);
}

void MsgPrinter::cancelled_job( pair<string, string> id_status)
{

    if (!json)
        {
            cout << "job " << id_status.first << ": " << id_status.second << endl;
            return;
        }

    map<string, string> object = map_list_of ("job_id", id_status.first) ("status", id_status.second);

    addToArray(
        json_out,
        "job",
        object
    );
}

void MsgPrinter::missing_parameter(string name)
{

    if (!json)
        {
            cout << "missing parameter: " << name << endl;
            return;
        }

    json_out.put("error.missing_parameter", name);
}

void MsgPrinter::bulk_submission_error(int line, string msg)
{

    if (!json)
        {
            cout << "submit: in line " << line << ": " << msg << endl;
            return;
        }

    json_out.put("error.line", line);
    json_out.put("error.message", msg);
}

void MsgPrinter::wrong_endpoint_format(string endpoint)
{

    if (!json)
        {
            cout << "wrongly formated endpoint: " << endpoint << endl;
            return;
        }

    json_out.put("wrong_format.endpoint", endpoint);
}

void MsgPrinter::version(string version)
{

    if (!json)
        {
            cout << "version: " << version << endl;
            return;
        }

    json_out.put("client_version", version);
}

void MsgPrinter::job_id(string job_id)
{

    if (!json)
        {
            cout << job_id << endl;
            return;
        }

    json_out.put("job.job_id", job_id);
}

void MsgPrinter::status(JobStatus js)
{

    if (!json)
        {
            cout << js.jobStatus << endl;
            return;
        }

    map<string, string> object = map_list_of ("job_id", js.jobId) ("status", js.jobStatus);
    addToArray(json_out, "job", object);
}

void MsgPrinter::error_msg(string msg)
{

    if (!json)
        {
            cout << "error: " << msg << endl;
            return;
        }

    json_out.put("error.message", msg);
}

void MsgPrinter::gsoap_error_msg(string msg)
{
    // remove backspaces if any in the string
    string::size_type  pos;
    while((pos = msg.find(8)) != string::npos)
        {
            msg.erase(pos, 1);
        }


    pos = msg.find("\"\nDetail: ", 0);

//	regex re (".*\"(.+)\"\nDetail: (.+)\n");

    if (pos == string::npos)
        {
            error_msg(msg);
            return;
        }

    if (!json)
        {
            cout << msg << endl;
            return;
        }

    string detail = msg.substr(pos + 10);
    string::size_type size = detail.size();
    if (detail[size - 1] == '\n') detail = detail.substr(0, size - 1);

    pos = msg.find("\"");
    string err_msg;
    if (pos != string::npos) err_msg = msg.substr(pos + 1);
    pos = msg.find("\"");
    if (pos != string::npos) err_msg = msg.substr(0, pos);
    size = err_msg.size();
    if (err_msg[size - 1] == '\n') err_msg = err_msg.substr(0, size - 1);

    json_out.put("error.message", err_msg);
    json_out.put("error.detail", detail);
}

void MsgPrinter::job_status(JobStatus js)
{
    // change the precision from msec to sec
    js.submitTime /= 1000;

    char time_buff[20];
    strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.submitTime));

    if (!json)
        {
            cout << "Request ID: " << js.jobId << endl;
            cout << "Status: " << js.jobStatus << endl;

            // if not verbose return
            if (!verbose) return;

            cout << "Client DN: " << js.clientDn << endl;
            cout << "Reason: " << (js.reason.empty() ? "<None>": js.reason) << endl;
            cout << "Submission time: " << time_buff << endl;
            cout << "Files: " << js.numFiles << endl;
            cout << "Priority: " << js.priority << endl;
            cout << "VOName: " << js.voName << endl;
            cout << endl;

            return;
        }

    map<string, string> object;

    if (verbose)
        {
            map<string, string> aux = map_list_of
                                      ("job_id", js.jobId)
                                      ("status", js.jobStatus)
                                      ("dn", js.clientDn)
                                      ("reason", js.reason.empty() ? "<None>": js.reason)
                                      ("submision_time", time_buff)
                                      ("file_count", lexical_cast<string>(js.numFiles))
                                      ("priority", lexical_cast<string>(js.priority))
                                      ("vo", js.voName)
                                      ;
            object = aux;
        }
    else
        {
            map<string, string> aux = map_list_of ("job_id", js.jobId) ("status", js.jobStatus);
            object = aux;
        }

    addToArray(json_out, "job", object);
}

void MsgPrinter::job_summary(JobSummary js)
{

    if (!json)
        {
            job_status(js.status);
            cout << "\tActive: " << js.numActive << endl;
            cout << "\tReady: " << js.numReady << endl;
            cout << "\tCanceled: " << js.numCanceled << endl;
            cout << "\tFinished: " << js.numFinished << endl;
            cout << "\tSubmitted: " << js.numSubmitted << endl;
            cout << "\tFailed: " << js.numFailed << endl;
            return;
        }

    // change the precision from msec to sec
    js.status.submitTime /= 1000;

    char time_buff[20];
    strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.status.submitTime));

    map<string, string> object = map_list_of
                                 ("job_id", js.status.jobId)
                                 ("status", js.status.jobStatus)
                                 ("dn", js.status.clientDn)
                                 ("reason", js.status.reason.empty() ? "<None>": js.status.reason)
                                 ("submision_time", time_buff)
                                 ("file_count", lexical_cast<string>(js.status.numFiles))
                                 ("priority", lexical_cast<string>(js.status.priority))
                                 ("vo", js.status.voName)

                                 ("summary.active", lexical_cast<string>(js.numActive))
                                 ("summary.ready", lexical_cast<string>(js.numReady))
                                 ("summary.canceled", lexical_cast<string>(js.numCanceled))
                                 ("summary.finished", lexical_cast<string>(js.numFinished))
                                 ("summary.submitted", lexical_cast<string>(js.numSubmitted))
                                 ("summary.failed", lexical_cast<string>(js.numFailed))
                                 ;

    addToArray(json_out, "job", object);
}

void MsgPrinter::file_list(vector<string> values, vector<string> retries)
{

    enum
    {
        SOURCE,
        DESTINATION,
        STATE,
        RETRIES,
        REASON,
        DURATION
    };

    if (!json)
        {
            cout << "  Source:      " << values[SOURCE] << endl;
            cout << "  Destination: " << values[DESTINATION] << endl;
            cout << "  State:       " << values[STATE] << endl;;
            cout << "  Reason:      " << values[REASON] << endl;
            cout << "  Duration:    " << values[DURATION] << endl;

            if (retries.size() > 0)
                {
                    cout << "  Retries: " << endl;
                    for_each(retries.begin(), retries.end(), cout << ("    " + lambda::_1) << '\n');
                }
            else
                {
                    cout << "  Retries:     " << values[RETRIES] << endl;
                }
            return;
        }

    ptree file;
    file.put("source", values[SOURCE]);
    file.put("destination", values[DESTINATION]);
    file.put("state", values[STATE]);
    file.put("reason", values[REASON]);
    file.put("duration", values[DURATION]);

    if (retries.size() > 0)
        {
            ptree retriesArray;
            vector<string>::const_iterator i;
            for (i = retries.begin(); i != retries.end(); ++i)
                retriesArray.push_front(std::make_pair("", ptree(*i)));
            file.put_child("retries", retriesArray);
        }
    else
        {
            file.put("retries", values[RETRIES]);
        }

    ptree& job = json_out.get_child("job");
    ptree::iterator it = job.begin();
    addToArray(it->second, "files", file);
}

MsgPrinter::MsgPrinter(ostream& out): verbose(false), json(false), out(out)
{

}

MsgPrinter::~MsgPrinter()
{
    if (json)
        write_json(out, json_out);
}

ptree MsgPrinter::getItem(map<string, string> values)
{

    ptree item;

    map<string, string>::iterator it;
    for (it = values.begin(); it != values.end(); it++)
        {
            item.put(it->first, it->second);
        }

    return item;
}

void MsgPrinter::put (ptree& root, string name, map<string, string>& object)
{
    map<string, string>::iterator it;
    for (it = object.begin(); it != object.end(); it++)
        {
            root.put(name + it->first, it->second);
        }
}

void MsgPrinter::addToArray(ptree& root, string name, map<string, string>& object)
{
    static const string array_sufix = "..";

    optional<ptree&> child = root.get_child_optional(name);
    if (child.is_initialized())
        {
            child.get().push_front(
                make_pair("", getItem(object))
            );
        }
    else
        {
            put(root, name + array_sufix, object);
        }
}

void MsgPrinter::addToArray(ptree& root, string name, string value)
{
    optional<ptree&> child = root.get_child_optional(name);
    if (child.is_initialized())
        {
            ptree item;
            item.put("", value);
            child.get().push_front(make_pair("", item));
        }
    else
        {
            ptree child, item;
            item.put("", value);
            child.push_front(make_pair("", item));
            root.put_child(name, child);
        }
}

void MsgPrinter::addToArray(ptree& root, string name, const ptree& node)
{
    optional<ptree&> child = root.get_child_optional(name);
    if (child.is_initialized())
        {
            child.get().push_back(make_pair("", node));
        }
    else
        {
            ptree files;
            files.push_back(std::make_pair("", node));
            root.put_child(name, files);
        }
}

} /* namespace server */
} /* namespace fts3 */
