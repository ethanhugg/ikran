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
 *  Webrtc Authors
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *  Cary Bran <cary.bran@gmail.com>
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

/*
 * This is a light weight Test application for
 * ikran video testing on OSX.
 * Presently only following functionalities are supported
 *   +Registration
 *   + DeRegistration
 *   + Place Call
 *   + End Call
 *   + Answer Call 
 *
 */

#include <iostream>

#include "test_main.h"
#include "test_mac_cocoa.h"


WindowManager* windowManager; 
bool inCall = false;
/*
 * Callback method from Sip Call Controller
 * on Incoming call event
 */
void TestMain::OnIncomingCall(std::string callingPartyName,
									std::string callingPartyNumber)
{
	_state = STATE_INCOMING_CALL;
	cout <<" Incoming Call From : " << endl;
	cout << callingPartyName << endl;
	cout << callingPartyNumber << endl;
	SipccController::GetInstance()->AnswerCall();
	_state = STATE_IN_CALL;
} 

/*
 * Callback from Sip Call Controller on
 * change in SIP Registration State
 */
void TestMain::OnRegisterStateChange(std::string registrationState)
{

} 

/*
 * Callback from Sip Call Controller on
 * termination of call (local/remote)
 */
void TestMain::OnCallTerminated()
{
	_state = STATE_REGISTERED;
} 

/*
 * Callback from Sip Call Controller on
 * media connected between the peers 
 */
void TestMain::OnCallConnected(){
	_state = STATE_IN_CALL;
}

void TestMain::OnCallHeld(){
}

void TestMain::OnCallResume(){
}

TestMain::TestMain(){
	_config = new TestConfiguration();
	_state = STATE_NOT_REGISTERED;
}

void TestMain::ManualConfiguration(){
	GetStartupMode();
	
	GetUserPhoneNumber();
	
	GetPhoneNumberToCall();
	
	GetVideoCalling();
	
	//get mode specific settings
	if (_config->UseP2PMode()) {
		GetP2PCallAddress();
	}
	else{
		GetSIPProxy();
	
		GetPassword();
		
		GetDeviceName();
	}	
}

/**
 * Loads configuration settings from a user specified file path
 */
void TestMain::LoadConfigurationFromFile(){
	string input;
	//draw the info to the screen
	cout << "Configuration file should have the following entries (one per line)" << endl;
	cout << "usernumber=<your phone number> a DN for cucm could be something else for other systems" << endl;
	cout << "password=<some password>" << endl;
	cout << "sipaddress=<ip address to sip proxy>" << endl;
	cout << "devicename=<name of device> cucm needs this" << endl;
	cout << "usevideo=<true or false> for video calling>" << endl;
	cout << "usep2p=<true or false> use p2p mode"<< endl;
	cout << "p2paddress=<ip address> ip address for P2P calling"<< endl;
	cout << "numbertodial=<phone number> the number to call"<< endl;
	cout << "usebatchmode=<true or false> use the configuration file to drive the call"<< endl;
	cout << endl;
	cout << "Enter path to configuration file:" << endl;
	
	//read in the user input
	getline(cin, input, '\n');
	
	ReadConfig(input);
	
}

/**
 * Reads in the file configuration
 * Assumes filePath will not be null and has some value
 **/
void TestMain::ReadConfig(string filePath){
	if(filePath.length() > 0){
		//read in the configuration
		if(_config->ReadConfigFromFile(filePath)){
			cout << "Configuration read from: " << filePath << endl;
			cout << "*************************************************" << endl << endl;
			cout << _config->toString() << endl;
			cout << "*************************************************" << endl << endl;
			
		}
		else{
			cout << "ERROR: failed to read configuration from file " << filePath << endl;
		}
		
	}
}

void TestMain::Register(){
	
	//P2P Registeration
	if(_config->UseP2PMode()) {
		RegisterP2P();
	}
	else if (_config->UseROAPMode()) {
		StartROAPProxy();
	}
	else {
		RegisterSIPProxy();
	}
	
	return;
	
}

