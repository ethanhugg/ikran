// GIPSVEHardware.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_HARDWARE_H__)
#define __GIPSVE_HARDWARE_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;
class SndCardObject;

class VOICEENGINE_DLLEXPORT GIPSVEHardware
{
public:
    static GIPSVEHardware* GetInterface(GIPSVoiceEngine* voiceEngine);

    // CPU load info
    virtual int GIPSVE_GetCPULoad(int& loadPercent) = 0;
    virtual int GIPSVE_GetSystemCPULoad(int& loadPercent) = 0;

    // Soundcard device handling
    virtual int GIPSVE_GetNumOfSoundDevices(int& playout, int& recording) = 0;
    virtual int GIPSVE_GetRecordingDeviceName(int index, char* strNameUTF8, int nameLen, char* strGuidUTF8 = NULL, int guidLen = 0) = 0;
    virtual int GIPSVE_GetPlayoutDeviceName(int index, char* strNameUTF8, int nameLen, char* strGuidUTF8 = NULL, int guidLen = 0) = 0;
    virtual int GIPSVE_SetSoundDevices(int recordingIndex, int playoutIndex, bool disableMicBoost = false, GIPS_StereoChannel recordingChannel = GIPS_StereoBoth)= 0;
    virtual int GIPSVE_SetSoundDevices(const char* recordingDevice, const char* playoutDevice) = 0;
    virtual int GIPSVE_GetPlayoutDeviceStatus(bool& isAvailable) = 0;
    virtual int GIPSVE_GetRecordingDeviceStatus(bool& isAvailable) = 0;
    virtual int GIPSVE_ResetSoundDevice() = 0;
    virtual int GIPSVE_SoundDeviceControl(unsigned int par1, unsigned int par2, unsigned int par3) = 0;

    // External soundcard interface
    virtual int GIPSVE_SetSoundCardObject(SndCardObject& object) = 0;
    virtual int GIPSVE_NeedMorePlayData(short* speechData10ms, int samplingFreqHz, int currentDelayMs, int encoding, int& samplesOut) = 0;
    virtual int GIPSVE_RecordedDataIsAvailable(short* speechData10ms, int samplingFreqHz, int currentDelayMs, int decoding, int& encodingDone) = 0;

    // Hardware and os info
    virtual int GIPSVE_GetDevice(char* device, unsigned int length) = 0;
    virtual int GIPSVE_GetPlatform(char* platform, unsigned int length) = 0;
    virtual int GIPSVE_GetOS(char* os, unsigned int length) = 0;

    // Grab soundcard
    virtual int GIPSVE_SetGrabPlayout(bool enable) = 0;
    virtual int GIPSVE_SetGrabRecording(bool enable) = 0;

    // Speaker audio routing
    virtual int GIPSVE_SetLoudspeakerStatus(bool enable) = 0;

	// Soundcard sampling rate (Embedded Linux only)
    virtual int GIPSVE_SetSamplingRate(int freqkHz) = 0;
    virtual int GIPSVE_GetSamplingRate(int& freqkHz)= 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVEHardware();
};

#endif    // #if !defined(__GIPSVE_HARDWARE_H__)
