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

#include <sys/timeb.h>

#include "csf_common.h"
#include "CSFLog.h"
#include "debug-psipcc-types.h"
#include "base/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/lock.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

#ifndef WIN32
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#endif

#include "CallControlManager.h"
#include "CC_Device.h"
#include "CC_DeviceInfo.h"
#include "CC_CallServerInfo.h"
#include "CC_Call.h"
#include "CC_CallInfo.h"
#include "CC_Line.h"
#include "CC_LineInfo.h"
#include "ECC_Types.h"
#include "CC_CallTypes.h"
#include "PhoneDetails.h"
#include "CSFAudioControl.h"

#ifndef NOVIDEO
#include "VideoWindow.h"
#endif

#include "EventPrinter.h"
#include "InputHandler.h"

using namespace std;
using namespace CSF;

#ifndef WIN32
typedef int SOCKET;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
#endif

static CC_CallPtr getFirstCallInGivenState (CallControlManagerPtr ccmPtr, cc_call_state_t callState);
static CC_CallPtr getFirstCallInGivenStates (CallControlManagerPtr ccmPtr, const vector<cc_call_state_t>& callStates);

/*
In general rather than setting the params here, you should use a config file. Pass in the name of the config
file to executable as a command line argument. If no command line arg is given then a file called "cucm-credentials.txt"
is looked for in the directory this app is running, and config data is taken from there.

Configuration file format is quite simple. File has one assignment on each line for example

////////
IP_ADDRESS=10.53.60.140
DEVICENAME=emannionsipdevice
LOGFILE=local.log
SIPUSER=enda
CREDENTIALS=1234
////////

DEVICENAME only required for CUCM
CREDENTIALS only required for ASrerisk
*/


#ifndef NOVIDEO
VideoWindow vWnd;
#ifdef WIN32
HWND hVideoWindow;
#define WM_WINDOWCLOSE (WM_USER + 101)
#else
Window hVideoWindow;
#endif
#endif

const char* logTag = "TestApp";
int CALL_ANSWER_DELAY_TIME_MS = 2000;
static CallControlManagerPtr ccmPtr;
static base::WaitableEvent _callCapsMayHaveChanged(true,false);
base::Lock _userOpRequestMutex;

static vector<UserOperationRequestDataPtr> _userOperationRequestVector;
static base::WaitableEvent _userOperationRequestEvent(true,false);
static base::WaitableEvent _WindowThreadEvent(true,false);
bool endWindowWorkItem = true;
bool g_bAutoAnswer = false;
bool showVideoAutomatically = false;

class MyPhoneListener : public CC_Observer
{
public:
    virtual void onDeviceEvent         (ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_DeviceInfoPtr info );
    virtual void onFeatureEvent        (ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_FeatureInfoPtr feature_info);
    virtual void onLineEvent           (ccapi_line_event_e lineEvent,     CC_LinePtr line,     CC_LineInfoPtr info );
    virtual void onCallEvent           (ccapi_call_event_e callEvent,     CC_CallPtr call,     CC_CallInfoPtr info );
};

void MyPhoneListener::onDeviceEvent ( ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_DeviceInfoPtr info )
{
}

void MyPhoneListener::onFeatureEvent (ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_FeatureInfoPtr info)
{
}

void MyPhoneListener::onLineEvent   ( ccapi_line_event_e lineEvent,     CC_LinePtr line,     CC_LineInfoPtr info )
{
}

/*
    The following set of data items work together to communicate data from one thread to another.
    I create a new UserOperationRequestData object and fill in the data fields that relate to
    the event that I want to communicate to another thread. I then lock the vector and push
    that new object into vector, and when this completes I unlock access to the vector.
    I then signal that there is data in the vector.

    When the consuming thread sees that the _userOperationRequestEvent has fired it locks
    the vector and iterates over the contents (UserOperationRequestData objects). When all
    requests have been processed it then empties the vector and unlocks access to the vector.
    The UserOperationRequestData objects need to be deleted by the consumer as well as removed
    from the vector as they were allocated on the heap when pushed into the vector.
    
    Note also that the data member "m_pData" is also explicitly deleted before being removed from
    the vector. m_pData is a void* pointer and can be any of the types of object shared in the
    union contained in the definition of UserOperationRequestData. Depending on the
    eUserOperationRequest value m_pData will either be NULL or pointer to an object of a type
    actually declared in the other members of the union. For example, in the case of
    eUserOperationRequest = eOriginatePhoneCall, there will be a non-NULL value for m_pData, which
    is should be referred to via the data member m_pPhoneNumberToCall, which is a pointer to a string.
    When m_pData is deleted the string is deleted.
*/


static void addUserOperationAndSignal (UserOperationRequestDataPtr pOperation)
{
    base::AutoLock lock(_userOpRequestMutex);
    _userOperationRequestVector.push_back(pOperation);
    _userOperationRequestEvent.Signal();
}

class MyUserOperationCallback: public UserOperationCallback
{
public:
	void onUserRequest(UserOperationRequestDataPtr data)
	{
		addUserOperationAndSignal(data);
	}
};

