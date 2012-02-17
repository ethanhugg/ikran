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
 *  Kai Chen <kaichen2@cisco.com>
 *  Yi Wang <yiw2@cisco.com>
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
 
#ifndef _COMMON
#define _COMMON

#include "SharedPtr.h"
#include "cc_constants.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include "base/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/lock.h"
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

using namespace std;

extern cc_sdp_direction_t _sVideoPref;

typedef enum eUserOperationRequest
{
    eLoop,
    eExit,
    eQuit,
	eIncomingCallReceived,
    eOriginatePhoneCall,
	sOriginatePhoneCall,
    eOriginateP2PPhoneCall,
    eAnswerCall,
	sAnswerCall,
	sNoAnswerCall,
    eCycleThroughVideoPrefOptions,
    eEndFirstCallWithEndCallCaps,
	sEndFirstCallWithEndCallCaps,
    eHoldFirstCallWithHoldCaps,
    eResumeFirstCallWithResumeCaps,
    eSendDTMFDigitOnFirstCallWithDTMFCaps,
    eMuteAudioForConnectedCall,
    eUnmuteAudioForConnectedCall,
    eMuteVideoForConnectedCall,
    eUnmuteVideoForConnectedCall,
    eVolumeUp,
    eVolumeDown,
    eToggleAutoAnswer,
    ePrintActiveCalls,
    eToggleShowVideoAutomatically,
    eAddVideoToConnectedCall,
    eRemoveVideoFromConnectedCall,
    eVideoAvailableChange,
    eVideoOffered,
    eDestroyAndCreateWindow,
	eIdle,
	eAnswerIdle,
	eWait
} eUserOperationRequest;

DECLARE_PTR(UserOperationRequestData);
class UserOperationRequestData
{
public:
    eUserOperationRequest request;

    UserOperationRequestData(eUserOperationRequest req, void * pData) : request(req), m_pData(pData) {}
    ~UserOperationRequestData();

    union
    {
        void * m_pData;
        std::string * m_pPhoneNumberToCall;// Data associated with "eOriginatePhoneCall" request.
        cc_digit_t * m_pDTMFDigit;
    };
private:
    UserOperationRequestData(const UserOperationRequestData& rhs);
    const UserOperationRequestData& operator=(const UserOperationRequestData& rhs);
};

class UserOperationCallback
{
public:
	virtual void onUserRequest(UserOperationRequestDataPtr data) = 0;
	virtual void onAddSignal(UserOperationRequestDataPtr data) =0;
	
	virtual void onAddUserOperation(UserOperationRequestDataPtr data)=0;
};


cc_sdp_direction_t getActiveVideoPref ();
void cycleToNextVideoPref();
const char * getUserFriendlyNameForVideoPref(cc_sdp_direction_t videoPref);


extern base::Lock _userOpRequestMutex;

extern  base::WaitableEvent _userOperationRequestEvent;
extern  base::WaitableEvent _userOperationResponseEvent;

int csf_getch(void);
int csf_kbhit(void);

#endif
