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

#include "InputHandler.h"

#include "csf_common.h"
#include "CSFLog.h"
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

static const char* logTag = "InputHandler";

static cc_sdp_direction_t _sVideoPref = CC_SDP_DIRECTION_INACTIVE;

using namespace std;


#ifndef WIN32
static int csf_getch(void)
{
    char buf = 0;
    struct termios old;
   
    memset(&old, 0, sizeof(termios));

    if (tcgetattr(0, &old) < 0)
    {
        CSFLogDebugS(logTag, "tcgetattr() failed.\n");
        return 0;
    }

    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSANOW, &old) < 0)
    {
        CSFLogDebugS(logTag, "tcsetattr ICANON.\n");
        return 0;
    }

    if (read(0, &buf, 1) < 0)
    {
        CSFLogDebugS(logTag, "read()");
    }

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;

    if (tcsetattr(0, TCSADRAIN, &old) < 0)
    {
        perror ("tcsetattr ~ICANON");
    }
    return (buf);
}

static int csf_kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

#else
static char csf_getch(void)
{
    return _getch();
}
static int csf_kbhit(void)
{
    return _kbhit();
}
#endif


UserOperationRequestData::~UserOperationRequestData()
{
    if (m_pData == NULL)
    {
        return;
    }

    switch (request)
    {
    case eOriginatePhoneCall:
        delete m_pPhoneNumberToCall;
        break;
    case eOriginateP2PPhoneCall:
        delete m_pPhoneNumberToCall;
        break;
    case eSendDTMFDigitOnFirstCallWithDTMFCaps:
        delete m_pDTMFDigit;
        break;
    default:
    	break;
    }
}

void InputHandler::createUserOperationAndSignal (eUserOperationRequest request, void * pData)
{
    UserOperationRequestDataPtr pOperation(new UserOperationRequestData(request, pData));
    if(callback != NULL)
    {
    	callback->onUserRequest(pOperation);
    }
}

std::string InputHandler::UserInputWorkItem::getInput(const std::string& prompt)
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
void InputHandler::UserInputWorkItem::signalSendDTMF (int asciiChar)
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

void InputHandler::UserInputWorkItem::Run()
{
    int ch = 0;

    clearBufferOfTypeAhead();

    do
    {
        ch = csf_getch();//Blocks here until user presses a key

        cc_digit_t dtmf;
        if (convertASCIIToDTMFDigit(ch, dtmf))
        {
            signalSendDTMF(ch);
            continue;
        }

        switch (ch)
        {
        case 'd':
            context->createUserOperationAndSignal(eOriginatePhoneCall, new string(getInput("Enter DN# to call: ")));
            break;
        case 'p':
            context->createUserOperationAndSignal(eOriginateP2PPhoneCall, new string(getInput("Enter DN# to call: ")));
            break;
        case 'a':
            context->createUserOperationAndSignal(eAnswerCall);
            break;
        case 'e':
            context->createUserOperationAndSignal(eEndFirstCallWithEndCallCaps);
            break;
        case 'h':
            context->createUserOperationAndSignal(eHoldFirstCallWithHoldCaps);
            break;
        case 'r':
            context->createUserOperationAndSignal(eResumeFirstCallWithResumeCaps);
            break;
        case 'm':
            context->createUserOperationAndSignal(eMuteAudioForConnectedCall);
            break;
        case 'n':
            context->createUserOperationAndSignal(eUnmuteAudioForConnectedCall);
            break;
        case 'k':
            context->createUserOperationAndSignal(eMuteVideoForConnectedCall);
            break;
        case 'j':
            context->createUserOperationAndSignal(eUnmuteVideoForConnectedCall);
            break;
        case 'v':
            context->createUserOperationAndSignal(eCycleThroughVideoPrefOptions);
            break;
        case 'l':
            context->createUserOperationAndSignal(ePrintActiveCalls);
            break;
        case '+':
            context->createUserOperationAndSignal(eVolumeUp);
            break;
        case '-':
            context->createUserOperationAndSignal(eVolumeDown);
            break;
        case 't':
            context->createUserOperationAndSignal(eToggleAutoAnswer);
            break;
        case 'z':
            context->createUserOperationAndSignal(eToggleShowVideoAutomatically);
            break;
        case 'x':
            context->createUserOperationAndSignal(eAddVideoToConnectedCall);
            break;
        case 'y':
            context->createUserOperationAndSignal(eRemoveVideoFromConnectedCall);
            break;
        case 'w':
            context->createUserOperationAndSignal(eDestroyAndCreateWindow);
            break;
        case '?':
            CSFLogDebugS(logTag, "\nl = print list of all calls");
            CSFLogDebugS(logTag, "\nd = dial");
            CSFLogDebugS(logTag, "\na = answer (incoming call)");
            CSFLogDebug(logTag, "\nv = cycle to next video pref option (current=%s)", getUserFriendlyNameForVideoPref(getActiveVideoPref()));
            CSFLogDebugS(logTag, "\ne = end call");
            CSFLogDebugS(logTag, "\nh = hold");
            CSFLogDebugS(logTag, "\nr = resume");
            CSFLogDebugS(logTag, "\nm = audio mute");
            CSFLogDebugS(logTag, "\nn = audio unmute");
            CSFLogDebugS(logTag, "\nk = video mute");
            CSFLogDebugS(logTag, "\nj = video unmute");
            CSFLogDebugS(logTag, "\n+ = volume up");
            CSFLogDebugS(logTag, "\n- = volume down");
#ifndef NOVIDEO
            CSFLogDebugS(logTag, "\nx = Add video to connected call (escalate)");
            CSFLogDebugS(logTag, "\ny = Remove video from connected call (de-escalate)");
            CSFLogDebugS(logTag, "\nz = toggle show-video-automatically");
#endif
            CSFLogDebugS(logTag, "\nt = toggle auto-answer");
            CSFLogDebugS(logTag, "\nw = destroy video window and create a new one");
            CSFLogDebugS(logTag, "\n0123456789#*ABCD = send digit");
            CSFLogDebugS(logTag, "\nq = quit");
            CSFLogDebugS(logTag, "\n");
            break;
        }
    } while (ch != 'q');

    context->createUserOperationAndSignal(eQuit);
}

