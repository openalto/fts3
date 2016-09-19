/*
 * Copyright (c) CERN 2013-2016
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

#pragma once

#include <soci/soci.h>
#include "db/generic/GenericDbIfce.h"
#include "db/generic/StoragePairState.h"
#include "db/generic/SanityFlags.h"
#include "msg-bus/consumer.h"
#include "msg-bus/producer.h"

#define TIME_TO_SLEEP_BETWEEN_TRANSACTION_RETRIES 1


class MySqlAPI : public GenericDbIfce
{
public:
    MySqlAPI();
    virtual ~MySqlAPI();

    class CleanUpSanityChecks
    {
    public:
        CleanUpSanityChecks(MySqlAPI *instanceLocal, soci::session &sql, struct SanityFlags &msg) :
            instanceLocal(instanceLocal), sql(sql), msg(msg), returnValue(false)
        {
            returnValue = instanceLocal->assignSanityRuns(sql, msg);
        }

        ~CleanUpSanityChecks()
        {
            instanceLocal->resetSanityRuns(sql, msg);
        }

        bool getCleanUpSanityCheck()
        {
            return returnValue;
        }

        MySqlAPI* instanceLocal;
        soci::session& sql;
        struct SanityFlags &msg;
        bool returnValue;
    };

    struct HashSegment
    {
        unsigned start;
        unsigned end;

        HashSegment(): start(0), end(0xFFFF) {}
    } hashSegment;

    /// Initialize database connection by providing information from fts3config file
    /// @param nPooledConnections   The number connections to pool
    virtual void init(const std::string& username, const std::string& password,
        const std::string& connectString, int nPooledConnections);

    /// Recover from the DB transfers marked as ACTIVE for the host 'host'
    virtual std::list<fts3::events::MessageUpdater> getActiveInHost(const std::string &host);

    /// Get a list of transfers ready to go for the given queues
    /// When session reuse is enabled for a job, all the files belonging to that job should run at once
    /// @param queues       Queues for which to check (see getQueuesWithSessionReusePending)
    /// @param[out] files   A map where the key is the VO. The value is a queue of pairs (jobId, list of transfers)
    virtual void getReadySessionReuseTransfers(const std::vector<QueueId>& queues,
        std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile>>>>& files);

    /// Get a list of transfers ready to go for the given queues
    /// @param queues       Queues for which to check (see getQueuesWithPending)
    /// @param[out] files   A map where the key is the VO. The value is a list of transfers belonging to that VO
    virtual void getReadyTransfers(const std::vector<QueueId>& queues,
        std::map< std::string, std::list<TransferFile>>& files);

    /// Get a list of multihop jobs ready to go
    /// @param[out] files   A map where the key is the VO. The value is a queue of pairs (jobId, list of transfers)
    virtual void getMultihopJobs(std::map< std::string, std::queue<std::pair<std::string, std::list<TransferFile>>>>& files);

    /// Update the status of a transfer
    /// @param jobId            The job ID
    /// @param fileId           The file ID
    /// @param throughput       Transfer throughput
    /// @param transferState    Transfer statue
    /// @param errorReason      For failed states, the error message
    /// @param processId        fts_url_copy process ID running the transfer
    /// @param filesize         Actual filesize reported by the storage
    /// @param duration         How long (in seconds) took to transfer the file
    /// @param retry            If the error is considered recoverable by fts_url_copy
    /// @return                 true if an updated was done into the DB, false otherwise
    ///                         (i.e. trying to set ACTIVE an already ACTIVE transfer)
    /// @note                   If jobId is empty, or if fileId is 0, then processId will be used to decide
    ///                         which transfers to update
    virtual bool updateTransferStatus(const std::string& jobId, int fileId, double throughput,
        const std::string& transferState, const std::string& errorReason,
        int processId, double filesize, double duration, bool retry);

    /// Update the status of a job
    /// @param jobId            The job ID
    /// @param jobState         The job state
    /// @param pid              The PID of the fts_url_copy process
    /// @note                   If jobId is empty, the pid will be used to decide which job to update
    virtual bool updateJobStatus(const std::string& jobId, const std::string& jobState, int pid);

    /// Get the credentials associated with the given delegation ID and user
    /// @param delegationId     Delegation ID. See insertCredentialCache
    /// @param userDn           The user's DN
    /// @return                 The delegated credentials, if any
    virtual boost::optional<UserCredential> findCredential(const std::string& delegationId, const std::string& userDn);

    /// Check if the credential for the given delegation id and user is expired or not
    /// @param delegationId     Delegation ID. See insertCredentialCache
    /// @param userDn           The user's DN
    /// @return                 true if the stored credentials expired or do not exist
    virtual bool isCredentialExpired(const std::string& delegationId, const std::string &userDn);

    /// Get the debug level for the given pair
    /// @param sourceStorage    The source storage as protocol://host
    /// @param destStorage      The destination storage as protocol://host
    /// @return                 An integer with the debug level configured for the pair. 0 = no debug.
    virtual unsigned getDebugLevel(const std::string& sourceStorage, const std::string& destStorage);

    /// Check whether submission is allowed for the given storage
    /// @param storage          The storage to blacklist (as protocol://host)
    /// @param voName           The submitting VO
    virtual bool allowSubmit(const std::string& storage, const std::string& voName);

    /// Check is the user has been banned
    virtual bool isDnBlacklisted(const std::string& userDn);

    /// Optimizer data source
    virtual fts3::optimizer::OptimizerDataSource* getOptimizerDataSource();

    /// Checks if there are available slots to run transfers for the given pair
    /// @param sourceStorage        The source storage  (as protocol://host)
    /// @param destStorage          The destination storage  (as protocol://host)
    /// @param[out] currentActive   The current number of running transfers is put here
    virtual bool isTrAllowed(const std::string& sourceStorage, const std::string& destStorage, int &currentActive);

    virtual int getSeOut(const std::string & source, const std::set<std::string> & destination);

    virtual int getSeIn(const std::set<std::string> & source, const std::string & destination);

    /// Mark a reuse or multihop job (and its files) as failed
    /// @param jobId    The job id
    /// @param pid      The PID of the fts_url_copy
    /// @param message  The error message
    /// @note           If jobId is empty, the implementation may look for the job bound to the pid.
    ///                 Note that I am not completely sure you can get an empty jobId.
    virtual bool terminateReuseProcess(const std::string & jobId, int pid, const std::string & message);

    /// Goes through transfers marked as 'ACTIVE' and make sure the timeout didn't expire
    /// @param[out] collectJobs A map of fileId with its corresponding jobId that have been cancelled
    virtual void forceFailTransfers(std::map<int, std::string>& collectJobs);

    /// Set the PID for all the files inside a reuse or multihop job
    /// @param jobId    The job id for which the files will be updated
    /// @param pid      The process ID
    /// @note           Transfers within reuse and multihop jobs go all together to a single fts_url_copy process
    virtual void setPidForJob(const std::string& jobId, int pid);

    /// Search for transfers stuck in 'READY' state and revert them to 'SUBMITTED'
    /// @note   AFAIK 'READY' only applies for reuse and multihop, but inside
    ///         MySQL reuse seems to be explicitly filtered out, so I am not sure
    ///         how much is this method actually doing
    virtual void revertToSubmitted();

    /// Moves old transfer and job records to the archive tables
    /// Delete old entries in other tables (i.e. t_optimize_evolution)
    /// @param[in]          How many jobs per iteration must be processed
    /// @param[out] nJobs   How many jobs have been moved
    /// @param[out] nFiles  How many files have been moved
    virtual void backup(long bulkSize, long* nJobs, long* nFiles);

    /// Mark all the transfers as failed because the process fork failed
    /// @param jobId    The job id for which url copy failed to fork
    /// @note           This method is used only for reuse and multihop jobs
    virtual void forkFailed(const std::string& jobId);

    /// Mark the files contained in 'messages' as stalled (FAILED)
    /// @param messages Only file_id, job_id and process_id from this is used
    /// @param diskFull Set to true if there are no messages because the disk is full
    virtual bool markAsStalled(const std::vector<fts3::events::MessageUpdater>& messages, bool diskFull);

    /// Return true if the group 'groupName' exists
    virtual bool checkGroupExists(const std::string & groupName);

    /// @return the group to which storage belong
    /// @note   It will be the empty string if there is no group
    virtual std::string getGroupForSe(const std::string storage);

    //t_config_symbolic
    virtual std::unique_ptr<LinkConfig> getLinkConfig(std::string source, std::string destination);

    virtual void addShareConfig(const ShareConfig& cfg);
    virtual std::unique_ptr<ShareConfig> getShareConfig(std::string source, std::string destination, std::string vo);
    virtual std::vector<ShareConfig> getShareConfig(std::string source, std::string destination);

    virtual void addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo);

    virtual int countActiveTransfers(std::string source, std::string destination, std::string vo);

    virtual int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    virtual int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    virtual int sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos);

    virtual int getRetry(const std::string & jobId);

    virtual int getRetryTimes(const std::string & jobId, int fileId);

    virtual void setToFailOldQueuedJobs(std::vector<std::string>& jobs);

    virtual void updateProtocol(const std::vector<fts3::events::Message>& tempProtocol);

    virtual std::vector<TransferState> getStateOfTransfer(const std::string& jobId, int file_id);

    virtual void cancelWaitingFiles(std::set<std::string>& jobs);

    virtual void revertNotUsedFiles();

    virtual void checkSanityState();

    virtual void checkSchemaLoaded();

    virtual void setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason,
        int errcode);

    virtual void updateFileTransferProgressVector(std::vector<fts3::events::MessageUpdater>& messages);

    virtual void transferLogFileVector(std::map<int, fts3::events::MessageLog>& messagesLog);

    /**
     * Signals that the server is alive
     * The total number of running (alive) servers is put in count
     * The index of this specific machine is put in index
     * A default implementation is provided, as this is used for optimization,
     * so it is not mandatory.
     * start and end are set to the interval of hash values this host will process
     */
    virtual void updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end,
        std::string service_name);

    virtual unsigned int updateFileStatusReuse(TransferFile const & file, const std::string status);

    virtual void getCancelJob(std::vector<int>& requestIDs);

    virtual bool getDrain();

    virtual bool isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname);

    virtual bool isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname);

    virtual int getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname);

    virtual int getGlobalTimeout();

    virtual int getSecPerMb();

    virtual int getBufferOptimization();

    virtual void getQueuesWithPending(std::vector<QueueId>& queues);

    virtual void getQueuesWithSessionReusePending(std::vector<QueueId>& queued);

    /// Updates the status for delete operations
    /// @param delOpsStatus  Update for files in delete or started
    virtual void updateDeletionsState(const std::vector<MinFileStatus>& delOpsStatus);

    /// Gets a list of delete operations in the queue
    /// @params[out] delOps A list of namespace operations (deletion)
    virtual void getFilesForDeletion(std::vector<DeleteOperation>& delOps);

    /// Revert namespace operations already in 'STARTED' back to the 'DELETE'
    /// state, so they re-enter the queue
    virtual void requeueStartedDeletes();

    /// Updates the status for staging operations
    /// @param stagingOpStatus  Update for files in staging or started
    virtual void updateStagingState(const std::vector<MinFileStatus>& stagingOpStatus);

    //  job_id / file_id / token
    virtual void updateBringOnlineToken(std::map< std::string,
        std::map<std::string, std::vector<int> > > const & jobs, std::string const & token);

    /// Get staging operations ready to be started
    /// @params[out] stagingOps The list of staging operations will be put here
    virtual void getFilesForStaging(std::vector<StagingOperation> &stagingOps);

    /// Get staging operations already started
    /// @params[out] stagingOps The list of started staging operations will be put here
    virtual void getAlreadyStartedStaging(std::vector<StagingOperation> &stagingOps);

    //file_id / surl / token
    virtual void getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files);

    /// Retrieve the credentials for a cloud storage endpoint for the given user/VO
    virtual bool getCloudStorageCredentials(const std::string& userDn,
        const std::string& voName,
        const std::string& cloudName,
        CloudStorageAuth& auth);

    /// Get if the user dn should be visible or not in the logs
    virtual bool getUserDnVisible();