void MyPhoneListener::onCallEvent   ( ccapi_call_event_e callEvent, CC_CallPtr call, CC_CallInfoPtr info )
{
	CSFLogDebugS(logTag, "MyPhoneListener::onCallEvent");

	_callCapsMayHaveChanged.Signal();

    if (callEvent == CCAPI_CALL_EV_STATE)
    {
        if (info->getCallState() == RINGIN)
        {
            addUserOperationAndSignal(UserOperationRequestDataPtr(new UserOperationRequestData(eIncomingCallReceived, NULL)));
        }
        else if (info->getCallState() == ONHOOK)
        {
        	CSFLogDebugS(logTag, "MyPhoneListener::onCallEvent -- ONHOOK");
        }
    }

	else if(callEvent == CCAPI_CALL_EV_VIDEO_AVAIL)
	{
        addUserOperationAndSignal(UserOperationRequestDataPtr(new UserOperationRequestData(eVideoAvailableChange, NULL)));
	}
	else if(callEvent == CCAPI_CALL_EV_VIDEO_OFFERED)
	{
        addUserOperationAndSignal(UserOperationRequestDataPtr(new UserOperationRequestData(eVideoOffered, NULL)));
	}
}

#ifndef NOVIDEO

class VideoWindowWorkItem : public base::SimpleThread
{
public:
	VideoWindowWorkItem () : base::SimpleThread("VideoWindowWorkItem") {};
    virtual void Run ();
};

DECLARE_PTR(VideoWindowWorkItem);

void VideoWindowWorkItem::Run ()
{
    VideoWindow vNewWnd;
    vWnd = vNewWnd;
    vWnd.SetWindowStyle(WT_FRAME|WT_NORMAL/*|WT_NOWINPROC*/);
    vWnd.InitWindow();
    
#ifdef WIN32
    hVideoWindow = (HWND)vWnd.GetRenderingWindowHandle();
#else
    hVideoWindow = (Window)vWnd.GetRenderingWindowHandle();
#endif
    CSFLogDebug(logTag, "new window handle: %d", (int) hVideoWindow);

    // if there is a current call, make sure it gets the window handle as soon as possible
	if (ccmPtr != NULL)
	{    
	    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, CONNECTED);
	    if (call != NULL)
	    {
	    	CSFLogDebug(logTag, "setting rem window on current call: %d\n", (int) hVideoWindow);
	    	call->setRemoteWindow((VideoWindowHandle)hVideoWindow);
	    }
	}
    
#ifdef WIN32
    MSG Msg;
    while(GetMessage(&Msg, hVideoWindow, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);

        switch (Msg.message)
        {
            case WM_WINDOWCLOSE:
                vWnd.CloseWindow();
                return;
            case WM_DESTROY:
                vWnd.CloseWindow();
                return;
        }
        if (endWindowWorkItem == true)
            break;
    }
#else

    while(true)
    {
    	CSFLogDebugS(logTag, "_WindowThreadEvent.TimedWait(-1);");
        _WindowThreadEvent.TimedWait(base::TimeDelta::FromMilliseconds(-1));
        CSFLogDebugS(logTag, "return from _WindowThreadEvent.await(-1);");

        if (endWindowWorkItem == true)
            break;
    }
#endif

	endWindowWorkItem = false; // so we do not immediately end the next one	
	CSFLogDebugS(logTag, "VideoWindowWorkItem exiting");
}
VideoWindowWorkItem videoThread;

#endif


/**
  This function gets a snapshot of all calls and examines each one in turn. If it find a call that has the
  specified capability then it returns this call.

  @param [in] ccmPtr - 

  @param [in] cap - This is the capability being examined. All existing calls are examined and the first (in
                    no particular order) call that has this capability is returned, if there is one.
                    
  @return Returns a NULL_PTR if there are no calls with the given capability. If a call is found with the given
          capability then that CC_CallPtr is returned.
*/
static CC_CallPtr getFirstCallWithCapability (CallControlManagerPtr ccmPtr, CC_CallCapabilityEnum::CC_CallCapability cap)
{
    if (ccmPtr == NULL)
    {
        return NULL_PTR(CC_Call);
    }

    CC_DevicePtr devicePtr = ccmPtr->getActiveDevice();

    if (devicePtr == NULL)
    {
        return NULL_PTR(CC_Call);
    }

    CC_DeviceInfoPtr deviceInfoPtr = devicePtr->getDeviceInfo();

    if (deviceInfoPtr == NULL)
    {
        return NULL_PTR(CC_Call);
    }

    vector<CC_CallPtr> calls = deviceInfoPtr->getCalls();

    for (vector<CC_CallPtr>::iterator it = calls.begin(); it != calls.end(); it++)
    {
        CC_CallPtr call = *it;

        if (call == NULL)
        {
            continue;
        }

        CC_CallInfoPtr callInfoPtr = call->getCallInfo();

        if (callInfoPtr == NULL)
        {
            continue;
        }

        if (callInfoPtr->hasCapability(cap))
        {
            return call;
        }
    }//end for (calls)

    return NULL_PTR(CC_Call);
}



/**
  This function gets a snapshot of all calls and examines each one in turn. If it find a call whose
  state is one of the specified states then it returns that call.

  @param [in] ccmPtr -

  @param [in] callStates - A vector of the call states being checked for. All existing calls are examined and the
                          first (in no particular order) call that is in one of those states is returned,
                          if there is one.

  @return Returns a NULL_PTR if there are no calls in the given states. If a call is found in the given states
          then that CC_CallPtr is returned.
*/
static CC_CallPtr getFirstCallInGivenStates (CallControlManagerPtr ccmPtr, const vector<cc_call_state_t>& callStates)
{
	CSFLogDebugS(logTag, "getFirstCallInGivenStates()");

	if (ccmPtr == NULL)
    {
        return NULL_PTR(CC_Call);
    }

    CC_DevicePtr devicePtr = ccmPtr->getActiveDevice();

    if (devicePtr == NULL)
    {
    	CSFLogDebugS(logTag, "devicePtr was NULL, returning");
        return NULL_PTR(CC_Call);
    }

    CC_DeviceInfoPtr deviceInfoPtr = devicePtr->getDeviceInfo();

    if (deviceInfoPtr == NULL)
    {
    	CSFLogDebugS(logTag, "deviceInfoPtr was NULL, returning");
        return NULL_PTR(CC_Call);
    }

    vector<CC_CallPtr> calls = deviceInfoPtr->getCalls();

    for (vector<CC_CallPtr>::iterator it = calls.begin(); it != calls.end(); it++)
    {
        CC_CallPtr call = *it;

        if (call == NULL)
        {
            continue;
        }

        CC_CallInfoPtr callInfoPtr = call->getCallInfo();

        if (callInfoPtr == NULL)
        {
            continue;
        }

        CSFLogDebugS(logTag, "about to check call for states");

        for (vector<cc_call_state_t>::const_iterator it = callStates.begin(); it != callStates.end(); it++)
        {
          	cc_call_state_t callState = *it;
            if (callInfoPtr->getCallState() == callState)
            {
                return call;
            }
        }
    }//end for (calls)

    return NULL_PTR(CC_Call);
}

