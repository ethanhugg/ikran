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
// vie_autotest.h
//

#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_

#include "common_types.h"

#include "voe_base.h"
#include "voe_codec.h"
#include "voe_hardware.h"
#include "voe_audio_processing.h"

#include "vie_base.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "vie_file.h"
#include "vie_network.h"
#include "vie_render.h"
#include "vie_rtp_rtcp.h"
//#include "vie_defines.h"
#include "vie_errors.h"
//#include "video_render_defines.h"

#include <string>

const unsigned int kMaxIPLength = 16;

class ViEMediaTest
{
public:
 	//Singleton Object
    static ViEMediaTest* GetInstance();

    ViEMediaTest();
    ~ViEMediaTest();

    // custom call and helper functions
    int InitMediaEngine();
    int StartMedia();
    void StopMedia();
	// we need this to do a poor software color model conversion
	 void SetExternalRenderer(void* renderer) 
	{
    	ext_renderer = renderer;
    }

	void SetPeerIPAddress(const char* ipadd)
	{
    	memset(ipAddress, 0, kMaxIPLength);
		strcpy(ipAddress, ipadd);	
	}

	void SetTxAudioPort(std::string aTxPort)
	{
		 audioTxPort = atoi(aTxPort.c_str());
	}

	void SetRxAudioPort(std::string aRxPort)
	{
		 audioRxPort = atoi(aRxPort.c_str());
	}

	void SetTxVideoPort(std::string vTxPort)
	{
		 videoTxPort = atoi(vTxPort.c_str());
	}

	void SetRxVideoPort(std::string vRxPort)
	{
		 videoRxPort = atoi(vRxPort.c_str());
	}


    // general settings functions
    bool GetVideoDevice(webrtc::ViEBase* ptrViEBase,
                        webrtc::ViECapture* ptrViECapture,
                        char* captureDeviceName, char* captureDeviceUniqueId);
    bool GetIPAddress(char* IP);
    bool ValidateIP(std::string iStr);
    void PrintCallInformation(char* IP, char* videoCaptureDeviceName,
                              char* videoCaptureUniqueId,
                              webrtc::VideoCodec videoCodec, int videoTxPort,
                              int videoRxPort, char* audioCaptureDeviceName,
                              char* audioPlaybackDeviceName,
                              webrtc::CodecInst audioCodec, int audioTxPort,
                              int audioRxPort);

    // video settings functions
    bool GetVideoPorts(int* txPort, int* rxPort);
    bool GetVideoCodec(webrtc::ViECodec* ptrViECodec,
                       webrtc::VideoCodec& videoCodec);

    // audio settings functions
    bool GetAudioDevices(webrtc::VoEBase* ptrVEBase,
                         webrtc::VoEHardware* ptrVEHardware,
                         char* recordingDeviceName, int& recordingDeviceIndex,
                         char* playbackDeviceName, int& playbackDeviceIndex);
    bool GetAudioDevices(webrtc::VoEBase* ptrVEBase,
                         webrtc::VoEHardware* ptrVEHardware,
                         int& recordingDeviceIndex, int& playbackDeviceIndex);
    bool GetAudioPorts(int* txPort, int* rxPort);
    bool GetAudioCodec(webrtc::VoECodec* ptrVeCodec,
                       webrtc::CodecInst& audioCodec);

private:
    void PrintAudioCodec(const webrtc::CodecInst audioCodec);
    void PrintVideoCodec(const webrtc::VideoCodec videoCodec);

	//webrtc class instance variables
	webrtc::VoiceEngine* ptrVE ;
	webrtc::VoEBase* ptrVEBase;
	webrtc::VoECodec* ptrVECodec;
	webrtc::VoEHardware* ptrVEHardware;
	webrtc::VoEAudioProcessing* ptrVEAPM;
	webrtc::VideoEngine* ptrViE ;
    webrtc::ViEBase* ptrViEBase;
    webrtc::ViECapture* ptrViECapture;
    webrtc::ViERender* ptrViERender;
    webrtc::ViECodec* ptrViECodec ;
    webrtc::ViENetwork* ptrViENetwork;
	webrtc::ViERTP_RTCP* ptrViERtpRtcp;
	int captureId;
	int videoTxPort;
    int videoRxPort;
    int videoChannel;
    int audioTxPort;
    int audioRxPort;
	int audioChannel;
	int audioCaptureDeviceIndex;
	int audioPlaybackDeviceIndex;
	char ipAddress[kMaxIPLength];
	webrtc::CodecInst audioCodec;
	webrtc::VideoCodec videoCodec;
    
	void* ext_renderer;
     // singleton instance
    static ViEMediaTest* _instance;

};

#endif  // WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_
