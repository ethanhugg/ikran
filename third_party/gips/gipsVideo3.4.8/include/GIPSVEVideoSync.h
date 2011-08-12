// GIPSVEVideoSync.h
//
// Copyright (c) 2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_VIDEO_SYNC_H__)
#define __GIPSVE_VIDEO_SYNC_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

class VOICEENGINE_DLLEXPORT GIPSVEVideoSync
{
public:
    static GIPSVEVideoSync* GetInterface(GIPSVoiceEngine* voiceEngine);

    // Received RTP timestamp
    virtual int GIPSVE_GetPlayoutTimestamp(int channel, unsigned int& timestamp) = 0;

    // Send RTP timestamp and sequence number
    virtual int GIPSVE_SetInitTimestamp(int channel, unsigned int timestamp) = 0;
    virtual int GIPSVE_SetInitSequenceNumber(int channel, short sequenceNumber) = 0;

    // Delay
    virtual int GIPSVE_SetMinimumPlayoutDelay(int channel, int delayMs) = 0;
    virtual int GIPSVE_GetDelayEstimate(int channel, int& delayMs) = 0;
    virtual int GIPSVE_GetSoundcardBufferSize(int& bufferMs) = 0;

    virtual int Release() = 0;

protected:
    virtual ~GIPSVEVideoSync();
};

#endif  // #if !defined(__GIPSVE_VIDEO_SYNC_H__)
