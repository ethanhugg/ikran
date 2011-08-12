// GIPSVEPTT.h
//
// Copyright (c) 2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_PTT_H__)
#define __GIPSVE_PTT_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

class VOICEENGINE_DLLEXPORT GIPSVEPTT
{
public:
    static GIPSVEPTT* GetInterface(GIPSVoiceEngine* voiceEngine);

    // PTT
    virtual int GIPSVE_StartPTTPlayout(int channel) = 0;
    virtual int GIPSVE_GetPTTActivity(int channel, bool& activity) = 0;

    virtual int Release() = 0;

protected:
    virtual ~GIPSVEPTT();
};

#endif    // #if !defined(__GIPSVE_PTT_H__)
