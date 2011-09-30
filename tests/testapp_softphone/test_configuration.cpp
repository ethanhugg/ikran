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

const string TestConfiguration::USERNUMBER = "usernumber";
const string TestConfiguration::PASSWORD = "password";
const string TestConfiguration::SIPADDRESS = "sipaddress";
const string TestConfiguration::DEVICENAME = "devicename";
const string TestConfiguration::USEVIDEO = "usevideo";
const string TestConfiguration::USEP2P = "usep2p";
const string TestConfiguration::P2PADDRESS = "p2paddress";
const string TestConfiguration::NUMBERTODIAL = "numbertodial";
const string TestConfiguration::USEBATCHMODE = "usebatchmode";


TestConfiguration::TestConfiguration(void):_userNumber(), _userPassword(), _deviceName(), _sipProxyAddress(), _numberToDial(), _p2pAddress(), _useVideo(false), _useP2PMode(false), _useBatchMode(false){
	
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

	//convert the key to lowercase
	string k = key;
	transform( k.begin(), k.end(), k.begin(),::tolower );
	
	if(USERNUMBER == k){
		_userNumber = val;
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
	
	if(NUMBERTODIAL == k){
		_numberToDial = val;
		return;
	}
	
	if(USEVIDEO == k){
		_useVideo = (val == "true");
		return;
	}
	
	if(USEP2P == k){
		_useP2PMode = (val == "true");
		return;
	}
	if(P2PADDRESS == k){
		_p2pAddress = val;
	}
	
	if(USEBATCHMODE == k){
		_useBatchMode = (val == "true");
		return;
	}
	
	
	
}

/**
 *  Internal check to see if the configuration object has enough information
 *  to register and make a call
 **/
bool TestConfiguration::IsConfigured(void){
	//TODO: do we want to add specific sip proxy tests - e.g. CUCM requires a device name, asterisk doesn't etc..
	if(_useP2PMode){
		return !(_userNumber.empty());
	}
	else{
		return !(_userNumber.empty() || _sipProxyAddress.empty()); 
	}
}


/**
 *  Accessor methods
 **/
string TestConfiguration::GetUserNumber(void){
	return _userNumber;
}
void TestConfiguration::SetUserNumber(string usernumber){
	_userNumber = usernumber;
}

string TestConfiguration::GetPassword(void){
	return _userPassword;
}

void TestConfiguration::SetPassword(string password){
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

bool TestConfiguration::UseP2PMode(void){
	return _useP2PMode;
}
void TestConfiguration::SetUseP2PMode(bool useP2PMode){
	_useP2PMode = useP2PMode;
}

bool TestConfiguration::IsBatchMode(void){
	return _useBatchMode;
}
void TestConfiguration::SetBatchMode(bool useBatchMode){
	_useBatchMode = useBatchMode;
}

string TestConfiguration::GetNumberToDial(void){
	return _numberToDial;
}
void TestConfiguration::SetNumberToDial(string numberToDial){
	_numberToDial = numberToDial;
}

string TestConfiguration::GetP2PAddress(void){
	return _p2pAddress;
}

void TestConfiguration::SetP2PAddress(string p2pAddress){
	_p2pAddress = p2pAddress;
}


/**
 * returns a string representation of the object
 **/
string TestConfiguration::toString(void){
	string s = "Configuration information:";
	s += "\n user number: ";
	s += _userNumber;
	s += "\n password: ";
	s += _userPassword;
	s += "\n device name: ";
	s += _deviceName;
	s += "\n SIP proxy: ";
	s += _sipProxyAddress;
	s += "\n Use video: ";
	s += _useVideo ? "true" : "false";
	s += "\n Number to dial: ";
	s += _numberToDial;
	s += "\n Use P2P mode: ";
	s += _useP2PMode ? "true" : "false";
	s += "\n P2P address: ";
	s += _p2pAddress;
	s += "\n Use batch mode: ";
	s += _useBatchMode ? "true" : "false";
	s += "\n";
	
	return s;
}