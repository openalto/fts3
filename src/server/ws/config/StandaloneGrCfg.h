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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * StandaloneGrCfg.h
 *
 *  Created on: Nov 19, 2012
 *      Author: Michal Simon
 */

#ifndef STANDALONEGRCFG_H_
#define STANDALONEGRCFG_H_

#include "StandaloneCfg.h"

#include <string>
#include <vector>

namespace fts3 {
namespace ws {

using namespace std;
using namespace fts3::common;

/**
 * The standalone SE group configuration
 * 	it is derived from StandaloneCfg
 *
 * @see StandaloneCfg
 */
class StandaloneGrCfg : public StandaloneCfg {

public:

	/**
	 * Constructor.
	 *
	 * Retrieves the configuration data from the DB.
	 *
	 * @param dn - client's DN
	 * @param name - SE group name
	 */
	StandaloneGrCfg(string dn, string name);

	/**
	 * Constructor.
	 *
	 * Retrieves the configuration data from the given CfgParser for the given SE group
	 *
	 * @param dn - client's DN
	 * @param parser - an object that has been used for parsing the JSON configuration
	 */
	StandaloneGrCfg(string dn, CfgParser& parser);

	/**
	 * Destructor.
	 */
	virtual ~StandaloneGrCfg();

	/**
	 * Creates a string containing the JSON configuration common for a SE group 'standalone' configurations
	 */
	virtual string json();


	/**
	 * Saves the current configuration into the DB
	 */
	virtual void save();

	/**
	 * Removes the configuration from the DB
	 */
	virtual void del();

private:

	/// SE group name
	string group;
	/// SE group members
	vector<string> members;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONEGRCFG_H_ */
