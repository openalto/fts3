/*
 * JobStatusHandler.h
 *
 *  Created on: Mar 9, 2012
 *      Author: simonm
 */

#ifndef JOBSTATUSHANDLER_H_
#define JOBSTATUSHANDLER_H_

#include "common/ThreadSafeInstanceHolder.h"

#include <map>
#include <string>
#include <iostream>

using namespace std;
using namespace boost;

namespace fts3 { namespace common {

/**
 * The JobStatusHandler class takes care of job status related operations.
 *
 * The class implements Singleton design pattern (derives from ThreadSafeInstanceHolder),
 * and grands access to a map containing all state names and the corresponding
 * IDs. The class inherits after MonitorObject and is thread safe.
 */
class JobStatusHandler: public ThreadSafeInstanceHolder<JobStatusHandler> {

	friend class ThreadSafeInstanceHolder<JobStatusHandler>;

public:

	/**
	 * Possible Job statuses.
	 *
	 * Value lower than 0 indicates that the job failed, value equal to 0 indicates
	 * that the job has been finished, value greater than 0 indicates that the job
	 * is still being processed
	 */
	enum JobStateEnum {
	    FTS3_STATUS_FAILED_ID = -6,
	    FTS3_STATUS_CATALOGFAILED_ID = -5,
	    FTS3_STATUS_FINISHED_DIRTY_ID = -4,
	    FTS3_STATUS_UNKNOWN_ID = -3,
	    FTS3_STATUS_CANCELED_ID = -2,
	    FTS3_STATUS_TRANSFERFAILED_ID = -1,
	    FTS3_STATUS_FINISHED_ID = 0,
	    FTS3_STATUS_SUBMITTED_ID,
	    FTS3_STATUS_PENDING_ID,
	    FTS3_STATUS_ACTIVE_ID,
	    FTS3_STATUS_CANCELING_ID,
	    FTS3_STATUS_WAITING_ID,
	    FTS3_STATUS_HOLD_ID,
	    FTS3_STATUS_DONE_ID,
	    FTS3_STATUS_READY_ID,
	    FTS3_STATUS_DONEWITHERRORS_ID,
	    FTS3_STATUS_FINISHING_ID,
	    FTS3_STATUS_AWAITING_PRESTAGE_ID,
	    FTS3_STATUS_PRESTAGING_ID,
	    FTS3_STATUS_WAITING_PRESTAGE_ID,
	    FTS3_STATUS_WAITING_CATALOG_RESOLUTION_ID,
	    FTS3_STATUS_WAITING_CATALOG_REGISTRATION_ID
	};

	///@{
	/**
	 * names of transfer job statuses
	 */
    static const string FTS3_STATUS_FAILED;
    static const string FTS3_STATUS_CATALOGFAILED;
    static const string FTS3_STATUS_FINISHED_DIRTY;
    static const string FTS3_STATUS_UNKNOWN;
    static const string FTS3_STATUS_CANCELED;
    static const string FTS3_STATUS_TRANSFERFAILED;
    static const string FTS3_STATUS_FINISHED;
    static const string FTS3_STATUS_SUBMITTED;
    static const string FTS3_STATUS_PENDING;
    static const string FTS3_STATUS_ACTIVE;
    static const string FTS3_STATUS_CANCELING;
    static const string FTS3_STATUS_WAITING;
    static const string FTS3_STATUS_HOLD;
    static const string FTS3_STATUS_DONE;
    static const string FTS3_STATUS_READY;
    static const string FTS3_STATUS_DONEWITHERRORS;
    static const string FTS3_STATUS_FINISHING;
    static const string FTS3_STATUS_AWAITING_PRESTAGE;
    static const string FTS3_STATUS_PRESTAGING;
    static const string FTS3_STATUS_WAITING_PRESTAGE;
    static const string FTS3_STATUS_WAITING_CATALOG_RESOLUTION;
    static const string FTS3_STATUS_WAITING_CATALOG_REGISTRATION;
	///@}

	/**
	 * Destructor (empty).
	 */
	virtual ~JobStatusHandler() {};

	/**
	 * Check whether the given status name indicates that a transfer is ready
	 *
	 * @return true if the transfer is ready
	 */
	bool isTransferReady(string status);

	/**
	 * Check weather the given state is valid
	 * (is one of the recognized states)
	 *
	 * @return true if the the state is valid
	 */
	bool isStatusValid(string status);

	/**
	 *
	 */
	template <typename JS>
	static void printJobStatus(JS* js){

		cout << "Request ID:\t" << *js->jobID << endl;
		cout << "Status: " << *js->jobStatus << endl;
		cout << "Client DN: " << *js->clientDN << endl;

		if (js->reason) {

			cout << "Reason: " << *js->reason << endl;

		} else {

			cout << "Reason: <None>" << endl;
		}

		cout << "Submit time: " << ""/*TODO*/ << endl;
		cout << "Files: " << js->numFiles << endl;
	    cout << "Priority: " << js->priority << endl;
	    cout << "VOName: " << *js->voName << endl;
	}

private :

	/**
	 * Default constructor
	 *
	 * Private, should not be used
	 */
	JobStatusHandler();

	/**
	 * Copying constructor
	 *
	 * Private, should not be used
	 */
	JobStatusHandler(JobStatusHandler const&);

	/**
	 * Assignment operator
	 *
	 * Private, should not be used
	 */
	JobStatusHandler & operator=(JobStatusHandler const&);

	/// maps job status name to job status id
	const map<string, JobStateEnum> statuses;
};


}
}
#endif /* JOBSTATUSHANDLER_H_ */
