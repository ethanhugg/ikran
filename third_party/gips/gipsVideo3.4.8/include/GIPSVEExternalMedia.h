// GIPSVEExternalMedia.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_EXTERNAL_MEDIA_H__)
#define __GIPSVE_EXTERNAL_MEDIA_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

enum GIPS_ProcessingTypes
{
    PLAYBACK_PER_CHANNEL = 0,
    PLAYBACK_ALL_CHANNELS_MIXED,
    RECORDING_PER_CHANNEL,
    RECORDING_ALL_CHANNELS_MIXED
};

class VOICEENGINE_DLLEXPORT GIPSVEMediaProcess
{
public:
    virtual void Process(int channelNumber, short* audioLeft10ms, short* audioRight10ms, int length, int samplingFreq, bool isStereo) = 0;
protected:
    virtual ~GIPSVEMediaProcess() {}
};

class VOICEENGINE_DLLEXPORT GIPSVEExternalMedia
{
public:
    static GIPSVEExternalMedia* GetInterface(GIPSVoiceEngine* voiceEngine);

    // External recording and playout
    virtual int GIPSVE_SetExternalRecording(bool enable) = 0;
    virtual int GIPSVE_SetExternalPlayout(bool enable) = 0;
    virtual int GIPSVE_ExternalRecordingInsertData(const short* speechData10ms, unsigned int lengthSamples, int samplingFreqHz, int currentDelayMs) = 0;
    virtual int GIPSVE_ExternalPlayoutGetData(short* speechData10ms, int samplingFreqHz, int currentDelayMs, unsigned int& lengthSamples) = 0;

    // External media processing
    virtual int GIPSVE_SetExternalMediaProcessing(GIPS_ProcessingTypes type, int channel, bool enable, GIPSVEMediaProcess& proccessObject) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVEExternalMedia();
};

#endif    // #if !defined(__GIPSVE_EXTERNAL_MEDIA_H__)
