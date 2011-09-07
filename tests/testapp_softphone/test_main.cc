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

TestMain::TestMain(): sipProxy("10.99.10.75"),
					  userName("1000"),
					  userPassword(""),
					  deviceName("emannionsip01") 
{
}

void TestMain::FillInUserData()
{
	std::string input;
	cout << "Enter SIP Server IP Address [ "<< sipProxy <<" ]: " ;
	getline(cin, input, '\n');
	if(input.length() > 0)
	{
		sipProxy = input;
	}
	cout << "Enter SIP Server username (phone DN for CUCM) [ " << userName << " ]: " ;
	getline(cin, input, '\n');
	if(input.length() > 0)
	{
		userName = input;
	}
	cout << "Enter Password (not required for CUCM [ Enter ]:" ;
	getline(cin, input, '\n');
	if(input.length() > 0)
	{
		userPassword = input;
	}
	cout << "Enter device name (only required for CUCM) [ " << deviceName << " ]: " ;
	getline(cin, input, '\n');
	if(input.length() > 0)
	{
		deviceName = input;
	}
}

bool TestMain::BeginOSIndependentTesting()
{
    // Create platform dependent render windows
    windowManager = new WindowManager();
	char window1Title[1024] = "Not Supported Preview Window";
    char window2Title[1024] = "Remote Party Video Window";

    windowManager->CreateWindows( window1Title,
                                 window2Title);
    windowManager->SetTopmostWindow();
	
   	std::string dn; 
    int testType = 0;
    int testErrors = 0;
    do
    {
		cout << "********************************" << endl;        
        cout << "Please Choose One : " << endl;
		cout << "\t 0. Quit" << endl;
		cout << "\t 1. Register User" << endl;
		cout << "\t 2. Place Call" << endl;
        cout << "\t 3. Answer Call" << endl;
		cout << "\t 4. End Call" << endl;
		
		cout << " What Action do you want to perform Monsieur ?? " << endl;
		scanf("%d", &testType);
		getchar();
		cout << "********************************" << endl;        
        if (testType < 0 || testType > 4)
        {
			continue;
        }

        switch (testType)
        {
            case 0:
				SipccController::GetInstance()->UnRegister();
				break;
            case 1:
				FillInUserData();
				SipccController::GetInstance()->Register(deviceName, userName, userPassword, sipProxy);
				SipccController::GetInstance()->SetVideoWindow(windowManager->GetWindow2());
                break;

            case 2:
				SipccController::GetInstance()->SetVideoWindow(windowManager->GetWindow2());
				cout << "Enter the number to dail, signore " ;
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

			default:
                break;
        }
    } while (testType != 0);

    windowManager->TerminateWindows();

   
	printf("Press enter to quit...");
    delete windowManager;

    return true;
}

