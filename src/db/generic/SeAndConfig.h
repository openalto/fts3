/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 ***********************************************/

/**
 * @file SeAndConfig.h
 * @brief model SeAndConfig
 * @author Michail Salichos
 * 
 **/


#pragma once

#include <iostream>

class SeAndConfig {
	
public:
   SeAndConfig(){}
   ~SeAndConfig(){}
      			
	std::string ENDPOINT;
	std::string SE_TYPE;
	std::string SITE;
	std::string NAME;
	std::string STATE;
	std::string VERSION;
	std::string HOST;
    	std::string SE_TRANSFER_TYPE;
	std::string SE_TRANSFER_PROTOCOL;
	std::string SE_CONTROL_PROTOCOL;
	std::string GOCDB_ID;	
  	std::string SE_NAME; 
 	std::string SHARE_TYPE; 
 	std::string SHARE_ID; 
 	std::string SHARE_VALUE; 	

};
