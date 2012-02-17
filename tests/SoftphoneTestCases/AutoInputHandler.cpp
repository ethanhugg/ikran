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

#include "AutoInputHandler.h"
#include "Common.h"

#include "csf_common.h"
#include "CSFLogStream.h"
#include <iostream>
#ifndef WIN32
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <windows.h>
#include <conio.h>
#include <process.h>
#endif

//static const char* logTag = "AutoInputHandler";


using namespace std;

extern string calluser;


void AutoInputHandler::createUserOperationAndSignal (eUserOperationRequest request, void * pData)
{
	UserOperationRequestDataPtr pOperation(new UserOperationRequestData(request, pData));
	if(callback != NULL)
	{
		callback->onUserRequest(pOperation);
	}
}

std::string AutoInputHandler::UserInputWorkItem::getInput(const std::string& prompt)
{
	string phoneNumberToCall;
	cout << endl << prompt;
	cin >> phoneNumberToCall;
	return phoneNumberToCall;
}

#define SINGLEQUOTE(x) ((#x)[0])

#define NAME2(fun,suffix) fun ## _ ## suffix

#define TRANSFORM_ASCII_TO_DIGIT1(prefix, ascii)\
	NAME2(fun,suffix)

#define TRANSFORM_ASCII_TO_DIGIT(asciiValue)\
	TRANSFORM_ASCII_TO_DIGIT1(KEY_, asciiValue)



#define CREATE_TRANSFORM_ENTRY(digit)\
	transformArray[SINGLEQUOTE(digit)] = KEY_##digit;


static bool convertASCIIToDTMFDigit (int asciiChar, cc_digit_t & dtmfDigit)
{
	//0,         1,         2,         3,         4,         5,         6,         7,         8,         9
	static int transformArray['D'+1] = { 0,         0,         0,         0,         0,         0,         0,         0,         0,         0,     //9
		0,         0,         0,         0,         0,         0,         0,         0,         0,         0,     //19
		0,         0,         0,         0,         0,         0,         0,         0,         0,         0,     //29
		0,         0,         0,         0,         0,         KEY_POUND, 0,         0,         0,         0,     //39
		0,         0,         KEY_STAR,  KEY_PLUS,  0,         0,         0,         0,         KEY_0,     KEY_1, //49
		KEY_2,     KEY_3,     KEY_4,     KEY_5,     KEY_6,     KEY_7,     KEY_8,     KEY_9,     0,         0,     //59
		0,         0,         0,         0,         0,         KEY_A,     KEY_B,     KEY_C,     KEY_D  //68
	}; //Need array big enough to hold the largest ASCII value (which is 'D'=68 dec)

	if (((asciiChar >= '0') && (asciiChar <= '9')) ||
		(asciiChar == '#') || (asciiChar == '*') ||
		((asciiChar >= 'A') && (asciiChar <= 'D')))
	{
		dtmfDigit = (cc_digit_t) transformArray[asciiChar];
		return true;
	}

	return false;
}


/* Signals to the main thread to send the specified DTMF character on the first active call it finds.
By "active" I mean the call is established and not on hold. */
void AutoInputHandler::UserInputWorkItem::signalSendDTMF (int asciiChar)
{
	cc_digit_t dtmfDigit = KEY_1;
	if (convertASCIIToDTMFDigit(asciiChar, dtmfDigit))
	{
		context->createUserOperationAndSignal(eSendDTMFDigitOnFirstCallWithDTMFCaps, new cc_digit_t(dtmfDigit));
	}
}

static void clearBufferOfTypeAhead ()
{
	while (csf_kbhit())//doesn't block, returns 0 if no char in buffer
	{
		csf_getch();
	}
}

