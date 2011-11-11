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

#ifndef TEST_MAIN_H_
#define TEST_MAIN_H_

#if defined __APPLE__

#include <string>
#include "sipcc_controller.h"
#include "test_configuration.h"

using namespace std;

//states that the test app can be in
#define STATE_NOT_REGISTERED 0
#define STATE_REGISTERED 1
#define STATE_IN_CALL 2
#define STATE_INCOMING_CALL 3

//user input 
#define USER_INPUT_QUIT 0
#define USER_INPUT_PLACE_CALL 1
#define USER_INPUT_ANSWER_CALL 2
#define USER_INPUT_END_CALL 3
#define USER_INPUT_FILE_CONFIG 4
#define USER_INPUT_MANUAL_CONFIG 5
#define USER_INPUT_PRINT_CONFIG 6

class TestMain :
		public SipccControllerObserver
{
public:
    TestMain();
	bool BeginOSIndependentTesting(string pathToConfigFile);
	virtual void OnIncomingCall(string callingPartyName, string callingPartyNumber);
 	virtual void OnRegisterStateChange(string registrationState);
 	virtual void OnCallTerminated(); 
	virtual void OnCallConnected(char* sdp);
	virtual void OnCallHeld();
	virtual void OnCallResume();
private:
	void ManualConfiguration();
	void LoadConfigurationFromFile();
	void ReadConfig(string filePath);
	void Register();
	void PlaceCall();
	int  GetUserInput();
	void ProcessInput(int op);
	void GetUserPhoneNumber();
	void GetPhoneNumberToCall();
	void GetP2PCallAddress();
	void EndCall();
	void RegisterSIPProxy();
	void RegisterP2P();
	void StartROAPProxy();
	void GetStartupMode();
	void GetVideoCalling();
	void GetSIPProxy();
	void GetPassword();
	void GetDeviceName();
	
	int _state;
	bool videoWinEnabled;	
	
	TestConfiguration* _config;
	string p2pIPAddress;
};

#endif
#endif  // TEST_MAIN_H_