/**
  This function gets a snapshot of all calls and examines each one in turn. If it find a call that is in
  the specified state then it returns this call.

  @param [in] ccmPtr - 

  @param [in] callState - This is the call state being checked for. All existing calls are examined and the
                          first (in no particular order) call that is in this state is returned, if there is one.
                    
  @return Returns a NULL_PTR if there are no calls in the given state. If a call is found in the given state
          then that CC_CallPtr is returned.
*/
static CC_CallPtr getFirstCallInGivenState (CallControlManagerPtr ccmPtr, cc_call_state_t callState)
{
    if (ccmPtr == NULL)
    {
        return NULL_PTR(CC_Call);
    }

    CC_DevicePtr devicePtr = ccmPtr->getActiveDevice();

    if (devicePtr == NULL)
    {
        return NULL_PTR(CC_Call);
    }

    CC_DeviceInfoPtr deviceInfoPtr = devicePtr->getDeviceInfo();

    if (deviceInfoPtr == NULL)
    {
        return NULL_PTR(CC_Call);
    }

    vector<CC_CallPtr> calls = deviceInfoPtr->getCalls();

    for (vector<CC_CallPtr>::iterator it = calls.begin(); it != calls.end(); it++)
    {
        CC_CallPtr call = *it;

        if (call == NULL)
        {
            continue;
        }

        CC_CallInfoPtr callInfoPtr = call->getCallInfo();

        if (callInfoPtr == NULL)
        {
            continue;
        }

        if (callInfoPtr->getCallState() == callState)
        {
            return call;
        }
    }//end for (calls)

    return NULL_PTR(CC_Call);
}

int GetTimeNow()
{
    // Something like GetTickCount but portable
    // It rolls over every ~ 12.1 days (0x100000/24/60/60)
    // Use GetTimeSpan to correct for rollover
    timeb tb;
    ftime( &tb );
    int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
    return nCount;
}

int GetTimeElapsedSince ( int nTimeStart )
{
    int nSpan = GetTimeNow() - nTimeStart;
    if ( nSpan < 0 )
        nSpan += 0x100000 * 1000;
    return nSpan;
}

//Returns 0 if character is NOT a legitimate phone DN digit
static int isPhoneDNDigit (int digit)
{
    static const char * pdnDigits = "0123456789*#+";

    return (strchr(pdnDigits, digit) != NULL);
}

//Returns true if given capability is available on entry to this function, or became
//available within the specified timeframe
bool awaitCallCapability (CC_CallPtr call, CC_CallCapabilityEnum::CC_CallCapability capToWaitFor, int timeoutMilliSeconds)
{
    int nTimeStart = GetTimeNow();
    int remainingTimeout = timeoutMilliSeconds;

    do
    {
        CC_CallInfoPtr info = call->getCallInfo();

        if (info != NULL)
        {
            if (info->hasCapability(capToWaitFor))
            {
                return true;
            }
        }
        else
        {
        	CSFLogDebugS(logTag, "CC_CallPtr->getCallInfo() failed");
            return false;
        }

        int nTimeElapsed = GetTimeElapsedSince( nTimeStart );
        remainingTimeout -= nTimeElapsed;
    } while (_callCapsMayHaveChanged.TimedWait(base::TimeDelta::FromMilliseconds(remainingTimeout)));

    return false;
}

static void handleOriginatePhoneCall (CallControlManagerPtr ccmPtr, const string & phoneNumberToCall)
{
    string digits = phoneNumberToCall;
    digits.erase(remove_if(digits.begin(), digits.end(), std::not1(std::ptr_fun(isPhoneDNDigit))), digits.end());
    CC_DevicePtr devicePtr = ccmPtr->getActiveDevice();

    if (devicePtr != NULL)
    {
        CC_CallPtr outgoingCall = devicePtr->createCall();
        CSFLogDebugS(logTag, "Created call, ");

        cc_sdp_direction_t videoPref = getActiveVideoPref();
        const char * pMediaTypeStr = getUserFriendlyNameForVideoPref(videoPref);


#ifndef NOVIDEO		
		// associate the video window - even if this is not a video call, it might be escalated later, so 
		// it is easier to always associate them
    	outgoingCall->setRemoteWindow((VideoWindowHandle)hVideoWindow);
#endif

    	CSFLogDebug(logTag, " dialing (%s) # %s...", pMediaTypeStr, digits.c_str());

        if (outgoingCall->originateCall(videoPref, digits))
        {
        	CSFLogDebug(logTag, "Dialing (%s) %s...", pMediaTypeStr, digits.c_str());
        }
        else
        {
        	CSFLogDebugS(logTag, "Attempt to originate call failed.");
        }
    }
}