void AutoInputHandler::UserInputWorkItem::Run()
{
	eUserOperationRequest ch=eLoop;

	clearBufferOfTypeAhead();

	do
	{
		if(context->GetActionVector()->size() ==0 )
			continue;
		ch = context->GetActionVector()->front();//Blocks here until user presses a key

		cc_digit_t dtmf;
		if (convertASCIIToDTMFDigit(ch, dtmf))
		{
			signalSendDTMF(ch);
			continue;
		}

		switch (ch)
		{
		case eQuit:
			context->createUserOperationAndSignal(eQuit);
			break;
		case eIdle:
			base::PlatformThread::Sleep(20000);
			break;
		case eWait:
			if(_userOperationResponseEvent.TimedWait(base::TimeDelta::FromMilliseconds(40000))){
				context->createUserOperationAndSignal(eWait);
			}
			break;
		case eOriginatePhoneCall:
			context->createUserOperationAndSignal(eOriginatePhoneCall, new string(calluser.c_str()));
			break;
		case eAnswerIdle:
			if(_userOperationResponseEvent.TimedWait(base::TimeDelta::FromMilliseconds(40000))){
				context->createUserOperationAndSignal(eAnswerIdle);
			}
			break;

		case sOriginatePhoneCall:

			if(_userOperationResponseEvent.TimedWait(base::TimeDelta::FromMilliseconds(40000))){
				context->createUserOperationAndSignal(sOriginatePhoneCall);
				

			}
			break;
		case eOriginateP2PPhoneCall:
			context->createUserOperationAndSignal(eOriginateP2PPhoneCall, new string(getInput("Enter DN# to call P2P: ")));
			break;
		case eAnswerCall:
			context->createUserOperationAndSignal(eAnswerCall);
			break;
		case eEndFirstCallWithEndCallCaps:
			context->createUserOperationAndSignal(eEndFirstCallWithEndCallCaps);
			break;
		case sEndFirstCallWithEndCallCaps:
			if(_userOperationResponseEvent.TimedWait(base::TimeDelta::FromMilliseconds(40000))){
				context->createUserOperationAndSignal(sEndFirstCallWithEndCallCaps);
			}
			break;
		case eHoldFirstCallWithHoldCaps:
			context->createUserOperationAndSignal(eHoldFirstCallWithHoldCaps);
			break;
		case eResumeFirstCallWithResumeCaps:
			context->createUserOperationAndSignal(eResumeFirstCallWithResumeCaps);
			break;
		case eMuteAudioForConnectedCall:
			context->createUserOperationAndSignal(eMuteAudioForConnectedCall);
			break;
		case eUnmuteAudioForConnectedCall:
			context->createUserOperationAndSignal(eUnmuteAudioForConnectedCall);
			break;
		case eMuteVideoForConnectedCall:
			context->createUserOperationAndSignal(eMuteVideoForConnectedCall);
			break;
		case eUnmuteVideoForConnectedCall:
			context->createUserOperationAndSignal(eUnmuteVideoForConnectedCall);
			break;
		case eCycleThroughVideoPrefOptions:
			context->createUserOperationAndSignal(eCycleThroughVideoPrefOptions);
			break;
		case ePrintActiveCalls:
			context->createUserOperationAndSignal(ePrintActiveCalls);
			break;
		case eVolumeUp:
			context->createUserOperationAndSignal(eVolumeUp);
			break;
		case eVolumeDown:
			context->createUserOperationAndSignal(eVolumeDown);
			break;
		case eToggleAutoAnswer:
			context->createUserOperationAndSignal(eToggleAutoAnswer);
			break;
		case eToggleShowVideoAutomatically:
			context->createUserOperationAndSignal(eToggleShowVideoAutomatically);
			break;
		case eAddVideoToConnectedCall:
			context->createUserOperationAndSignal(eAddVideoToConnectedCall);
			break;
		case eRemoveVideoFromConnectedCall:
			context->createUserOperationAndSignal(eRemoveVideoFromConnectedCall);
			break;
		case eDestroyAndCreateWindow:
			context->createUserOperationAndSignal(eDestroyAndCreateWindow);
			break;
		default:
			break;

		}
		if(context->GetActionVector()->size() !=0 )
			context->GetActionVector()->erase(context->GetActionVector()->begin());
		base::PlatformThread::Sleep(10000);

	} while (ch != eExit);

	context->createUserOperationAndSignal(eQuit);
}

AutoInputHandler::UserInputWorkItem::~UserInputWorkItem()
{
	if(context->thread.HasBeenStarted())
	{
		vector<eUserOperationRequest>* v = context->GetActionVector();
		v->clear();
		v->push_back(eExit);
		context->thread.Join();
	}
}

AutoInputHandler::AutoInputHandler()
	: thread(this), callback(NULL)
{
	//thread.Start();
}

AutoInputHandler::~AutoInputHandler()
{
}

void AutoInputHandler::setCallback(UserOperationCallback* callback)
{
	this->callback = callback;
}

