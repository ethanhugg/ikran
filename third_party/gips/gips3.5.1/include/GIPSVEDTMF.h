// GIPSVEDTMF.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_DTMF_H__)
#define __GIPSVE_DTMF_H__

#include "GIPSVECommon.h"

enum GIPS_TelephoneEventDetectionMethods
{
    IN_BAND = 0,
    OUT_OF_BAND,
    IN_AND_OUT_OF_BAND
};

class GIPSVoiceEngine;

class VOICEENGINE_DLLEXPORT GIPSVEDTMFCallback
{
public:
    virtual void OnDTMFEvent(short channel, short event, bool endOfEvent) = 0;
protected:
    virtual ~GIPSVEDTMFCallback() { }
};

class VOICEENGINE_DLLEXPORT GIPSVEDTMF
{
public:
    static GIPSVEDTMF* GetInterface(GIPSVoiceEngine* voiceEngine);

    // Telephone event sending
    // This includes DTMF but also applies to other telephone events
    virtual int GIPSVE_SendDTMF(int channel, int eventNumber, bool outBand = true, int lengthMs = 160, int attenuationDb = 10) = 0;
    virtual int GIPSVE_SetSendDTMFPayloadType(int channel, int type) = 0;

    // DTMF sending feedback
    virtual int GIPSVE_SetDTMFFeedbackStatus(bool enable, bool directFeedback = false) = 0;
    virtual int GIPSVE_GetDTMFFeedbackStatus(bool& enabled, bool& directFeedback) = 0;

    // Local DTMF playout
    virtual int GIPSVE_PlayDTMFTone(int eventNumber, int lengthMs = 200, int attenuationDb = 10) = 0;

    // Telephone event detection
    // This includes DTMF but also applies to other telephone events
    virtual int GIPSVE_SetDTMFDetection(bool enable, GIPSVEDTMFCallback* dtmfCallback,
        GIPS_TelephoneEventDetectionMethods detectionMethod = IN_BAND) = 0;

    // Enable or disable playout of received out-of-band DTMF events
    virtual int GIPSVE_SetDTMFPlayoutStatus(int channel, bool enable) = 0;
    virtual int GIPSVE_GetDTMFPlayoutStatus(int channel, bool& enabled) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVEDTMF();
};

#endif    // #if !defined(__GIPSVE_DTMF_H__)
