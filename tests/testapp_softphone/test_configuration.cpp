/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Cary Bran <cbran@cisco.com>
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <iostream>
#include <fstream>


#include "test_configuration.h"


TestConfiguration::TestConfiguration(void):_userName(), _userPassword(), _deviceName(), _sipProxyAddress(), _useVideo(false){
	
}

/**
 * Reads in a file to initialize the object
 *
 * The file format should be <name>=<value> pairs
 * The following are expected values for initializing theo object
 * username=<some user name>
 * password=<some password>
 * sipaddress=<ip address to sip proxy>
 * devicename=<name of device>
 * audioonly=<true or false>
 * 
 * Any other value pairs will be ignored
 *
 **/
bool TestConfiguration::ReadConfigFromFile(string pathToConfigurationFile){
	if (pathToConfigurationFile.empty()) {
		cout << "ERROR: file not specified configuration load failed" << endl;
		return false;
	}
	
	ifstream config(pathToConfigurationFile.c_str());
	if(!config.is_open()){
		cout << "ERROR: couldn't open file: " << pathToConfigurationFile << endl;
		return false;
	}
	string line;
	while (config.good()) {
		getline( config, line);
		//line will be <key>=<value>, if not ignore it
		size_t pos = line.find("=");
		if(pos == string::npos || int(pos) == 0){
			cout << "INFO: file contains unrecognized line (skipping): " << line << endl;
			continue;
		}
		
		//split the string along the first equal sign, using substr b/c there may be additional "=" signs after the
		//first occurance of "="
		string key = line.substr(0, int(pos));
		
		string val = line.substr(int(pos) + 1, line.size()-1);
		
		AddConfigurationSetting(key, val);
		
	}
	config.close();
	
	return true;	
}

/**
 * tries to match the key with an existing property member variable, if there
 * is a match then the property variable is set to the value
 **/
void TestConfiguration::AddConfigurationSetting(string key, string val){
	const string USERNAME = "username";
	const string PASSWORD = "password";
	const string SIPADDRESS = "sipaddress";
	const string DEVICENAME = "devicename";
	const string USEVIDEO = "usevideo";

	//convert the key to lowercase
	string k = key;
	transform( k.begin(), k.end(), k.begin(),::tolower );
	
	if(USERNAME == k){
		_userName = val;
		return;
	}
	
	if(PASSWORD == k){
		_userPassword = val;
		return;
	}
	
	if(SIPADDRESS == k){
		_sipProxyAddress = val;
		return;
	}
	
	if(DEVICENAME == k){
		_deviceName = val;
		return;
	}
	
	if(USEVIDEO == k){
			_useVideo = (val == "true");
	}
	
}


/**
 *  Accessor methods
 **/
string TestConfiguration::GetUserName(void){
	return _userName;
}
void TestConfiguration::SetUserName(string username){
	_userName = username;
}

string TestConfiguration::GetUserPassword(void){
	return _userPassword;
}

void TestConfiguration::SetUserPassword(string password){
	_userPassword = password;
}

string TestConfiguration::GetDeviceName(void){
	return _deviceName;
}

void TestConfiguration::SetDeviceName(string deviceName){
	_deviceName = deviceName;
}

string TestConfiguration::GetSIPProxyAddress(void){
	return _sipProxyAddress;
}

void TestConfiguration::SetSIPProxyAddress(string address){
	_sipProxyAddress = address;
}

bool TestConfiguration::UseVideo(void){
	return _useVideo;
}

void TestConfiguration::SetUseVideo(bool useVideo){
	_useVideo = useVideo;
}


/**
 * returns a string representation of the object
 **/
string TestConfiguration::toString(void){
	string stringRepresentation = "Configuration information:";
	stringRepresentation += "\n user name: ";
	stringRepresentation += _userName;
	stringRepresentation += "\n password: ";
	stringRepresentation += _userPassword;
	stringRepresentation += "\n device name: ";
	stringRepresentation += _deviceName;
	stringRepresentation += "\n SIP proxy: ";
	stringRepresentation += _sipProxyAddress;
	stringRepresentation += "\n Use video: ";
	stringRepresentation += _useVideo ? "true" : "false";
	stringRepresentation += "\n";
	
	return stringRepresentation;
}