void TestMain::RegisterP2P(){
		
	cout << "Setting up for P2P Mode";
	if(_config->GetUserNumber().empty()){
		GetUserPhoneNumber();
		//error case for empty 
		if(_config->GetUserNumber().empty()){
			cout << "ERROR: invalid user number cannot make P2P call";
			_state = STATE_NOT_REGISTERED;
			return;
		}			
	}
	SipccController::GetInstance()->StartP2PMode(_config->GetUserNumber());
	_state = STATE_REGISTERED;
}

void TestMain::RegisterSIPProxy(){
	
	//SIP Proxy Registration
	if (!_config->IsConfigured()) {
		cout << "Could not register!" << endl; 
		cout << "Please review your configuration information: " << endl;
		cout << "*************************************************" << endl << endl;
		cout << _config->toString() << endl;
		cout << "*************************************************" << endl << endl;
		
		return;
	}
	
	if (_state == STATE_REGISTERED) {
		cout << "Already registered, ignoring registration request" << endl;
		return;
	}
	
	cout << "Registering to SIP Proxy with " << _config->GetSIPProxyAddress() << endl;

	SipccController::GetInstance()->SetProperty("transport", "tcp");
		
	int regResult = SipccController::GetInstance()->Register(_config->GetDeviceName(), _config->GetUserNumber(), _config->GetPassword(), _config->GetSIPProxyAddress());
	//zero means happy here
	//TODO - maybe finer grained results?
	if (regResult != 0) {
		cout << "ERROR registering with " << _config->GetSIPProxyAddress() << " check your log files " << endl;
		_state = STATE_NOT_REGISTERED;
	}
	else {
		cout << "Successfully registered with " << _config->GetSIPProxyAddress() << endl;
		_state = STATE_REGISTERED;
	}
	
}

void TestMain::StartROAPProxy(){

	//SIP Proxy Registration
	if (!_config->IsConfigured()) {
		cout << "Could not register!" << endl;
		cout << "Please review your configuration information: " << endl;
		cout << "*************************************************" << endl << endl;
		cout << _config->toString() << endl;
		cout << "*************************************************" << endl << endl;

		return;
	}

	if (_state == STATE_REGISTERED) {
		cout << "Already registered, ignoring registration request" << endl;
		return;
	}

	cout << "Registering to SIP Proxy as ROAP Proxy with " << _config->GetSIPProxyAddress() << endl;

	SipccController::GetInstance()->SetProperty("transport", "tcp");

	int regResult = SipccController::GetInstance()->StartROAPProxy(_config->GetDeviceName(), _config->GetUserNumber(), _config->GetPassword(), _config->GetSIPProxyAddress());
	//zero means happy here
	//TODO - maybe finer grained results?
	if (regResult != 0) {
		cout << "ERROR registering with " << _config->GetSIPProxyAddress() << " check your log files " << endl;
		_state = STATE_NOT_REGISTERED;
	}
	else {
		cout << "Successfully registered with " << _config->GetSIPProxyAddress() << endl;
		_state = STATE_REGISTERED;
	}

}


/**
 * Places a phone call and will prompt the user for a phone number if 
 * one is not set already
 **/