#ifndef NOVIDEO
static void handleCycleThroughVideoPrefOptions (CallControlManagerPtr ccmPtr)
{
    cycleToNextVideoPref();
    CSFLogDebug(logTag, "cc_sdp_direction_t changed to %s...", sdp_direction_getname(getActiveVideoPref()));
}
#endif

static void handlePickUpFirstCallWithAnswerCaps (CallControlManagerPtr ccmPtr)
{
    CC_CallPtr answerableCall = getFirstCallWithCapability(ccmPtr, CC_CallCapabilityEnum::canAnswerCall);

    if (answerableCall != NULL)
    {
    	CSFLogDebugS(logTag, "Answering incoming call...");
        // CC_SDP_DIRECTION_SENDRECV means 2 way video. We could use CC_SDP_DIRECTION_INACTIVE to answer with no video
        // Note that we are attempting to negotiate video on every call, even tough
        // some calls might not have video capability - so maybe we need to check the call first.

        cc_sdp_direction_t videoPref = getActiveVideoPref();

#ifndef NOVIDEO
        videoPref = (showVideoAutomatically) ? videoPref : CC_SDP_DIRECTION_INACTIVE;
#endif
        if (!answerableCall->answerCall(videoPref))
        {
        	CSFLogDebugS(logTag, "Attempt to answer incoming call failed.");
        }

#ifndef NOVIDEO			
    	if(videoPref != CC_SDP_DIRECTION_INACTIVE)
    	{
    		answerableCall->setRemoteWindow((VideoWindowHandle)hVideoWindow);
    	}
#endif
		
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be answered.");
    }
}

static void handleEndFirstCallWithEndCallCaps (CallControlManagerPtr ccmPtr)
{
    CC_CallPtr endableCall = getFirstCallWithCapability(ccmPtr, CC_CallCapabilityEnum::canEndCall);

    if (endableCall != NULL)
    {
    	CSFLogDebugS(logTag, "Hanging up 1st call that can be ended...");
        if (!endableCall->endCall())
        {
        	CSFLogDebugS(logTag, "\nAttempt to end call failed.");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be ended.");
    }
}

static void handleSendDTMFDigitOnFirstCallWithDTMFCaps (CallControlManagerPtr ccmPtr, const cc_digit_t & dtmfDigit)
{
    CC_CallPtr dtmfAbleCall = getFirstCallWithCapability(ccmPtr, CC_CallCapabilityEnum::canSendDigit);

    if (dtmfAbleCall != NULL)
    {
    	CSFLogDebug(logTag, "Sending DTMF digit \"%s\" on first call that allows DTMF.", digit_getname(dtmfDigit));
        if (!dtmfAbleCall->sendDigit(dtmfDigit))
        {
        	CSFLogDebugS(logTag, "Attempt to send DTMF digit failed.");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "Cannot send DTMF digit as there are no calls that allow DTMF.");
    }
}


static void handleHoldFirstCallWithHoldCaps (CallControlManagerPtr ccmPtr)
{
    CC_CallPtr holdableCall = getFirstCallWithCapability(ccmPtr, CC_CallCapabilityEnum::canHold);

    if (holdableCall != NULL)
    {
    	CSFLogDebugS(logTag, "Putting 1st call that can be held on HOLD...");
        if (!holdableCall->hold(CC_HOLD_REASON_NONE))
        {
        	CSFLogDebugS(logTag, "Attempt to HOLD call failed.");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be put on HOLD.");
    }
}

static void handleResumeFirstCallWithResumeCaps (CallControlManagerPtr ccmPtr)
{
    CC_CallPtr resumableCall = getFirstCallWithCapability(ccmPtr, CC_CallCapabilityEnum::canResume);

    if (resumableCall != NULL)
    {
        cc_sdp_direction_t videoPref = getActiveVideoPref();
        CSFLogDebug(logTag, "RESUMING 1st HELD call as a (%s) call...", getUserFriendlyNameForVideoPref(videoPref));
        if (!resumableCall->resume(videoPref))
        {
        	CSFLogDebugS(logTag, "Attempt to RESUME call failed.");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be RESUMED.");
    }
}

static void handleMuteAudioForFirstConnectedCall (CallControlManagerPtr ccmPtr)
{
    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, CONNECTED);

    if (call != NULL)
    {
    	CSFLogDebugS(logTag, "Muting (audio) 1st connected call...");
    	if (!call->muteAudio())
    	{
    		CSFLogDebugS(logTag, "Attempt to mute (audio) failed.\n");
    	}
        CC_CallInfoPtr info = call->getCallInfo();
        if (!info->isAudioMuted())
        {
            // note that when CTI adds mute it might fail here, because the setAudioMute will not be a synchronous call
            // So we might change this check for CTI when that time comes.
        	CSFLogDebugS(logTag, "MUTE FAILED TO BE REFLECTED IN INFO");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be muted (audio).");
    }
}

static void handleUnmuteAudioForFirstConnectedCall (CallControlManagerPtr ccmPtr)
{
    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, CONNECTED);

    if (call != NULL)
    {
    	CSFLogDebugS(logTag, "Unmuting (audio) 1st connected call...");
    	if (!call->unmuteAudio())
    	{
    		CSFLogDebugS(logTag, "Attempt to unmute (audio) failed.");
    	}
        CC_CallInfoPtr info = call->getCallInfo();
        if (info->isAudioMuted())
        {
            // note that when CTI adds mute it might fail here, because the setAudioMute will not be a synchronous call
            // So we might change this check for CTI when that time comes.
        	CSFLogDebugS(logTag, "MUTE FAILED TO BE REFLECTED IN CALL INFO");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be unmuted (audio).");
    }
}

static void handleMuteVideoForFirstConnectedCall (CallControlManagerPtr ccmPtr)
{
    //If there is more than 1 call with a mix of audio/video calls
    //the we may need to look at the cc_sdp_direction_t to figure out
    //which CONNECTED call is actually a video call and is sending video
    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, CONNECTED);

    if (call != NULL)
    {
    	CSFLogDebugS(logTag, "Muting (video) 1st connected call...");
    	if (!call->muteVideo())
    	{
    		CSFLogDebugS(logTag, "Attempt to mute (video) failed.");
    	}
        CC_CallInfoPtr info = call->getCallInfo();
        if ( ! info->isVideoMuted())
        {
        	CSFLogDebugS(logTag, "MUTE FAILED TO BE REFLECTED IN CALL INFO.");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be muted (video).");
    }
}

static void handleUnmuteVideoForFirstConnectedCall (CallControlManagerPtr ccmPtr)
{
    //If there is more than 1 call with a mix of audio/video calls
    //the we may need to look at the cc_sdp_direction_t to figure out
    //which CONNECTED call is actually a video call and is sending video
    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, CONNECTED);

    if (call != NULL)
    {
    	CSFLogDebugS(logTag, "Unmuting (video) 1st connected call...");
    	if (!call->unmuteVideo())
    	{
    		CSFLogDebugS(logTag, "Attempt to unmute (video) failed.");
    	}
        CC_CallInfoPtr info = call->getCallInfo();
        if (info->isVideoMuted())
        {
        	CSFLogDebugS(logTag, "UNMUTE FAILED TO BE REFLECTED IN CALL INFO.");
        }
    }
    else
    {
    	CSFLogDebugS(logTag, "No calls exist that can be unmuted (video).");
    }
}

static void handleAddVideoToConnectedCall( CallControlManagerPtr ccmPtr)
{
#ifndef NOVIDEO
    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, CONNECTED);

    if(call != NULL)
    {
    	if(call->getCallInfo()->getVideoDirection() == CC_SDP_DIRECTION_SENDRECV)
    	{
    		CSFLogDebugS(logTag, "First connected call already has video.");
    	}
    	else
    	{
        	if(call->getCallInfo()->hasCapability(CC_CallCapabilityEnum::canUpdateVideoMediaCap))
        	{
				if(call->updateVideoMediaCap(CC_SDP_DIRECTION_SENDRECV))
				{
					call->setRemoteWindow((VideoWindowHandle)hVideoWindow);
					CSFLogDebugS(logTag, "Video added to first connected call.");
				}
				else
				{
					CSFLogDebugS(logTag, "Failed to add video to first connected call.");
				}
    		}
        	else
        	{
        		CSFLogDebugS(logTag, "Cannot add video to this call - capability not currently available.");
        	}
		}
    }
    else
    {
    	CSFLogDebugS(logTag, "No currently connected calls, cannot add video.");
    }
#endif
}

static void handleRemoveVideoFromConnectedCall( CallControlManagerPtr ccmPtr)
{
    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, CONNECTED);

    if(call != NULL)
    {
		if(call->getCallInfo()->hasCapability(CC_CallCapabilityEnum::canUpdateVideoMediaCap))
		{
			if(call->updateVideoMediaCap(CC_SDP_DIRECTION_INACTIVE))
			{
				CSFLogDebugS(logTag, "Removed video from first connected call.");
			}
			else
			{
				CSFLogDebugS(logTag, "Failed to remove video from first connected call.");
			}
		}
		else
		{
			CSFLogDebugS(logTag, "Cannot add video to this call - capability not currently available.");
		}
    }
    else
    {
    	CSFLogDebugS(logTag, "No currently connected calls, cannot remove video.");
    }
}


