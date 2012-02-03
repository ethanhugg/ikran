/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//
// vie_autotest_defines.h
//


#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_DEFINES_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_DEFINES_H_

#include <cassert>
#include <stdarg.h>
#include <stdio.h>

#include "third_party/webrtc/engine_configurations.h"


// Choose how to log
//#define VIE_LOG_TO_FILE
#define VIE_LOG_TO_STDOUT

// Choose one way to test error
#define VIE_ASSERT_ERROR

#define VIE_LOG_FILE_NAME "ViEAutotestLog.txt"

#undef RGB
#define RGB(r,g,b) r|g<<8|b<<16

// Default values for custom call
#define DEFAULT_SEND_IP					"127.0.0.1"
#define DEFAULT_VIDEO_PORT			    1024	
#define DEFAULT_VIDEO_CODEC				"vp8"
#define DEFAULT_VIDEO_CODEC_WIDTH	 	640	
#define DEFAULT_VIDEO_CODEC_HEIGHT	    480		
#define DEFAULT_AUDIO_PORT				163840
#define DEFAULT_AUDIO_CODEC				"isac"

enum
{
    KAutoTestSleepTimeMs = 5000
};

#endif  // WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_DEFINES_H_
