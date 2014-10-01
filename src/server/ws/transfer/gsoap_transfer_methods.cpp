/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include "db/generic/SingleDbInstance.h"
#include "config/serverconfig.h"

#include "JobSubmitter.h"
#include "RequestLister.h"
#include "VersionResolver.h"
#include "GSoapJobStatus.h"
#include "Blacklister.h"
#include "JobCancelHandler.h"
#include "JobStatusGetter.h"

#include "ws/CGsiAdapter.h"
#include "ws/AuthorizationManager.h"

#include "common/JobStatusHandler.h"

#include "common/logger.h"
#include "common/error.h"
#include "common/uuid_generator.h"
#include "parse_url.h"
#include "ws/delegation/GSoapDelegationHandler.h"

#include <fstream>
#include <sstream>
#include <list>

#include <boost/scoped_ptr.hpp>

#include <string.h>

#include <cgsi_plugin.h>
#include "ws-ifce/gsoap/fts3.nsmap"

#include "ws/SingleTrStateInstance.h"

using namespace boost;
using namespace db;
using namespace fts3::config;
using namespace fts3::ws;
using namespace fts3::common;
using namespace std;


int fts3::impltns__fileDelete(soap* ctx, tns3__deleteFiles* fileNames,impltns__fileDeleteResponse& resp)
{
    try
        {

            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER);
            resp._jobid = UuidGenerator::generateUUID();

            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            string hostN;
            const regex fileUrlRegex("([a-zA-Z][a-zA-Z0-9+\.-]*://[a-zA-Z0-9\\.-]+)(:\\d+)?/.+");

            multimap<string, string> rulsHost;
            vector<string>::iterator it;

            for (it = fileNames->delf.begin(); it != fileNames->delf.end(); ++it)
                {

                    //checks the url validation...
                    Uri u0 = Uri::Parse(*it);
                    if(!(u0.Host.length() != 0 && u0.Protocol.length() != 0 && u0.Path.length() != 0))
                        {
                            string errMsg2 = "Something is not right with uri: " + (*it);
                            throw Err_Custom(errMsg2);
                        }
                    smatch what;
                    if (regex_match(*it, what,fileUrlRegex, match_extra))
                        {
                            // indexes are shifted by 1 because at index 0 is the whole string
                            hostN =  string(what[1]);
                        }
                    else
                        {
                            string errMsg = "Can't extract hostname from url: " + (*it);
                            throw Err_Custom(errMsg);
                        }

                    // correlates the file url with its' hostname
                    rulsHost.insert(pair<string, string>((*it),hostN));

                }

            std::string credID;
            GSoapDelegationHandler handler(ctx);
            credID = handler.makeDelegationId();

            DBSingleton::instance().getDBObjectInstance()->submitdelete(resp._jobid,rulsHost,dn,vo, credID);

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception when submitting file deletions has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "DeleteException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception when submitting file deletions has been caught"  << commit;
            soap_receiver_fault(ctx, "fileDelete", "DeleteException");
            return SOAP_FAULT;
        }
    return SOAP_OK;
}