// Volume change: If a RINGIN call exists, adjust ringer volume.
//                If CONNECTED exists, adjust current call volume.
//                Otherwise, adjust default call volume.
static void handleAdjustVolume (CallControlManagerPtr ccmPtr, int adjust)
{
    CC_CallPtr call = getFirstCallInGivenState(ccmPtr, RINGIN);
    if (call != NULL)
    {
    	CSFLogDebug(logTag, "Changing ringer volume %d...", adjust);
        int currentVolume = ccmPtr->getAudioControl()->getRingerVolume();
    	if (currentVolume == -1)
    	{
    		CSFLogDebugS(logTag, "Attempt to determine ringer volume failed.");
    	}
    	else
    	{
    		int newVolume = currentVolume + adjust;
    		newVolume = newVolume > 100 ? 100 : newVolume < 0 ? 0 : newVolume;
    		if(!ccmPtr->getAudioControl()->setRingerVolume(newVolume))
    		{
    			CSFLogDebugS(logTag, "Attempt to adjust ringer volume failed.");
    		}
    	}
    	return;
    }

    call = getFirstCallInGivenState(ccmPtr, CONNECTED);
    if (call != NULL)
    {
    	CSFLogDebug(logTag, "Changing call volume %d...", adjust);
        CC_CallInfoPtr info = call->getCallInfo();
        int currentVolume = info->getVolume();
    	if (currentVolume == -1)
    	{
    		CSFLogDebugS(logTag, "Attempt to determine call volume failed.");
    	}
    	else
    	{
    		int newVolume = currentVolume + adjust;
    		newVolume = newVolume > 100 ? 100 : newVolume < 0 ? 0 : newVolume;
    		if(!call->setVolume(newVolume))
    		{
    			CSFLogDebugS(logTag, "Attempt to adjust call volume failed.");
    		}
    	}
    	return;
    }

    CSFLogDebug(logTag, "Changing default call volume %d...", adjust);
	int currentVolume = ccmPtr->getAudioControl()->getDefaultVolume();
	if (currentVolume == -1)
	{
		CSFLogDebugS(logTag, "Attempt to determine default call volume failed.");
	}
	else
	{
		int newVolume = currentVolume + adjust;
		newVolume = newVolume > 100 ? 100 : newVolume < 0 ? 0 : newVolume;
		if(!ccmPtr->getAudioControl()->setDefaultVolume(newVolume))
		{
			CSFLogDebugS(logTag, "Attempt to adjust default call volume failed.");
		}
	}
}

