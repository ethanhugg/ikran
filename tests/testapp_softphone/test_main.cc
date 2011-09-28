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

	cout <<" Incoming Call From : " << endl;
	cout << callingPartyName << endl;
	cout << callingPartyNumber << endl;
	SipccController::GetInstance()->AnswerCall();
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
	inCall = false;
} 

/*
 * Callback from Sip Call Controller on
 * media connected between the peers 
 */
void TestMain::OnCallConnected()
{

}

TestMain::TestMain(){
	_config = new TestConfiguration();
}

void TestMain::ManualConfiguration()
{
	std::string input;  
	cout << "Enter SIP server IP address: [current value = " << _config->GetSIPProxyAddress() << "] " ;
	getline(cin, input, '\n');
	if(input.length() > 0){
		_config->SetSIPProxyAddress(input);
	}
	
	cout << "Enter SIP server username (phone DN for CUCM): [current value = " << _config->GetUserName() << "] " ;
	getline(cin, input, '\n');
	if(input.length() > 0){
		_config->SetUserName(input);
	}
	
	cout << "Enter password (not required for CUCM) [ Enter ]:" ;
	getline(cin, input, '\n');
	if(input.length() > 0){
		_config->SetUserPassword(input);
	}
	
	cout << "Enter device name (only required for CUCM): [current value = " << _config->GetDeviceName() << "] " ;
	getline(cin, input, '\n');
	if(input.length() > 0){
		_config->SetDeviceName(input);
	}
	
	cout << "Use video camera? [yes|no]: [current value = " << (_config->UseVideo() ? "yes" : "no") << "] " ; 
	getline(cin, input, '\n');
	if(input.length() > 0){
		transform(input.begin(), input.end(), input.begin(),::tolower );
		_config->SetUseVideo(input == "yes");
	}
}

/**
 * Loads configuration settings from a user specified file path
 */
void TestMain::LoadConfigurationFromFile(){
	string input;
	cout << "Configuration file should have the following entries (one per line" << endl;
	cout << "username=<some user name>" << endl;
	cout << "password=<some password>" << endl;
	cout << "sipaddress=<ip address to sip proxy>" << endl;
	cout << "devicename=<name of device>" << endl;
	cout << "audioonly=<true or false>" << endl;
	cout << endl;
	cout << "Enter path to configuration file:" << endl;
	
	//read in the user input
	getline(cin, input, '\n');
	if(input.length() > 0){
		//read in the configuration
		if(_config->ReadConfigFromFile(input)){
			cout << "configuration read from " << input << endl;
		}
		else{
			cout << "ERROR: failed to read configuration from file " << input << endl;
		}
			
	}
}


bool TestMain::BeginOSIndependentTesting(){
	
    // Create platform dependent render windows
    windowManager = new WindowManager();
	
   	std::string dn;
   	std::string input;
    int testType = 0;
    do{
		cout << "********************************" << endl;        
        cout << "Please Choose One : " << endl;
		cout << "\t 0. Quit" << endl;
		cout << "\t 1. Register" << endl;
		cout << "\t 2. Place Call" << endl;
        cout << "\t 3. Answer Call" << endl;
		cout << "\t 4. End Call" << endl;
		cout << "\t 5. Start P2P mode" << endl;
		cout << "\t 6. Place P2P Call" << endl;
		cout << "\t 7. Manual configuration" << endl;
		cout << "\t 8. Load configuration from file" << endl;
		cout << "\t 9. Print current configuration" << endl;
		
		cout << " What Action do you want to perform Monsieur ?? " << endl;
		scanf("%d", &testType);
		getchar();
		cout << "********************************" << endl;        
        if (testType < 0 || testType > 9){
			continue;
        }
	
	     switch (testType){
				//TODO: replace literals with an enum or #defines
            case 0:
				SipccController::GetInstance()->UnRegister();
				break;
            case 1:
				SipccController::GetInstance()->Register(_config->GetDeviceName(), _config->GetUserName(), _config->GetUserPassword(), _config->GetSIPProxyAddress());
			    break;
            case 2:
				if (_config->UseVideo()) {
					cout << "setting up for video call";
					SipccController::GetInstance()->SetVideoWindow(windowManager->GetVideoWindow());
				}
				cout << "Enter the number to dial, signore " ;
				getline(cin, dn, '\n');
				SipccController::GetInstance()->PlaceCall(dn);
				inCall = true;
				break;
            case 3:
    			SipccController::GetInstance()->AnswerCall();
				break;

            case 4:
               	SipccController::GetInstance()->EndCall(); 
				inCall = false;
                break;

            case 5:
            	cout << "Enter local peer username (usually a phone DN)[current value = " << _config->GetUserName() << "] " ;
            	getline(cin, input, '\n');
            	if((input.length() > 0 )&& (input != _config->GetUserName()))
            	{
            		_config->SetUserName(input);
            	}
            	SipccController::GetInstance()->StartP2PMode(_config->GetUserName());
            	break;

            case 6:
				 if (_config->UseVideo()) {
					 cout << "setting up for video P2P call";
					 SipccController::GetInstance()->SetVideoWindow(windowManager->GetVideoWindow());
				 }	
				cout << "Enter the number to dail, signore: " ;
            	getline(cin, dn, '\n');
            	cout << "Enter the IP Address to dial, signore: " ;
            	getline(cin, input, '\n');
            	if(input.length() > 0)
            	{
            		p2pIPAddress = input;
            	}
            	SipccController::GetInstance()->PlaceP2PCall(dn, p2pIPAddress);
				break;
				
			case 7:
				 ManualConfiguration();
				 break;
				 
			case 8:
				 LoadConfigurationFromFile();
				 break;
			
			case 9:
				cout << _config->toString();
				break;
			
			default:
                break;
        }
    } while (testType != 0);

   // windowManager->TerminateWindows();

   
	printf("Press enter to quit...");
    delete windowManager;

    return true;
}

