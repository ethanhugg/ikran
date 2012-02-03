/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 *  vie_autotest_custom_call.cc
 *
 */

#include "vie_mediatest_defines.h"
#include "vie_mediatest.h"

#include <iostream>

using namespace std;

#define VCM_RED_PAYLOAD_TYPE        96
#define VCM_ULPFEC_PAYLOAD_TYPE     97

static const char* logTag = "Ikran";

ViEMediaTest* ViEMediaTest::_instance = 0;


ViEMediaTest* ViEMediaTest::GetInstance()
{
        if(_instance == 0)
        {
                _instance = new ViEMediaTest();
        }
        return _instance;
}

ViEMediaTest::ViEMediaTest():videoRxPort(8888),
						   audioRxPort(7777)
						
{

}


int ViEMediaTest::InitMediaEngine()
{

  // obtain all the interfaces
  int error = 0;
  std::string str;
	
  // VoE
  ptrVE = webrtc::VoiceEngine::Create();
	
  ptrVEBase = webrtc::VoEBase::GetInterface(ptrVE);
	
  error = ptrVEBase->Init();
 
	
  ptrVECodec = webrtc::VoECodec::GetInterface(ptrVE);
	
  ptrVEHardware = webrtc::VoEHardware::GetInterface(ptrVE);
	
  ptrVEAPM = webrtc::VoEAudioProcessing::GetInterface(ptrVE);
    
  // ViE
  ptrViE = webrtc::VideoEngine::Create();
	
  ptrViEBase = webrtc::ViEBase::GetInterface(ptrViE);
	
  error = ptrViEBase->Init();


  ptrViECapture = webrtc::ViECapture::GetInterface(ptrViE);
	
  ptrViERender = webrtc::ViERender::GetInterface(ptrViE);
	
  ptrViECodec = webrtc::ViECodec::GetInterface(ptrViE);
	
  ptrViENetwork = webrtc::ViENetwork::GetInterface(ptrViE);
		
	
  // video codecs
  memset((void*)&videoCodec, 0, sizeof(videoCodec));
  GetVideoCodec(ptrViECodec, videoCodec);
	
  // audio codec
  memset((void*)&audioCodec, 0, sizeof(audioCodec));
  GetAudioCodec(ptrVECodec, audioCodec);
	
  return 0;
}