void TestMain::PlaceCall(){
	if (_state == STATE_NOT_REGISTERED) {
		Register();
	}
	//if we still can't register or are in some other state quit
	if(_state != STATE_REGISTERED){
		cout << "ERROR: Cannot place call, currently in a state other than registered" << endl;
		return;
	}
	
	if (_config->GetNumberToDial().empty()) {
		//try to get a phone number
		GetPhoneNumberToCall();
		//error condition, no input
		if (_config->GetNumberToDial().empty()) {
			cout << "ERROR: no number to dial specified, call aborted" << endl;
			return;
		}
	}
	
	//handle P2P case
	if (_config->UseP2PMode() && _config->GetP2PAddress().empty()) {
		//try to get a phone number
		GetP2PCallAddress();
		//error condition, no input
		if (_config->GetP2PAddress().empty()) {
			cout << "ERROR: no P2P address specified, call aborted" << endl;
			return;
		}
	}
	
	if (_config->UseVideo()) {
		cout << "setting up for video call";
		SipccController::GetInstance()->SetVideoWindow(windowManager->GetVideoWindow());
	}
	
    if(_config->UseP2PMode()){
		cout << "Calling P2P using: number = " << _config->GetNumberToDial() << " address = " << _config->GetP2PAddress() << endl;
		SipccController::GetInstance()->PlaceP2PCall(_config->GetNumberToDial(), _config->GetP2PAddress());
	}
	else{
		cout << "Calling: " << _config->GetNumberToDial() << endl;
		SipccController::GetInstance()->PlaceCall(_config->GetNumberToDial(), (char *) "");
	}
	_state = STATE_IN_CALL;	
}


/**
 * Terminates the call
 **/
void TestMain::EndCall(){
	if(_state == STATE_IN_CALL){
		SipccController::GetInstance()->EndCall(); 
		inCall = false;
		_state = STATE_REGISTERED;
	}
	else {
		cout << "Not in a call right now, nothing to end" << endl;
	}	
}

/**
 * Renders the user input screen
 */
int TestMain::GetUserInput(){
	int userInput = 0;
	cout << "********************************" << endl;  
	if (_config->IsBatchMode()) {
		cout << "*********BATCH MODE*************" << endl;  
	}
	else{
		cout << "********MANUAL MODE*************" << endl;  
	}
	cout << "********************************" << endl;  
	
	cout << "Please Choose One : " << endl;
	cout << "\t 0. Quit" << endl;
	cout << "\t 1. Call" << endl;
	cout << "\t 2. Answer" << endl;
	cout << "\t 3. Hang up" << endl;
	cout << "\t 4. Load configuration from file" << endl;
	cout << "\t 5. Manual configuration" << endl;
	cout << "\t 6. Print current configuration" << endl;
	
	cout << "Select Action: " << endl;
	scanf("%d", &userInput);
	//blocking call
	getchar();
	cout << "********************************" << endl; 
	
	return userInput;
}


/**
 * Processes the user entered input and calls the appropriate operations
 */
void TestMain::ProcessInput(int op){
	string input;
	string dn;
	switch (op){
		case USER_INPUT_QUIT:
			//check to make sure we should be calling this before quitting
			if (_state == STATE_REGISTERED) {
				cout << "Unregistering from SIP Proxy" << endl;
				SipccController::GetInstance()->UnRegister();
			}
			break;
		case USER_INPUT_PLACE_CALL:
			PlaceCall(); 
			break;
		case USER_INPUT_ANSWER_CALL:
			if(_state == STATE_INCOMING_CALL){
				SipccController::GetInstance()->AnswerCall();
			}
			else {
				cout << "No incoming call to answer" << endl;
			}

			break;
		case USER_INPUT_END_CALL:
			EndCall();
			break;
		case USER_INPUT_FILE_CONFIG:
			LoadConfigurationFromFile();
			break;
		case USER_INPUT_PRINT_CONFIG:
			cout << _config->toString();
			break;
		case USER_INPUT_MANUAL_CONFIG:
			ManualConfiguration();
			break;
			
		default:
			break;
	}
			
	
}
//Begin - Get User Input Section////////////