InputHandler::UserInputWorkItem::~UserInputWorkItem()
{
	context->thread.Join();
}

InputHandler::InputHandler()
: thread(this), callback(NULL)
{
	thread.Start();
}

InputHandler::~InputHandler()
{
}

void InputHandler::setCallback(UserOperationCallback* callback)
{
	this->callback = callback;
}


void getPasswordFromConsole (const char * pPromptText, string & password)
{
    password.clear();

    if (pPromptText == NULL)
    {
        return;
    }

    cout << pPromptText << flush;

    int ch = csf_getch();

#ifdef WIN32
const int ENTER_KEY = 13;
const int BS_KEY = 8;
#else
const int ENTER_KEY = '\n';
const int BS_KEY = 127;
#endif

    while(ch != ENTER_KEY)
    {
        if (ch == BS_KEY) // backspace
        {
            if (password.length() > 0)
            {
                cout <<"\b \b" << flush;  // print a space over the '*' that was there
                password.erase(password.length() - 1);
            }
        }
        else
        {
            password += (char)ch;
            cout << '*' << flush;
        }
        ch = csf_getch();
    }
}

cc_sdp_direction_t getActiveVideoPref ()
{
    return _sVideoPref;
}

void cycleToNextVideoPref()
{
    int nextVideoPrefAsInt = (((int) _sVideoPref) + 1);
    _sVideoPref = (cc_sdp_direction_t) nextVideoPrefAsInt;

    if (_sVideoPref == CC_SDP_MAX_QOS_DIRECTIONS)
    {
        assert(((int) CC_SDP_DIRECTION_INACTIVE) == 0);
        _sVideoPref = CC_SDP_DIRECTION_INACTIVE;
    }
}

const char * getUserFriendlyNameForVideoPref (cc_sdp_direction_t videoPref)
{
    const char * pMediaTypeStr = (videoPref == CC_SDP_DIRECTION_INACTIVE) ? "audio" :
                                 (videoPref == CC_SDP_DIRECTION_SENDONLY) ? "video out" :
                                 (videoPref == CC_SDP_DIRECTION_RECVONLY) ? "video in" :
                                 (videoPref == CC_SDP_DIRECTION_SENDRECV) ? "video in/out" : "unknown";

    return pMediaTypeStr;
}


