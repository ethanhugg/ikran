#include "Common.h"

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