int ViEMediaTest::StartMedia()
{
    const unsigned int KMaxUniqueIdLength = 256;
    int error = 0;
    std::string str;
	bool startCall = false;
	startCall = true;
	char uniqueId[KMaxUniqueIdLength] = "";
    char deviceName[KMaxUniqueIdLength] = "";
    char audioCaptureDeviceName[KMaxUniqueIdLength] = "";
    char audioPlaybackDeviceName[KMaxUniqueIdLength] = "";
    audioCaptureDeviceIndex = -1;
    audioPlaybackDeviceIndex = -1;

	//audio devices
    memset(audioCaptureDeviceName, 0, KMaxUniqueIdLength);
    memset(audioPlaybackDeviceName, 0, KMaxUniqueIdLength);
    GetAudioDevices(ptrVEBase, ptrVEHardware, audioCaptureDeviceName,
                    audioCaptureDeviceIndex, audioPlaybackDeviceName,
                    audioPlaybackDeviceIndex);


	
	// video devices
	memset(deviceName, 0, KMaxUniqueIdLength);
	memset(uniqueId, 0, KMaxUniqueIdLength);
	GetVideoDevice(ptrViEBase, ptrViECapture, deviceName, uniqueId);
	


    //***************************************************************
    // Begin create/initialize WebRTC Video Engine for testing
    //***************************************************************
    if(startCall == true)
    {
        // Configure Audio first
        audioChannel = ptrVEBase->CreateChannel();
        error = ptrVEBase->SetSendDestination(audioChannel, audioTxPort,
                                              ipAddress);

        error = ptrVEBase->SetLocalReceiver(audioChannel, audioRxPort);

        error = ptrVEHardware->SetRecordingDevice(audioCaptureDeviceIndex);

        error = ptrVEHardware->SetPlayoutDevice(audioPlaybackDeviceIndex);

        error = ptrVECodec->SetSendCodec(audioChannel, audioCodec);

        error = ptrVEAPM->SetAgcStatus(true, webrtc::kAgcDefault);

        error = ptrVEAPM->SetNsStatus(true, webrtc::kNsHighSuppression);

		
		
        ptrViE->SetTraceFilter(webrtc::kTraceAll);
        error = ptrViE->SetTraceFile("/tmp/ViECustomCall.txt");

        error = ptrViEBase->SetVoiceEngine(ptrVE);

        error = ptrViEBase->CreateChannel(videoChannel);

        error = ptrViEBase->ConnectAudioChannel(videoChannel, audioChannel);

		captureId = 0;
		error = ptrViECapture->AllocateCaptureDevice(uniqueId,
													 KMaxUniqueIdLength,
													 captureId);
        error = ptrViECapture->ConnectCaptureDevice(captureId, videoChannel);
        error = ptrViECapture->StartCapture(captureId);
	    ptrViERtpRtcp =
		webrtc::ViERTP_RTCP::GetInterface(ptrViE);
        error = ptrViERtpRtcp->SetRTCPStatus(videoChannel,
                                             webrtc::kRtcpCompound_RFC4585);
        error = ptrViERtpRtcp->SetKeyFrameRequestMethod(
											 videoChannel, webrtc::kViEKeyFrameRequestPliRtcp);
		int protectionMethod = 0;
		switch (protectionMethod)
        {
            case 0: // None
				error = ptrViERtpRtcp->SetFECStatus(videoChannel, false,
													VCM_RED_PAYLOAD_TYPE,
													VCM_ULPFEC_PAYLOAD_TYPE);
			   error = ptrViERtpRtcp->SetNACKStatus(videoChannel, false);
				break;
				
            case 1: // FEC only
				error = ptrViERtpRtcp->SetFECStatus(videoChannel, true,
													VCM_RED_PAYLOAD_TYPE,
													VCM_ULPFEC_PAYLOAD_TYPE);
				
				error = ptrViERtpRtcp->SetNACKStatus(videoChannel, false);
				break;
				
            case 2: // NACK only
				error = ptrViERtpRtcp->SetNACKStatus(videoChannel, true);
				break;
				
            case 3: // Hybrid NACK and FEC
				error = ptrViERtpRtcp->SetHybridNACKFECStatus(videoChannel, true,
															  VCM_RED_PAYLOAD_TYPE,
															  VCM_ULPFEC_PAYLOAD_TYPE);
				break;
        }
		
        error = ptrViERtpRtcp->SetTMMBRStatus(videoChannel, true);
		error = ptrViERender->AddRenderer(videoChannel, (webrtc::RawVideoType)0,
											(webrtc::ExternalRenderer*) ext_renderer);

        error = ptrViENetwork->SetSendDestination(videoChannel, ipAddress,
                                                  videoTxPort);

        error = ptrViENetwork->SetLocalReceiver(videoChannel, videoRxPort);

        error = ptrViECodec->SetSendCodec(videoChannel, videoCodec);

        error = ptrViECodec->SetReceiveCodec(videoChannel, videoCodec);
	  
		// Set receive codecs for FEC and hybrid NACK/FEC
        if (protectionMethod == 1 || protectionMethod == 3)
        {
            // RED
            error = ptrViECodec->GetCodec(ptrViECodec->NumberOfCodecs() - 2,
                                          videoCodec);
           
            error = ptrViECodec->SetReceiveCodec(videoChannel, videoCodec);
           
            // ULPFEC
            error = ptrViECodec->GetCodec(ptrViECodec->NumberOfCodecs() - 1,
										  videoCodec);
           
            error = ptrViECodec->SetReceiveCodec(videoChannel, videoCodec);
		}
       

        // **** start the engines
        // VE first
        error = ptrVEBase->StartReceive(audioChannel);

        error = ptrVEBase->StartPlayout(audioChannel);

        error = ptrVEBase->StartSend(audioChannel);


        // ViE next
        error = ptrViEBase->StartSend(videoChannel);

        error = ptrViEBase->StartReceive(videoChannel);

        error = ptrViERender->StartRender(videoChannel);
       
    }
    return 0;
}