private:
    size_t                poolSize;
    soci::connection_pool* connectionPool;
    std::string           hostname;
    std::string username_;
    std::map<std::string, int> queuedStagingFiles;

    Producer           producer;
    Consumer           consumer;

    bool assignSanityRuns(soci::session& sql, struct SanityFlags &msg);
    void resetSanityRuns(soci::session& sql, struct SanityFlags &msg);

    void updateHeartBeatInternal(soci::session& sql, unsigned* index, unsigned* count, unsigned* start, unsigned* end,
        std::string service_name);

    std::map<std::string, int> getFilesNumPerActivity(soci::session& sql,
        std::string src, std::string dst, std::string vo, int filesNum,
        std::set<std::string> & default_activities);

    std::map<std::string, long long> getActivitiesInQueue(soci::session& sql, std::string src,
        std::string dst, std::string vo);

    std::map<std::string, double> getActivityShareConf(soci::session& sql, std::string vo);

    void updateDeletionsStateInternal(soci::session& sql, const std::vector<MinFileStatus> &delOpsStatus);

    void updateStagingStateInternal(soci::session& sql, const std::vector<MinFileStatus> &stagingOpsStatus);

    bool updateFileTransferStatusInternal(soci::session& sql, double throughput, std::string jobId, int fileId,
        std::string newState, std::string transferMessage, int processId, double filesize, double duration, bool retry);

    bool updateJobTransferStatusInternal(soci::session& sql, std::string job_id, const std::string status, int pid);

    void transferLogFileVectorInternal(soci::session& sql, std::map<int, fts3::events::MessageLog>& messagesLog);

    bool resetForRetryStaging(soci::session& sql, int file_id, const std::string & job_id, bool retry, int& times);

    bool resetForRetryDelete(soci::session& sql, int file_id, const std::string & job_id, bool retry);

    void setRetryTransferInternal(soci::session &sql, const std::string &jobId, int fileId, int retry,
        const std::string &reason, int errcode);

    int getBestNextReplica(soci::session& sql, const std::string & job_id, const std::string & vo_name);

    std::vector<TransferState> getStateOfTransferInternal(soci::session& sql, const std::string& jobId, int fileId);

    std::vector<TransferState> getStateOfDeleteInternal(soci::session& sql, const std::string& jobId, int fileId);

    void useFileReplica(soci::session& sql, std::string jobId, int fileId);

    int getCredits(soci::session& sql, const std::string & sourceSe, const std::string & destSe);

    bool getDrainInternal(soci::session& sql);

    int getMaxTimeInQueue();

    bool getUserDnVisibleInternal(soci::session& sql);

    int getStreamsOptimizationInternal(soci::session &sql, const std::string &sourceSe,
        const std::string &destSe);


    // Sanity checks
    void fixJobNonTerminallAllFilesTerminal(soci::session &sql);
    void fixJobTerminalFileNonTerminal(soci::session &sql);
    void fixDeleteInconsistencies(soci::session &sql);
    void recoverFromDeadHosts(soci::session &sql);
    void recoverStalledStaging(soci::session &sql);

    void fixEmptyJob(soci::session &sql, const std::string &jobId);
    void fixNonTerminalJob(soci::session &sql, const std::string &jobId,
        uint64_t filesInJob, uint64_t cancelCount, uint64_t finishedCount, uint64_t failedCount);
};