void TestMain::GetStartupMode(){
	string p2p;
	//cout  << "p2p, sip or ROAP? [p2p|sip|roap]: [current value = " << (_config->UseP2PMode() ? "p2p" : "sip") << "] " ;
	cout  << "p2p, sip or ROAP? [p2p|sip|roap]:";
	getline(cin, p2p, '\n');
	if(p2p.length() > 0){
		transform( p2p.begin(), p2p.end(), p2p.begin(),::tolower );

		if(p2p == "p2p" && !_config->UseP2PMode()) {
			_config->SetUseP2PMode(true);
		}
		else if(p2p == "roap") {
			_config->SetUseROAPMode(true);
			_config->SetUseP2PMode(false);
		}
		else {
			_config->SetUseP2PMode(false);
		}
	}
	//clean out any previous configurations
	if (_state == STATE_REGISTERED) {
		cout << "Switching mode unregistering with " << _config->GetSIPProxyAddress() << endl;
		//assume this is already registered with CUCM, so unregister
		SipccController::GetInstance()->UnRegister();
		_state = STATE_NOT_REGISTERED;
	}
}


void TestMain::GetDeviceName(){
	string dn;
	cout << "Set device name (only required for CUCM): [current value = " << _config->GetDeviceName() << "] " ;
	getline(cin, dn, '\n');
	if(dn.length() > 0){
		_config->SetDeviceName(dn);
	}	
	
}

void TestMain::GetPassword(){
	string password;
	cout << "Set password (not required for CUCM) [ Enter ]:" ;
	getline(cin, password, '\n');
	if(password.length() > 0){
		_config->SetPassword(password);
	}
}


void TestMain::GetSIPProxy(){
	string sipProxy;
	cout << "Set SIP proxy IP address: [current value = " << _config->GetSIPProxyAddress() << "] " ;
	getline(cin, sipProxy, '\n');
	if(sipProxy.length() > 0){
		_config->SetSIPProxyAddress(sipProxy);
	}
}


void TestMain::GetVideoCalling(){
	string videoCalling;
	cout << "Use video calling? [yes|no]: [current value = " << (_config->UseVideo() ? "yes" : "no") << "] " ; 
	getline(cin, videoCalling, '\n');
	if(videoCalling.length() > 0){
		transform(videoCalling.begin(), videoCalling.end(), videoCalling.begin(),::tolower );
		_config->SetUseVideo(videoCalling == "yes");
	}	
}


void TestMain::GetPhoneNumberToCall(){
	string phonenumber;
	cout << "Enter user name or phone number to dial: [current value = " << _config->GetNumberToDial() << "] " ;
	getline(cin, phonenumber, '\n');
	if(phonenumber.length() > 0){
		_config->SetNumberToDial(phonenumber);
	}
}

void TestMain::GetUserPhoneNumber(){
	string phonenumber;
	cout << "Enter your user name or phone number (CUCM needs a DN): [current value = " << _config->GetUserNumber() << "] " ;
	getline(cin, phonenumber, '\n');
	if(phonenumber.length() > 0){
		_config->SetUserNumber(phonenumber);
	}
}

void TestMain::GetP2PCallAddress(){
	string address;
	cout << "Enter caller's P2P address: [current value = " << _config->GetP2PAddress() << "] " ;
	getline(cin, address, '\n');
	if(address.length() > 0){
		_config->SetP2PAddress(address);
	}
}
//End - Get User Input Section////////////


bool TestMain::BeginOSIndependentTesting(string pathToConfigFile){
	
    // Create platform dependent render windows
    windowManager = new WindowManager();
	
	//if the configuration file path is set, then configure from the file
	if(!pathToConfigFile.empty()){
		ReadConfig(pathToConfigFile);
	}
	
	if (_config->IsBatchMode()) {
		if (_config->IsConfigured()) {
			Register();
			PlaceCall();
		}
		else {
			cout << "ERROR: Configuration file is missing information to run batch mode please review and rerun";
			cout << _config->toString();
			return false;
		}
	}
		
	int op = USER_INPUT_QUIT;
	do {
		//blocking call
		op = GetUserInput();
		
		ProcessInput(op);
			
	}while (op != USER_INPUT_QUIT);
		
	
	printf("Press enter to quit...");
    
	delete windowManager;

    return true;
}