void ViEMediaTest::StopMedia()
{
  int error = 0;
	
  // audio engine first
  error = ptrVEBase->StopReceive(audioChannel);
	
  error = ptrVEBase->StopPlayout(audioChannel);
	
  error = ptrVEBase->DeleteChannel(audioChannel);
	
  // now do video
  error = ptrViEBase->DisconnectAudioChannel(videoChannel);
	
  error = ptrViEBase->StopReceive(videoChannel);
	
  error = ptrViEBase->StopSend(videoChannel);
	
  error = ptrViERender->StopRender(captureId);
	
  error = ptrViERender->StopRender(videoChannel);
	
  error = ptrViERender->RemoveRenderer(captureId);
	
  error = ptrViERender->RemoveRenderer(videoChannel);
	
  error = ptrViECapture->StopCapture(captureId);
	
  error = ptrViECapture->DisconnectCaptureDevice(videoChannel);
	
  error = ptrViECapture->ReleaseCaptureDevice(captureId);
	
  error = ptrViEBase->DeleteChannel(videoChannel);
	
  int remainingInterfaces = 0;
  remainingInterfaces = ptrViECodec->Release();
	
  remainingInterfaces = ptrViECapture->Release();
	
  remainingInterfaces = ptrViERtpRtcp->Release();
	
  remainingInterfaces = ptrViERender->Release();
	
  remainingInterfaces = ptrViENetwork->Release();
	
  remainingInterfaces = ptrViEBase->Release();
	
  webrtc::VideoEngine::Delete(ptrViE);
	
}

//***************************************************************
//  UTILITY FUNCTION ON WEBRTC
//***************************************************************

bool ViEMediaTest::GetVideoDevice(webrtc::ViEBase* ptrViEBase,
                                 webrtc::ViECapture* ptrViECapture,
                                 char* captureDeviceName,
                                 char* captureDeviceUniqueId)
{
    int error = 0;
    int captureDeviceIndex = 0;
    std::string str;

    const unsigned int KMaxDeviceNameLength = 128;
    const unsigned int KMaxUniqueIdLength = 256;
    char deviceName[KMaxDeviceNameLength];
    char uniqueId[KMaxUniqueIdLength];

    while(1)
    {
        memset(deviceName, 0, KMaxDeviceNameLength);
        memset(uniqueId, 0, KMaxUniqueIdLength);

        int captureIdx = 0;
        for (captureIdx = 0;
             captureIdx < ptrViECapture->NumberOfCaptureDevices(); captureIdx++)
        {
            memset(deviceName, 0, KMaxDeviceNameLength);
            memset(uniqueId, 0, KMaxUniqueIdLength);

            error = ptrViECapture->GetCaptureDevice(captureIdx, deviceName,
                                                    KMaxDeviceNameLength,
                                                    uniqueId,
                                                    KMaxUniqueIdLength);

        }
        captureDeviceIndex = 0;

        if(captureDeviceIndex == 0)
        {
            // use default (or first) camera
            error = ptrViECapture->GetCaptureDevice(0, deviceName,
                                                    KMaxDeviceNameLength,
                                                    uniqueId,
                                                    KMaxUniqueIdLength);
            strcpy(captureDeviceUniqueId, uniqueId);
            strcpy(captureDeviceName, deviceName);
            return true;
        }
        else if(captureDeviceIndex < 0
                || (captureDeviceIndex >
                    (int)ptrViECapture->NumberOfCaptureDevices()))
        {
            // invalid selection
            continue;
        }
        else
        {
            error = ptrViECapture->GetCaptureDevice(captureDeviceIndex - 1,
                                                    deviceName,
                                                    KMaxDeviceNameLength,
                                                    uniqueId,
                                                    KMaxUniqueIdLength);
            strcpy(captureDeviceName, uniqueId);
            strcpy(captureDeviceName, deviceName);
            return true;
        }
    }
}

