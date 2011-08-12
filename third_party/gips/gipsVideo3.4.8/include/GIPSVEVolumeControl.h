// GIPSVEVolumeControl.h
//
// Copyright (c) 2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_VOLUME_CONTROL_H__)
#define __GIPSVE_VOLUME_CONTROL_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

class VOICEENGINE_DLLEXPORT GIPSVEVolumeControl
{
public:
    static GIPSVEVolumeControl* GetInterface(GIPSVoiceEngine* voiceEngine);

    // Mic, Speaker and WaveOut calls change the device settings on the computer.
    // Input and Output calls change the signals coming in to and going out from
    // VoiceEngine without affecting the device settings.

    // Speaker volume
    virtual int GIPSVE_SetSpeakerVolume(unsigned int volume) = 0;
    virtual int GIPSVE_GetSpeakerVolume(unsigned int& volume) = 0;

    // Microphone volume and input muting
    virtual int GIPSVE_SetMicVolume(unsigned int volume) = 0;
    virtual int GIPSVE_GetMicVolume(unsigned int& volume) = 0;
    virtual int GIPSVE_SetInputMute(int channel, bool enable) = 0;
    virtual int GIPSVE_GetInputMute(int channel, bool& enabled) = 0;

    // Speech level indications
    virtual int GIPSVE_GetSpeechInputLevel(unsigned int& level) = 0;
    virtual int GIPSVE_GetSpeechOutputLevel(int channel, unsigned int& level) = 0;
    virtual int GIPSVE_GetSpeechInputLevelFullRange(unsigned int& level) = 0;
    virtual int GIPSVE_GetSpeechOutputLevelFullRange(int channel, unsigned int& level) = 0;

    // Per channel output volume scaling
    virtual int GIPSVE_SetChannelOutputVolumeScaling(int channel, float scaling) = 0;
    virtual int GIPSVE_GetChannelOutputVolumeScaling(int channel, float& scaling) = 0;

    // Wave volume (Windows only)
    virtual int GIPSVE_SetWaveOutVolume(unsigned int volume) = 0;
    virtual int GIPSVE_GetWaveOutVolume(unsigned int& volume) = 0;

    // Stereo panning (Windows only)
    virtual int GIPSVE_SetOutputVolumePan(float left, float right) = 0;
    virtual int GIPSVE_GetOutputVolumePan(float& left, float& right) = 0;
    virtual int GIPSVE_SetChannelOutputVolumePan(int channel, float left, float right) = 0;
    virtual int GIPSVE_GetChannelOutputVolumePan(int channel, float& left, float& right) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVEVolumeControl();
};

#endif    // #if !defined(__GIPSVE_VOLUME_CONTROL_H__)