/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int fts3::impltns__transferSubmit(soap *soap, tns3__TransferJob *_job, struct impltns__transferSubmitResponse &_param_3)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit' request" << commit;

    try
        {
            // since submitting requires (sometimes) delegation we need authorization on the delegation level
            AuthorizationManager::getInstance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            JobSubmitter submitter (soap, _job, false);
            _param_3._transferSubmitReturn = submitter.submit();

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: transferSubmit"  << commit;
            soap_receiver_fault(soap, "transferSubmit", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'transferSubmit2' (returns error code or SOAP_OK)
int fts3::impltns__transferSubmit2(soap *soap, tns3__TransferJob *_job, struct impltns__transferSubmit2Response &_param_4)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit2' request" << commit;

    try
        {
            AuthorizationManager::getInstance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            JobSubmitter submitter (soap, _job, true);
            _param_4._transferSubmit2Return = submitter.submit();

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: transferSubmit2"  << commit;
            soap_receiver_fault(soap, "transferSubmit2", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int fts3::impltns__transferSubmit3(soap *soap, tns3__TransferJob2 *_job, struct impltns__transferSubmit3Response &_param_5)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit3' request" << commit;

    try
        {
            AuthorizationManager::getInstance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            JobSubmitter submitter (soap, _job);
            _param_5._transferSubmit3Return = submitter.submit();

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: transferSubmit3"  << commit;
            soap_receiver_fault(soap, "transferSubmit3", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__transferSubmit4(struct soap* ctx, tns3__TransferJob3 *job, impltns__transferSubmit4Response &resp)
{

    try
        {
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            JobSubmitter submitter (ctx, job);
            resp._transferSubmit4Return = submitter.submit();

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: transferSubmit4"  << commit;
            soap_receiver_fault(ctx, "transferSubmit4", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'listRequests' (returns error code or SOAP_OK)
int fts3::impltns__listRequests(soap *soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, struct impltns__listRequestsResponse &_param_7)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests' request" << commit;

    try
        {
            AuthorizationManager::Level lvl = AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER);
            RequestLister lister(soap, _inGivenStates);
            _param_7._listRequestsReturn = lister.list(lvl);

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: listRequests"  << commit;
            soap_receiver_fault(soap, "listRequests", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'listRequests2' (returns error code or SOAP_OK)
int fts3::impltns__listRequests2(soap *soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, string placeHolder, string _forDN, string _forVO, string src, string dst, struct impltns__listRequests2Response &_param_8)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests2' request" << commit;

    try
        {
            AuthorizationManager::Level lvl = AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER);
            RequestLister lister(soap, _inGivenStates, _forDN, _forVO, src, dst);
            _param_8._listRequests2Return = lister.list(lvl);

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: listRequests2"  << commit;
            soap_receiver_fault(soap, "listRequests2", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__listDeletionRequests(soap* ctx, impltns__ArrayOf_USCOREsoapenc_USCOREstring* inGivenStates, std::string placeHolder, std::string dn, std::string vo, std::string src, std::string dst, impltns__listDeletionRequestsResponse& resp)
{
    try
        {
            AuthorizationManager::Level lvl = AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER);
            RequestLister lister(ctx, inGivenStates, dn, vo, src, dst);
            resp._listRequests2Return = lister.listDm(lvl);

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: listRequests2"  << commit;
            soap_receiver_fault(ctx, "listRequests2", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'getFileStatus' (returns error code or SOAP_OK)
int fts3::impltns__getFileStatus(soap *ctx, string requestID, int offset, int limit, impltns__getFileStatusResponse & resp)
{
    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(requestID, false)
            );
            // Authorise the request
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());
            // create response
            resp._getFileStatusReturn = soap_new_impltns__ArrayOf_USCOREtns3_USCOREFileTransferStatus(ctx, -1);
            // fill the response
            JobStatusGetter getter (ctx, requestID, false, offset, limit, false);
            getter.file_status<tns3__FileTransferStatus>(resp._getFileStatusReturn->item);

        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getFileStatus" << commit;
            soap_receiver_fault(ctx, "getFileStatus", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'getFileStatus3' (returns error code or SOAP_OK)
int fts3::impltns__getFileStatus3(soap *ctx, fts3::tns3__FileRequest *req, fts3::impltns__getFileStatus3Response &resp)
{
    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(req->jobId, req->archive)
            );
            // Authorise the request
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());
            // create response
            resp.getFileStatusReturn = soap_new_impltns__ArrayOf_USCOREtns3_USCOREFileTransferStatus(ctx, -1);
            // fill the response
            JobStatusGetter getter (ctx, req->jobId, req->archive, req->offset, req->limit, req->retries);
            getter.file_status<tns3__FileTransferStatus>(resp.getFileStatusReturn->item);

        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getFileStatus" << commit;
            soap_receiver_fault(ctx, "getFileStatus", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'getFileStatus2' (returns error code or SOAP_OK)
int fts3::impltns__getFileStatus2(soap *ctx, string requestID, int offset, int limit, impltns__getFileStatus2Response &resp)
{
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getFileStatus2' request" << commit;

    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(requestID, false)
            );
            // Authorise the request
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());
            // create response
            resp._getFileStatus2Return = soap_new_impltns__ArrayOf_USCOREtns3_USCOREFileTransferStatus2(ctx, -1);
            // fill the response
            JobStatusGetter getter (ctx, requestID, false, offset, limit, false);
            getter.file_status<tns3__FileTransferStatus2>(resp._getFileStatus2Return->item);

        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getFileStatus" << commit;
            soap_receiver_fault(ctx, "getFileStatus", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'getTransferJobStatus' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobStatus(soap *ctx, string requestID,
                                        struct impltns__getTransferJobStatusResponse &resp)
{
    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(requestID, false)
            );

            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());

            JobStatusGetter getter (ctx, requestID, false);
            getter.job_status(resp._getTransferJobStatusReturn);
        }
    catch (Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getTransferJobStatus" << commit;
            soap_receiver_fault(ctx, "getTransferJobStatus", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'getTransferJobStatus2' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobStatus2(soap *ctx, fts3::tns3__JobRequest *req, fts3::impltns__getTransferJobStatus2Response &resp)
{
    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(req->jobId, req->archive)
            );

            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());

            JobStatusGetter getter (ctx, req->jobId, req->archive);
            getter.job_status(resp.getTransferJobStatusReturn);
        }
    catch (Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getTransferJobStatus"  << commit;
            soap_receiver_fault(ctx, "getTransferJobStatus", "TransferException");
            return SOAP_FAULT;
        }
    return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobSummary(soap *ctx, string requestID, impltns__getTransferJobSummaryResponse &resp)
{
    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(requestID, false)
            );


            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());

            JobStatusGetter getter(ctx, requestID, false);
            getter.job_summary(resp._getTransferJobSummaryReturn);

        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getTransferJobSummary" << commit;
            soap_receiver_fault(ctx, "getTransferJobSummary", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary2' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobSummary2(soap *ctx, string requestID, impltns__getTransferJobSummary2Response &resp)
{
    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(requestID, false)
            );
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());

            JobStatusGetter getter(ctx, requestID, false);
            getter.job_summary(resp._getTransferJobSummary2Return);

        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getTransferJobSummary2" << commit;
            soap_receiver_fault(ctx, "getTransferJobSummary2", "TransferException");
            return SOAP_FAULT;
        }


    return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary3' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobSummary3(soap *ctx, fts3::tns3__JobRequest *req,
        fts3::impltns__getTransferJobSummary3Response &resp)
{
    try
        {
            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(req->jobId, req->archive)
            );
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());

            JobStatusGetter getter(ctx, req->jobId, req->archive);
            getter.job_summary(resp.getTransferJobSummary2Return);
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getTransferJobSummary " << commit;
            soap_receiver_fault(ctx, "getTransferJobSummary", "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: "  << commit;
            soap_receiver_fault(ctx, "getTransferJobSummary", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}



/// Web service operation 'getVersion' (returns error code or SOAP_OK)
int fts3::impltns__getVersion(soap *soap, struct impltns__getVersionResponse &_param_21)
{
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getVersion' request" << commit;
    _param_21.getVersionReturn = VersionResolver::getInstance().getVersion();
    _param_21.getVersionReturn = "3.7.6-1";
    return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int fts3::impltns__getSchemaVersion(soap *soap, struct impltns__getSchemaVersionResponse &_param_22)
{
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getSchemaVersion' request" << commit;
    _param_22.getSchemaVersionReturn = VersionResolver::getInstance().getSchema();
    _param_22.getSchemaVersionReturn = "3.5.0";
    return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int fts3::impltns__getInterfaceVersion(soap *soap, struct impltns__getInterfaceVersionResponse &_param_23)
{
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getInterfaceVersion' request" << commit;
    _param_23.getInterfaceVersionReturn = VersionResolver::getInstance().getInterface();
    _param_23.getInterfaceVersionReturn = "3.7.0";
    return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int fts3::impltns__getServiceMetadata(soap *soap, string _key, struct impltns__getServiceMetadataResponse &_param_24)
{
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getServiceMetadata' request" << commit;
    _param_24._getServiceMetadataReturn = VersionResolver::getInstance().getMetadata();
    _param_24._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";
    return SOAP_OK;
}


/// Web service operation 'cancel' (returns error code or SOAP_OK)
int fts3::impltns__cancel(soap *ctx, impltns__ArrayOf_USCOREsoapenc_USCOREstring * request, struct impltns__cancelResponse &_param_14)
{
    try
        {
            if (request)
                {
                    JobCancelHandler handler (ctx, request->item);
                    handler.cancel();
                }
        }
    catch(Err& ex)
        {
            // the string that has to be erased
            const string erase = "SOAP fault: SOAP-ENV:Server - ";
            // Backspace character
            const char bs = 8;
            // glite error message that is used in case that transfer could not be canceled because it is in terminal state
            string glite_err_msg = "Cancel failed (nothing was done).";
            // add the backspaces at the front of the message in order to erased unwanted text
            for (int i = 0; i < erase.size(); i++)
                {
                    glite_err_msg = bs + glite_err_msg;
                }
            // check if we want to replace the original message with the glite one
            string err_msg (ex.what());
            if (err_msg.find("does not exist") != string::npos)
                {
                    err_msg = glite_err_msg;
                }
            // handle the exception
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, err_msg.c_str(), 0);

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, job can't be canceled "  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__cancel2(soap* ctx, impltns__ArrayOf_USCOREsoapenc_USCOREstring *request, impltns__cancel2Response &resp)
{
    try
        {
            if (request)
                {
                    JobCancelHandler handler (ctx, request->item);
                    handler.cancel(resp);
                }
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), 0);
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, job can't be cancelled "  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/// Web service operation 'setJobPriority' (returns error code or SOAP_OK)
int fts3::impltns__setJobPriority(soap *ctx, string requestID, int priority, impltns__setJobPriorityResponse &resp)
{
    impltns__prioritySetResponse r;
    impltns__prioritySet(ctx, requestID, priority, r);
    return SOAP_OK;
}

/// Web service operation 'addVOManager' (returns error code or SOAP_OK)
int fts3::impltns__addVOManager(soap *soap, string _VOName, string _principal, struct impltns__addVOManagerResponse &_param_16)
{
    return SOAP_OK;
}

/// Web service operation 'removeVOManager' (returns error code or SOAP_OK)
int fts3::impltns__removeVOManager(soap *soap, string _VOName, string _principal, struct impltns__removeVOManagerResponse &_param_17)
{
    return SOAP_OK;
}

/// Web service operation 'listVOManagers' (returns error code or SOAP_OK)
int fts3::impltns__listVOManagers(soap *soap, string _VOName, struct impltns__listVOManagersResponse &_param_18)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listVOManagers' request" << commit;

    _param_18._listVOManagersReturn = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(soap, -1);
    _param_18._listVOManagersReturn->item.push_back("default username");

    return SOAP_OK;
}

/// Web service operation 'getRoles' (returns error code or SOAP_OK)
int fts3::impltns__getRoles(soap *soap, struct impltns__getRolesResponse &_param_19)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRoles' request" << commit;

    _param_19.getRolesReturn = soap_new_tns3__Roles(soap, -1);

    _param_19.getRolesReturn->clientDN = soap_new_std__string(soap, -1);
    *_param_19.getRolesReturn->clientDN = "clientDN";
    _param_19.getRolesReturn->serviceAdmin = soap_new_std__string(soap, -1);
    *_param_19.getRolesReturn->serviceAdmin = "serviceAdmin";
    _param_19.getRolesReturn->submitter = soap_new_std__string(soap, -1);
    *_param_19.getRolesReturn->submitter = "submitter";

    tns3__StringPair* pair = soap_new_tns3__StringPair(soap, -1);
    pair->string1 = soap_new_std__string(soap, -1);
    *pair->string1 = "string1";
    pair->string2 = soap_new_std__string(soap, -1);
    *pair->string2 = "string2";
    _param_19.getRolesReturn->VOManager.push_back(pair);

    return SOAP_OK;
}

/// Web service operation 'getRolesOf' (returns error code or SOAP_OK)
int fts3::impltns__getRolesOf(soap *soap, string _otherDN, struct impltns__getRolesOfResponse &_param_20)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRolesOf' request" << commit;

    _param_20._getRolesOfReturn = soap_new_tns3__Roles(soap, -1);

    _param_20._getRolesOfReturn->clientDN = soap_new_std__string(soap, -1);
    *_param_20._getRolesOfReturn->clientDN = "clientDN";
    _param_20._getRolesOfReturn->serviceAdmin = soap_new_std__string(soap, -1);
    *_param_20._getRolesOfReturn->serviceAdmin = "serviceAdmin";
    _param_20._getRolesOfReturn->submitter = soap_new_std__string(soap, -1);
    *_param_20._getRolesOfReturn->submitter = "submitter";

    tns3__StringPair* pair = soap_new_tns3__StringPair(soap, -1);
    pair->string1 = soap_new_std__string(soap, -1);
    *pair->string1 = "string1";
    pair->string2 = soap_new_std__string(soap, -1);
    *pair->string2 = "string2";
    _param_20._getRolesOfReturn->VOManager.push_back(pair);

    return SOAP_OK;
}

/// Web service operation 'debugSet' (returns error code or SOAP_OK)
int fts3::impltns__debugSet(struct soap* soap, string _source, string _destination, bool _debug, struct impltns__debugSetResponse &_param_16)
{
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'debugSet' request" << commit;
    try
        {
            CGsiAdapter cgsi(soap);
            string dn = cgsi.getClientDn();

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn
                                             << " is turning " << (_debug ? "on" : "off") << " the debug mode for " << _source << commit;
            if (!_destination.empty())
                {
                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << " and " << _destination << " pair" << commit;
                }

            AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::CONFIG, AuthorizationManager::dummy);
            DBSingleton::instance().getDBObjectInstance()->setDebugLevel (
                _source,
                _destination,
                _debug ? 1 : 0
            );

            string cmd = "fts3-debug-set ";
            cmd += (_debug ? "on " : "off ") + _source + " " + _destination;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd, "debug");
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown"  << commit;
            return SOAP_FAULT;
        }
    return SOAP_OK;
}

int fts3::impltns__debugLevelSet(struct soap* soap, string _source, string _destination, int _level, struct impltns__debugLevelSetResponse &_resp)
{
//  FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'debugLevelSet' request" << commit;
    try
        {
            CGsiAdapter cgsi(soap);
            string dn = cgsi.getClientDn();

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn
                                             << " is turning debug to level "
                                             << _level << " for " << _source << commit;
            if (!_destination.empty())
                {
                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << " and " << _destination << " pair" << commit;
                }

            AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::CONFIG, AuthorizationManager::dummy);
            DBSingleton::instance().getDBObjectInstance()->setDebugLevel (
                _source,
                _destination,
                _level
            );

            string cmd = "fts3-debug-set ";
            cmd += boost::lexical_cast<std::string>(_level) + " " + _source + " " + _destination;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd, "debug");
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown"  << commit;
            return SOAP_FAULT;
        }
    return SOAP_OK;
}

int fts3::impltns__blacklistSe(struct soap* ctx, std::string name, std::string vo, std::string status, int timeout, bool blk, struct impltns__blacklistSeResponse &resp)
{

    try
        {
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::CONFIG, AuthorizationManager::dummy);

            Blacklister blacklister (ctx, name, vo, status, timeout, blk);
            blacklister.executeRequest();
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown "  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__blacklistDn(soap* ctx, string subject, bool blk, string status, int timeout, impltns__blacklistDnResponse &resp)
{

    try
        {

            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::CONFIG, AuthorizationManager::dummy);

            Blacklister blacklister(ctx, subject, status, timeout, blk);
            blacklister.executeRequest();

        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}


int fts3::impltns__prioritySet(soap* ctx, string job_id, int priority, impltns__prioritySetResponse &resp)
{

    vector<JobStatus*> fileStatuses;
    vector<JobStatus*>::iterator it;

    try
        {

            CGsiAdapter cgsi(ctx);
            string dn = cgsi.getClientDn();

            scoped_ptr<TransferJobs> job (
                DBSingleton::instance()
                .getDBObjectInstance()
                ->getTransferJob(job_id, false)
            );

            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());

            // check job status

            DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(job_id, false, fileStatuses);

            string status;
            if(!fileStatuses.empty())
                {
                    // get the job status
                    status = (*fileStatuses.begin())->jobStatus;
                    // release the memory
                    for (it = fileStatuses.begin(); it < fileStatuses.end(); ++it)
                        {
                            if(*it)
                                delete *it;
                        }
                    fileStatuses.clear();
                }
            else
                {
                    throw Err_Custom("Job ID <" + job_id + "> was not found");
                }

            // check if the job is not finished already
            if (JobStatusHandler::getInstance().isTransferFinished(status))
                {
                    throw Err_Custom("The transfer job is in " + status + " state, it is not possible to set the priority");
                }


            string cmd = "fts-set-priority " + job_id + " " + lexical_cast<string>(priority);

            DBSingleton::instance().getDBObjectInstance()->setPriority(job_id, priority);

            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << dn << " had set priority of transfer job: " << job_id << " to " << priority << commit;

        }
    catch(Err& ex)
        {
            // release the memory
            for (it = fileStatuses.begin(); it < fileStatuses.end(); ++it)
                {
                    if(*it)
                        delete *it;
                }
            fileStatuses.clear();
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            // release the memory
            for (it = fileStatuses.begin(); it < fileStatuses.end(); ++it)
                {
                    if(*it)
                        delete *it;
                }
            fileStatuses.clear();
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the priority cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__getSnapshot(soap* ctx, string vo, string src, string dst, impltns__getSnapshotResponse& resp)
{
    try
        {

            std::string endpoint = theServerConfig().get<std::string > ("Alias");
            std::stringstream result;
            DBSingleton::instance().getDBObjectInstance()->snapshot(vo, src, dst, endpoint, result);

            resp._result = result.str();
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: getSnapshot" << commit;
            soap_receiver_fault(ctx, "getSnapshot", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__detailedJobStatus(soap* ctx, std::string job_id, impltns__detailedJobStatusResponse& resp)
{

//	resp._detailedJobStatus = soap_new_impltns__detailedJobStatus(ctx, -1);

    try
        {
            // get the resource for authorization purposes
            scoped_ptr<TransferJobs> job(
                DBSingleton::instance().getDBObjectInstance()->getTransferJob(job_id, false)
            );
            // authorise
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job.get());
            // get the data from DB
            std::vector<boost::tuple<std::string, std::string, int, std::string, std::string> > files;
            DBSingleton::instance().getDBObjectInstance()->getTransferJobStatusDetailed(job_id, files);
            // create response
            tns3__DetailedJobStatus *jobStatus = soap_new_tns3__DetailedJobStatus(ctx, -1);
            // reserve the space in response
            jobStatus->transferStatus.reserve(files.size());
            // copy the data to response
            std::vector<boost::tuple<std::string, std::string, int, std::string, std::string> >::const_iterator it;
            for (it = files.begin(); it != files.end(); ++it)
                {
                    tns3__DetailedFileStatus * item = soap_new_tns3__DetailedFileStatus(ctx, -1);
                    item->jobId = boost::get<0>(*it);
                    item->fileState = boost::get<1>(*it);
                    item->fileId = boost::get<2>(*it);
                    item->sourceSurl = boost::get<3>(*it);
                    item->destSurl = boost::get<4>(*it);
                    jobStatus->transferStatus.push_back(item);
                }
            // set the response
            resp._detailedJobStatus = jobStatus;
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An exception has been caught: detailedJobStatus" << commit;
            soap_receiver_fault(ctx, "detailedJobStatus", "TransferException");
            return SOAP_FAULT;
        }

    return SOAP_OK;

    return SOAP_OK;
}