bool ViEMediaTest::GetAudioDevices(webrtc::VoEBase* ptrVEBase,
                                  webrtc::VoEHardware* ptrVEHardware,
                                  char* recordingDeviceName,
                                  int& recordingDeviceIndex,
                                  char* playbackDeviceName,
                                  int& playbackDeviceIndex)
{
    int error = 0;
    std::string str;

    const unsigned int KMaxDeviceNameLength = 128;
    const unsigned int KMaxUniqueIdLength = 128;
    char recordingDeviceUniqueName[KMaxDeviceNameLength];
    char playbackDeviceUniqueName[KMaxUniqueIdLength];

    int numberOfRecordingDevices = -1;
    error = ptrVEHardware->GetNumOfRecordingDevices(numberOfRecordingDevices);

    while(1)
    {
        recordingDeviceIndex = -1;
        int captureIdx = 0;

        for (captureIdx = 0; captureIdx < numberOfRecordingDevices;
             captureIdx++)
        {
            memset(recordingDeviceName, 0, KMaxDeviceNameLength);
            memset(recordingDeviceUniqueName, 0, KMaxDeviceNameLength);
            error = ptrVEHardware->GetRecordingDeviceName(
                captureIdx, recordingDeviceName, recordingDeviceUniqueName);
        }

        int captureDeviceIndex = 0;

        if(captureDeviceIndex == 0)
        {
            // use default (or first) camera
            recordingDeviceIndex = 0;
            error = ptrVEHardware->GetRecordingDeviceName(
                recordingDeviceIndex, recordingDeviceName,
                recordingDeviceUniqueName);

            break;
        }
        else if(captureDeviceIndex < 0
                || captureDeviceIndex > numberOfRecordingDevices)
        {
            // invalid selection
            continue;
        }
        else
        {
            recordingDeviceIndex = captureDeviceIndex - 1;
            error = ptrVEHardware->GetRecordingDeviceName(
                recordingDeviceIndex, recordingDeviceName,
                recordingDeviceUniqueName);

            break;
        }
    }

    int numberOfPlaybackDevices = -1;
    error = ptrVEHardware->GetNumOfPlayoutDevices(numberOfPlaybackDevices);

    while(1)
    {
        playbackDeviceIndex = -1;
        int captureIdx = 0;

        for (captureIdx = 0; captureIdx < numberOfPlaybackDevices;
             captureIdx++)
        {
            memset(playbackDeviceName, 0, KMaxDeviceNameLength);
            memset(playbackDeviceUniqueName, 0, KMaxDeviceNameLength);
            error = ptrVEHardware->GetPlayoutDeviceName(
                captureIdx, playbackDeviceName, playbackDeviceUniqueName);
        }

        int captureDeviceIndex = 0;

        if(captureDeviceIndex == 0)
        {
            // use default (or first) camera
            playbackDeviceIndex = 0;
            error = ptrVEHardware->GetPlayoutDeviceName(
                playbackDeviceIndex, playbackDeviceName,
                playbackDeviceUniqueName);
            return true;
        }
        else if(captureDeviceIndex < 0
                || captureDeviceIndex > numberOfPlaybackDevices)
        {
            // invalid selection
            continue;
        }
        else
        {
            playbackDeviceIndex = captureDeviceIndex - 1;
            error = ptrVEHardware->GetPlayoutDeviceName(
                playbackDeviceIndex, playbackDeviceName,
                playbackDeviceUniqueName);
            return true;
        }
    }
}

// general settings functions
bool ViEMediaTest::GetIPAddress(char* iIP)
{
    char oIP[16] = DEFAULT_SEND_IP;
    strcpy(iIP, oIP);
            return true;
}