static void handleToggleAutoAnswer (CallControlManagerPtr ccmPtr)
{
	g_bAutoAnswer = !g_bAutoAnswer;
	CSFLogDebug(logTag, "Auto-Answer is %s", g_bAutoAnswer ? "enabled" : "disabled");
}

static void handleToggleShowVideoAutomatically (CallControlManagerPtr ccmPtr)
{
	showVideoAutomatically = !showVideoAutomatically;
	CSFLogDebug(logTag, "Show video automatically is %s", showVideoAutomatically ? "enabled" : "disabled");
}

static void handleVideoAvailableChangeOnConnectedCall(CallControlManagerPtr ccmPtr)
{
	vector<cc_call_state_t> permittedCallStates;
	permittedCallStates.push_back(CONNECTED);
	permittedCallStates.push_back(RINGOUT);
	permittedCallStates.push_back(OFFHOOK);
	CSFLogDebugS(logTag, "About to getFirstCallInGivenStates()");
	CC_CallPtr call = getFirstCallInGivenStates(ccmPtr, permittedCallStates);

    if (call != NULL)
    {
    	if(call->getCallInfo()->getVideoDirection() == CC_SDP_DIRECTION_INACTIVE)
    	{
    	    //A GUI client would hide the video window at this point
    		CSFLogDebugS(logTag, "Video now inactive.");
    	}
    	else if(call->getCallInfo()->getVideoDirection() == CC_SDP_DIRECTION_SENDRECV)
    	{
    		//A GUI client would (re)display the video window at this point
    		CSFLogDebugS(logTag, "Video now active in both directions.");
    	}
    	else if(call->getCallInfo()->getVideoDirection() == CC_SDP_DIRECTION_RECVONLY)
    	{
    		//A GUI client would (re)display the video window at this point
    		CSFLogDebugS(logTag, "Video now active in receive direction.");
    	}
    	else
    	{
    		//We're not supporting either send-only or video,
    		//we should never end up in this state.
    		CSFLogDebugS(logTag, "Video now in unsupported configuration.");
    	}
    }
    else
    {
    	CSFLogDebugS(logTag,"Notified of VideoAvailable change, but no connected call to apply it to.");
    }

}

static void handleVideoOfferedOnConnectedCall(CallControlManagerPtr ccmPtr)
{
	CSFLogDebugS(logTag, "Remote party is now offering video, enter 'x' to accept their offer.");
}

static void handleDestroyAndCreateWindow(CallControlManagerPtr ccmPtr)
{
#ifndef NOVIDEO

	CSFLogDebugS(logTag, "Destroying video window...");
#ifdef WIN32
    PostMessage(hVideoWindow, WM_WINDOWCLOSE, 0, 0);
#else
    endWindowWorkItem = true;
	_WindowThreadEvent.Signal();
#endif

	// now post a new item
	CSFLogDebugS(logTag, "Creating video window...");

	videoThread.Start();

#else
	CSFLogDebugS(logTag, "Software built with no video flag, ignoring command.");
#endif
}

