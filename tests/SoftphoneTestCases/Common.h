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

//static cc_sdp_direction_t _sVideoPref = CC_SDP_DIRECTION_INACTIVE;
using namespace std;

typedef enum eUserOperationRequest
{
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


#endif