// video settings functions
bool ViEMediaTest::GetVideoPorts(int* txPort, int* rxPort)
{
    std::string str;

    // set to default values
    *txPort = DEFAULT_VIDEO_PORT;
    *rxPort = DEFAULT_VIDEO_PORT;

  return true;
}

bool ViEMediaTest::GetVideoCodec(webrtc::ViECodec* ptrViECodec,
                                webrtc::VideoCodec& videoCodec)
{
	int error = 0;
    int codecSelection = 0;
    std::string str;
    memset(&videoCodec, 0, sizeof(webrtc::VideoCodec));
	
    bool exitLoop=false;
    while(!exitLoop)
    {
        int codecIdx = 0;
        int defaultCodecIdx = 0;
        for (codecIdx = 0; codecIdx < ptrViECodec->NumberOfCodecs(); codecIdx++)
        {
            error = ptrViECodec->GetCodec(codecIdx, videoCodec);
          	
            // test for default codec index
            if(strcmp(videoCodec.plName, DEFAULT_VIDEO_CODEC) == 0)
            {
                defaultCodecIdx = codecIdx;
            }
			
        }
        codecSelection = 0;
		
        if(codecSelection == 0)
        {
            // use default
            error = ptrViECodec->GetCodec(defaultCodecIdx, videoCodec);
            exitLoop=true;
        }
        else
        {
            // user selection
            codecSelection = atoi(str.c_str())-1;
            error = ptrViECodec->GetCodec(codecSelection, videoCodec);
            if(error != 0)
            {
                continue;
            }
            exitLoop=true;
		}
    }
	
    int sizeSelection = 0;
    if(sizeSelection!=0)
    {
        videoCodec.width=640;
    }
	
    sizeSelection = 0; 
    if(sizeSelection!=0)
    {
        videoCodec.height=480;
    }


        videoCodec.width=640;
        videoCodec.height=480;
    return true;
}


// audio settings functions
bool ViEMediaTest::GetAudioPorts(int* txPort, int* rxPort)
{

    // set to default values
    *txPort = DEFAULT_AUDIO_PORT;
    *rxPort = DEFAULT_AUDIO_PORT;
	
    return true;;
}

bool ViEMediaTest::GetAudioCodec(webrtc::VoECodec* ptrVeCodec, webrtc::CodecInst& audioCodec)
{
	int error = 0;
    int codecSelection = 0;
    std::string str;
    memset(&audioCodec, 0, sizeof(webrtc::CodecInst));
	
    while(1)
    {
        int codecIdx = 0;
        int defaultCodecIdx = 0;
        for (codecIdx = 0; codecIdx < ptrVeCodec->NumOfCodecs(); codecIdx++)
        {
            error = ptrVeCodec->GetCodec(codecIdx, audioCodec);
            
            // test for default codec index
            if(strcmp(audioCodec.plname, DEFAULT_AUDIO_CODEC) == 0)
            {
				defaultCodecIdx = codecIdx;
            }
        }
        codecSelection = 0;
		
        if(codecSelection == 0)
        {
            // use default
            error = ptrVeCodec->GetCodec(defaultCodecIdx, audioCodec);
            return true;
        }
        else
        {
            // user selection
            codecSelection = atoi(str.c_str())-1;
            error = ptrVeCodec->GetCodec(codecSelection, audioCodec);
            if(error != 0)
            {
                continue;
            }
            return true;
        }
    }
    assert(false);
    return false;
}

void ViEMediaTest::PrintCallInformation(char* IP, char* videoCaptureDeviceName,
                                       char* videoCaptureUniqueId,
                                       webrtc::VideoCodec videoCodec,
                                       int videoTxPort, int videoRxPort,
                                       char* audioCaptureDeviceName,
                                       char* audioPlaybackDeviceName,
                                       webrtc::CodecInst audioCodec,
                                       int audioTxPort, int audioRxPort)
{

}