static bool handleAllUserRequests (CallControlManagerPtr ccmPtr)
{
    bool userRequestedQuit = false;
    base::AutoLock lock(_userOpRequestMutex);

    for (vector<UserOperationRequestDataPtr>::iterator it = _userOperationRequestVector.begin(); it != _userOperationRequestVector.end(); it++)
    {
        UserOperationRequestDataPtr pRequest = *it;

        switch (pRequest->request)
        {
        case eQuit:
            userRequestedQuit = true;
            break;
        case eIncomingCallReceived:
        	if(g_bAutoAnswer)
        	{
        		CSFLogDebug(logTag, "Waiting for %d second(s) before trying to anwer incoming call...\n", (CALL_ANSWER_DELAY_TIME_MS / 1000));
				base::PlatformThread::Sleep(CALL_ANSWER_DELAY_TIME_MS);
				handlePickUpFirstCallWithAnswerCaps(ccmPtr);
        	}
            break;
        case eAnswerCall:
        	handlePickUpFirstCallWithAnswerCaps(ccmPtr);
        	break;
        case eOriginatePhoneCall:
            if (pRequest->m_pData != NULL)
            {
                string * pNumber = pRequest->m_pPhoneNumberToCall;
                handleOriginatePhoneCall(ccmPtr, *pNumber);
            }
            break;
#ifndef NOVIDEO
        case eCycleThroughVideoPrefOptions:
            handleCycleThroughVideoPrefOptions(ccmPtr);
            break;
#endif
        case eEndFirstCallWithEndCallCaps:
            handleEndFirstCallWithEndCallCaps(ccmPtr);
            break;
        case eHoldFirstCallWithHoldCaps:
            handleHoldFirstCallWithHoldCaps(ccmPtr);
            break;
        case eResumeFirstCallWithResumeCaps:
            handleResumeFirstCallWithResumeCaps(ccmPtr);
            break;
        case eSendDTMFDigitOnFirstCallWithDTMFCaps:
            if (pRequest->m_pData != NULL)
            {
                cc_digit_t * pDigit = pRequest->m_pDTMFDigit;
                assert(pDigit != NULL);
                handleSendDTMFDigitOnFirstCallWithDTMFCaps(ccmPtr, *pDigit);
            }
            break;
        case eMuteAudioForConnectedCall:
            handleMuteAudioForFirstConnectedCall(ccmPtr);
            break;
        case eUnmuteAudioForConnectedCall:
            handleUnmuteAudioForFirstConnectedCall(ccmPtr);
            break;
        case eMuteVideoForConnectedCall:
            handleMuteVideoForFirstConnectedCall(ccmPtr);
            break;
        case eUnmuteVideoForConnectedCall:
            handleUnmuteVideoForFirstConnectedCall(ccmPtr);
            break;
        case eVolumeUp:
        	handleAdjustVolume(ccmPtr, +10);
            break;
        case eVolumeDown:
        	handleAdjustVolume(ccmPtr, -10);
            break;
        case eToggleAutoAnswer:
        	handleToggleAutoAnswer(ccmPtr);
            break;
        case ePrintActiveCalls:
        	printCallSummary(ccmPtr->getActiveDevice());
        	break;
        case eToggleShowVideoAutomatically:
        	handleToggleShowVideoAutomatically(ccmPtr);
        	break;
        case eAddVideoToConnectedCall:
        	handleAddVideoToConnectedCall(ccmPtr);
        	break;
        case eRemoveVideoFromConnectedCall:
        	handleRemoveVideoFromConnectedCall(ccmPtr);
        	break;
        case eVideoAvailableChange:
        	handleVideoAvailableChangeOnConnectedCall(ccmPtr);
        	break;
        case eVideoOffered:
        	handleVideoOfferedOnConnectedCall(ccmPtr);
        	break;
        case eDestroyAndCreateWindow:
        	handleDestroyAndCreateWindow(ccmPtr);
        	break;
        }//end switch

    }//end for

    _userOperationRequestVector.clear();
    return !userRequestedQuit;
}

static void processUserInput (CallControlManagerPtr ccmPtr)
{
	InputHandler input;
	MyUserOperationCallback callback;
	input.setCallback(&callback);

    while (1)
    {
        _userOperationRequestEvent.TimedWait(base::TimeDelta::FromMilliseconds(1000));

        if (!handleAllUserRequests(ccmPtr))
        {
            break;
        }
    }

    CSFLogDebugS(logTag, "User chose to stop test application.\nWaiting on user input thread to finish...");

	input.setCallback(NULL);
}

#define TEST_USER_NAME   "1000"
#define TEST_IP_ADDRESS  "10.99.10.75"
#define TEST_DEVICE_NAME "emannionsip01"

static string logDestination = "stdout";
static string CUCMIPAddress = TEST_IP_ADDRESS;
static string username = TEST_USER_NAME;
static string deviceName = TEST_DEVICE_NAME;
static string localIP = "";
static string credentials = "";


static void promptUserForInitialConfigInfo (const char * vxccCfgFilename)
{
	typedef map<string, string*> StringStringPtrMap;
	typedef pair<string, string*> StringStringPtrPair;

    StringStringPtrMap paramMap;

    paramMap.insert(StringStringPtrPair("IP_ADDRESS", &CUCMIPAddress));
    paramMap.insert(StringStringPtrPair("SIPUSER", &username));
    paramMap.insert(StringStringPtrPair("DEVICENAME", &deviceName));
    paramMap.insert(StringStringPtrPair("LOGFILE", &logDestination));
    paramMap.insert(StringStringPtrPair("LOCALIP", &localIP));
    paramMap.insert(StringStringPtrPair("CREDENTIALS", &credentials));

    ifstream myfile (vxccCfgFilename);

    if (myfile.is_open())
    {
        string line;
        while (getline(myfile, line))
        {
            unsigned long pos = line.find("=", 0);
            if (pos == line.npos)
            {
            	CSFLogDebug(logTag, "Config file has line with no assignement(=): %s", line.c_str());
            }
            string left(line, 0, pos);
            string right(line, pos+1);
            
            if (paramMap[left] == NULL)
            {
            	CSFLogDebug(logTag, "Bad param in config file: %s\n", line.c_str());
            }
            else
            {
                *paramMap[left] = right;
            }

        }
        myfile.close();
    }
    else
    {
        string input;
        cout << endl << "Enter SIP Server IP Address [" << CUCMIPAddress << "]: ";
        getline( cin, input, '\n');

        if (input.length() > 0)
        {
            CUCMIPAddress = input;
        }

        cout << "Enter SIP Server username (phone DN for CUCM) [" << username << "]: ";
        getline( cin, input, '\n');

        if (input.length() > 0)
        {
            username = input;
        }

        getPasswordFromConsole("Enter Password (not required for CUCM) [Enter]: ", input);
        if (input.length() > 0)
        {
        	credentials = input;
        }

        cout << "\nEnter device name (only required for CUCM) [" << deviceName << "]: ";
        getline( cin, input, '\n');

        if (input.length() > 0)
        {
        	deviceName = input;
        }
    }
}

/*
 * This function registers with the SIP server without using the device file.
 * This prevents the curl device file download.
 */
