// GIPSVECommon.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPS_VE_COMMON_H__)
#define __GIPS_VE_COMMON_H__

#ifndef NULL
#define NULL    0
#endif

#if defined(_WIN32) || defined(_WIN64)
#ifndef VOICEENGINE_DLLEXPORT
#ifdef GIPS_EXPORT
#define VOICEENGINE_DLLEXPORT _declspec(dllexport)
#elif GIPS_DLL
#define VOICEENGINE_DLLEXPORT _declspec(dllimport)
#else
#define VOICEENGINE_DLLEXPORT
#endif
#endif
#else
#if (__GNUC__ >= 4)
#define VOICEENGINE_DLLEXPORT __attribute__((visibility("default")))
#else
#define VOICEENGINE_DLLEXPORT
#endif
#endif

#include "GIPS_common_types.h"

enum GIPS_StereoChannel
{
    GIPS_StereoLeft = 0,
    GIPS_StereoRight,
    GIPS_StereoBoth
};

#endif
