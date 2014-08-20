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
 * fts3-config-get.cpp
 *
 *  Created on: Apr 3, 2012
 *      Author: Michał Simon
 */

#include "GSoapContextAdapter.h"
#include "MsgPrinter.h"
#include "ui/SnapshotCli.h"

#include "common/JobStatusHandler.h"

#include "exception/cli_exception.h"
#include "JsonOutput.h"

#include <iostream>
#include <string>


#include <boost/scoped_ptr.hpp>

#include <boost/property_tree/json_parser.hpp>

using namespace boost;
using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-cancel command line tool.
 */
int main(int ac, char* av[])
{
    try
        {
            SnapshotCli cli;
            // create and initialize the command line utility
            cli.parse(ac, av);
            if (!cli.validate()) return 1;

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter ctx (cli.getService());
            cli.printApiDetails(ctx);

            std::string src = cli.getSource(), dst = cli.getDestination(), vo = cli.getVo();


            std::string resp = ctx.getSnapShot(vo, src, dst);
            if(!resp.empty())
                std::cout << resp << std::endl;
            else
                std::cout << "{\"Message\":\"Nothing returned for the parameters provided\"}" << std::endl;

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