static int performRegister ()
{
    int returnCode = 0;
    int tBeforeConnect = GetTimeNow();

    if( ccmPtr->registerUser( deviceName, username, credentials, CUCMIPAddress ) == false)
    {
    	CSFLogDebugS(logTag, "Failed to connect.");
        returnCode = -1;
    }
    else
    {
        int loopCount = 0;
        int tDiff = GetTimeElapsedSince(tBeforeConnect);

        CSFLogDebug(logTag, "Connect succeeded after %d ms.\n", tDiff);

        while ((ccmPtr->getActiveDevice() == NULL) && (loopCount < 5))
        {
        	CSFLogDebugS(logTag, "Device not available yet. Trying again in 2 seconds...");
			base::PlatformThread::Sleep(2000);
            ++loopCount;
	    }

	    if (ccmPtr->getActiveDevice() != NULL)
	    {
	    	CSFLogDebugS(logTag, "Phone is now ready for use...");
	        processUserInput(ccmPtr);
	    }
	    else
	    {
	    	CSFLogDebugS(logTag, "Timed out waiting for device. Cannot continue...");
	        returnCode = -1;
	    }
    }

    return returnCode;
}

#ifndef WIN32
std::string proxy_ip_address_="10.99.10.75";
std::string local_ip_v4_address_;

//Only POSIX Complaint as of 7/6/11
static std::string NetAddressToString(const struct sockaddr* net_address,
                               socklen_t address_len) {

  // This buffer is large enough to fit the biggest IPv6 string.
  char buffer[128];
  int result = getnameinfo(net_address, address_len, buffer, sizeof(buffer),
                           NULL, 0, NI_NUMERICHOST);
  if (result != 0) {
    buffer[0] = '\0';
  }
  return std::string(buffer);
}

static bool GetLocalActiveInterfaceAddress()
{
	std::string local_ip_address = "0.0.0.0";
	CSFLogDebug(logTag, "Proxy IP address is (%s)\n", CUCMIPAddress.c_str());
	SOCKET sock_desc_ = INVALID_SOCKET;
	sock_desc_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in proxy_server_client;
 	proxy_server_client.sin_family = AF_INET;
	proxy_server_client.sin_addr.s_addr	= inet_addr(CUCMIPAddress.c_str());
	proxy_server_client.sin_port = 12345;
	fcntl(sock_desc_,F_SETFL,  O_NONBLOCK);
	int ret = connect(sock_desc_, reinterpret_cast<sockaddr*>(&proxy_server_client),
                    sizeof(proxy_server_client));

	if(ret == SOCKET_ERROR) {
	}

	struct sockaddr_storage source_address;
	socklen_t addrlen = sizeof(source_address);
	ret = getsockname(
			sock_desc_, reinterpret_cast<struct sockaddr*>(&source_address),&addrlen);

	//get the  ip address
	local_ip_address = NetAddressToString(
						reinterpret_cast<const struct sockaddr*>(&source_address),
						sizeof(source_address));
	local_ip_v4_address_ = local_ip_address;
	CSFLogDebug(logTag, "local IP address is (%s)\n", local_ip_address.c_str());

	close(sock_desc_);
	return true;
}
#endif

static int runMainLoop ()
{
    MyPhoneListener listener;
    CC_CapsPrinter CCCapsPrinter;
    ECC_CapsPrinter ECCCapsPrinter;

#ifndef NOVIDEO
    CSFLogDebugS(logTag, "Creating video window...");
	endWindowWorkItem = false;
	videoThread.Start();
#endif

    ccmPtr->addCCObserver(&listener);
    ccmPtr->addCCObserver(&CCCapsPrinter);
    ccmPtr->addECCObserver(&ECCCapsPrinter);

#ifndef WIN32
    GetLocalActiveInterfaceAddress();

    CSFLogDebug(logTag, "LOCAL IP IS %s \n", local_ip_v4_address_.c_str());

    ccmPtr->setLocalIpAddressAndGateway(local_ip_v4_address_,"");
#else
    ccmPtr->setLocalIpAddressAndGateway(localIP,"");
#endif

    ccmPtr->setSIPCCLoggingMask( GSM_DEBUG_BIT | FIM_DEBUG_BIT | SIP_DEBUG_MSG_BIT | CC_APP_DEBUG_BIT | SIP_DEBUG_REG_STATE_BIT );

    return performRegister();
}

static void initLogging(int argc, char** argv)
{
	InitChromeLogging(argc, argv);
}

int main(int argc, char** argv)
{
    const char* vxccCfgFilename ="cucm-credentials.txt";

    if (argc > 1)
    {
        vxccCfgFilename = argv[1];
    }
    
    promptUserForInitialConfigInfo(vxccCfgFilename);

    initLogging(argc, argv);


#ifdef LOOP
    for (int i=0; i<5; i++)
    {
#endif
    	CSFLogDebugS(logTag, "Creating a new CallControlManager object.");
        ccmPtr = CallControlManager::create();

        runMainLoop();

        CSFLogDebugS(logTag, "Disconnecting");
        ccmPtr->disconnect();
        CSFLogDebugS(logTag, "Ready to destroy CallControlManager object.");
        ccmPtr->destroy();
        ccmPtr = NULL_PTR(CallControlManager);
        CSFLogDebugS(logTag, "Finished destroying CallControlManager object.");

#ifndef NOVIDEO
        CSFLogDebugS(logTag, "Destroying video window...");
#ifdef WIN32
        PostMessage(hVideoWindow, WM_WINDOWCLOSE, 0, 0);
#else
	    endWindowWorkItem = true;
		_WindowThreadEvent.Signal();
#endif

		if(videoThread.HasBeenStarted())
        {
#ifdef WIN32
			PostMessage(hVideoWindow, WM_WINDOWCLOSE, 0, 0);
#else
			endWindowWorkItem = true;
			_WindowThreadEvent.Signal();
#endif
			videoThread.Join();
        }
#endif

#ifdef LOOP
    }
#endif

}
