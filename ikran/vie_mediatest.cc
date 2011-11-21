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
#include "CSFLogStream.h"

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
  CSFLogDebugS(logTag, " Init Media Engine ");

  // obtain all the interfaces
  int error = 0;
  std::string str;
	
  // VoE
  ptrVE = webrtc::VoiceEngine::Create();
	
  ptrVEBase = webrtc::VoEBase::GetInterface(ptrVE);
	
  error = ptrVEBase->Init();
 
  CSFLogDebugS(logTag, "VoEBase::Init() error: " << error); 
	
  ptrVECodec = webrtc::VoECodec::GetInterface(ptrVE);
	
  ptrVEHardware = webrtc::VoEHardware::GetInterface(ptrVE);
	
  ptrVEAPM = webrtc::VoEAudioProcessing::GetInterface(ptrVE);
  CSFLogDebugS(logTag, " Voice Engine Init'ed ");		
    
  // ViE
  ptrViE = webrtc::VideoEngine::Create();
	
  ptrViEBase = webrtc::ViEBase::GetInterface(ptrViE);
	
  error = ptrViEBase->Init();

  CSFLogDebugS(logTag, "ViEBase::Init() error: " << error);

  ptrViECapture = webrtc::ViECapture::GetInterface(ptrViE);
	
  ptrViERender = webrtc::ViERender::GetInterface(ptrViE);
	
  ptrViECodec = webrtc::ViECodec::GetInterface(ptrViE);
	
  ptrViENetwork = webrtc::ViENetwork::GetInterface(ptrViE);
		
  CSFLogDebugS(logTag, " Video Engine Init'ed ");		
	
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
    CSFLogDebugS(logTag, " StartMedia ");
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
	

	std::cout << "\tIP Address of the peer is " << ipAddress << endl;	
	std::cout << "\tVideo TxPort: " << videoTxPort <<"Video RxPort: " << videoRxPort<< endl;	
	std::cout << "\tAudio TxPort: " << audioTxPort <<"Audio RxPort: " << audioRxPort<< endl;	

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

	    std::cout << "Allocating the capture device " << endl;
		std::cout << " StartCall: Configuing VIE" << endl;
		captureId = 0;
		error = ptrViECapture->AllocateCaptureDevice(uniqueId,
													 KMaxUniqueIdLength,
													 captureId);
	    std::cout << " Allocate capture device error: " << error << endl;
        error = ptrViECapture->ConnectCaptureDevice(captureId, videoChannel);
	    std::cout << " Starting the capture " << endl;
        error = ptrViECapture->StartCapture(captureId);
	    ptrViERtpRtcp =
		webrtc::ViERTP_RTCP::GetInterface(ptrViE);
        error = ptrViERtpRtcp->SetRTCPStatus(videoChannel,
                                             webrtc::kRtcpCompound_RFC4585);
        error = ptrViERtpRtcp->SetKeyFrameRequestMethod(
											 videoChannel, webrtc::kViEKeyFrameRequestPliRtcp);
	    std::cout << " StartCall: Started Capture" << videoChannel <<  endl;
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
        std::cout << " Adding the renderer " << ext_renderer << endl;
		error = ptrViERender->AddRenderer(videoChannel, (webrtc::RawVideoType)0,
											(webrtc::ExternalRenderer*) ext_renderer);

        std::cout << " Added the renderer: " << ext_renderer << " error " << error << endl ;	
        error = ptrViENetwork->SetSendDestination(videoChannel, ipAddress,
                                                  9000);

        error = ptrViENetwork->SetLocalReceiver(videoChannel, 9000);

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
	 std::cout << " STARTING THE ENGINES NOW " << endl;
        // VE first
        error = ptrVEBase->StartReceive(audioChannel);

        error = ptrVEBase->StartPlayout(audioChannel);

        error = ptrVEBase->StartSend(audioChannel);


        // ViE next
        error = ptrViEBase->StartSend(videoChannel);
		std::cout << " Start Send: error " << error << endl;

        error = ptrViEBase->StartReceive(videoChannel);
		std::cout << " Start Recieve: error " << error << endl;

        error = ptrViERender->StartRender(videoChannel);
		std::cout << " Start Render : error " << error << endl;
       
    }
	std::cout << " CALL SHOULD BE UP NOW ";
    return 0;
}


void ViEMediaTest::StopMedia()
{
  CSFLogDebugS(logTag, " Stop Media Invoked ");
  int error = 0;
	
  // audio engine first
  error = ptrVEBase->StopReceive(audioChannel);
  CSFLogDebugS(logTag, "StopReceive error: " << error);
	
  error = ptrVEBase->StopPlayout(audioChannel);
  CSFLogDebugS(logTag, "StopPlayout error: " << error);
	
  error = ptrVEBase->DeleteChannel(audioChannel);
  CSFLogDebugS(logTag, "DeleteChannel error: " << error);
	
  // now do video
  error = ptrViEBase->DisconnectAudioChannel(videoChannel);
  CSFLogDebugS(logTag, "DisconnectAudioChannel error: " << error);
	
  error = ptrViEBase->StopReceive(videoChannel);
  CSFLogDebugS(logTag, "StopReceive error: " << error);
	
  error = ptrViEBase->StopSend(videoChannel);
  CSFLogDebugS(logTag, "StopSend error: " << error);
	
  error = ptrViERender->StopRender(captureId);
  CSFLogDebugS(logTag, "StopRender error: " << error);
	
  error = ptrViERender->StopRender(videoChannel);
  CSFLogDebugS(logTag, "StopRender error: " << error);
	
  error = ptrViERender->RemoveRenderer(captureId);
  CSFLogDebugS(logTag, "RemoveRender error: " << error);
	
  error = ptrViERender->RemoveRenderer(videoChannel);
  CSFLogDebugS(logTag, "RemoveRender error: " << error);
	
  error = ptrViECapture->StopCapture(captureId);
  CSFLogDebugS(logTag, "StopCapture error: " << error);
	
  error = ptrViECapture->DisconnectCaptureDevice(videoChannel);
  CSFLogDebugS(logTag, "DisconnectCaptureDevice error: " << error);
	
  error = ptrViECapture->ReleaseCaptureDevice(captureId);
  CSFLogDebugS(logTag, "ReleaseCaptureDevice error: " << error);
	
  error = ptrViEBase->DeleteChannel(videoChannel);
  CSFLogDebugS(logTag, "DeleteChannel error: " << error);
	
  int remainingInterfaces = 0;
  remainingInterfaces = ptrViECodec->Release();
	
  remainingInterfaces = ptrViECapture->Release();
	
  remainingInterfaces = ptrViERtpRtcp->Release();
	
  remainingInterfaces = ptrViERender->Release();
	
  remainingInterfaces = ptrViENetwork->Release();
	
  remainingInterfaces = ptrViEBase->Release();
  CSFLogDebugS(logTag, "Call Teardown complete remaining interfaces: " << remainingInterfaces);
	
  webrtc::VideoEngine::Delete(ptrViE);
	
  std::cout << " CALL TEAR DOWN DONE NOW ";
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

        std::cout << std::endl;
        std::cout << "Available capture devices:" << std::endl;
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
            CSFLogDebugS(logTag, "GetCaptureDevice error: " << error);

            std::cout << "   " << captureIdx+1 << ". " << deviceName
                      << std::endl;
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
            CSFLogDebugS(logTag, "GetCaptureDevice error: " << error);
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
    CSFLogDebugS(logTag, "GetNumOfRecordingDevices error: " << error);

    while(1)
    {
        recordingDeviceIndex = -1;
        std::cout << std::endl;
        std::cout << "Available audio capture devices:" << std::endl;
        int captureIdx = 0;

        for (captureIdx = 0; captureIdx < numberOfRecordingDevices;
             captureIdx++)
        {
            memset(recordingDeviceName, 0, KMaxDeviceNameLength);
            memset(recordingDeviceUniqueName, 0, KMaxDeviceNameLength);
            error = ptrVEHardware->GetRecordingDeviceName(
                captureIdx, recordingDeviceName, recordingDeviceUniqueName);
            std::cout << "   " << captureIdx+1 << ". " << recordingDeviceName
                      << std::endl;
        }

        int captureDeviceIndex = 0;

        if(captureDeviceIndex == 0)
        {
            // use default (or first) camera
            recordingDeviceIndex = 0;
            error = ptrVEHardware->GetRecordingDeviceName(
                recordingDeviceIndex, recordingDeviceName,
                recordingDeviceUniqueName);
            CSFLogDebugS(logTag, "GetRecordingDeviceName error: " << error);

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
            CSFLogDebugS(logTag, "GetRecordingDeviceName error: " << error);

            break;
        }
    }

    int numberOfPlaybackDevices = -1;
    error = ptrVEHardware->GetNumOfPlayoutDevices(numberOfPlaybackDevices);
    CSFLogDebugS(logTag, "GetNumOfPlayoutDevices error: " << error);

    while(1)
    {
        playbackDeviceIndex = -1;
        std::cout << std::endl;
        std::cout << "Available audio playout devices:" << std::endl;
        int captureIdx = 0;

        for (captureIdx = 0; captureIdx < numberOfPlaybackDevices;
             captureIdx++)
        {
            memset(playbackDeviceName, 0, KMaxDeviceNameLength);
            memset(playbackDeviceUniqueName, 0, KMaxDeviceNameLength);
            error = ptrVEHardware->GetPlayoutDeviceName(
                captureIdx, playbackDeviceName, playbackDeviceUniqueName);
            CSFLogDebugS(logTag, "GetPlayoutDeviceName error: " << error);
            std::cout << "   " << captureIdx+1 << ". " << playbackDeviceName
                      << std::endl;
        }

        int captureDeviceIndex = 0;

        if(captureDeviceIndex == 0)
        {
            // use default (or first) camera
            playbackDeviceIndex = 0;
            error = ptrVEHardware->GetPlayoutDeviceName(
                playbackDeviceIndex, playbackDeviceName,
                playbackDeviceUniqueName);
            CSFLogDebugS(logTag, "GetPlayoutDeviceName error: " << error);
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
            CSFLogDebugS(logTag, "GetPlayoutDeviceName error: " << error);
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
	std::cout<< "\t default video tx port " << *txPort << endl;
	std::cout<< "\t default video rx port " << *rxPort << endl;

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
        std::cout << std::endl;
        std::cout << "Available video codecs:" << std::endl;
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
			
            std::cout << "   " << codecIdx+1 << ". " << videoCodec.plName
			<< std::endl;
        }
        std::cout << std::endl;
        std::cout << "Choose video codec. Press enter for default ("
		<< DEFAULT_VIDEO_CODEC << ")";
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
                std::cout << "ERROR: Code=" << error << " Invalid selection"
				<< std::endl;
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
	 std::cout << "\t default tx audio port: " << *txPort<< std::endl;
	 std::cout << "\t default rx audio port: " << *rxPort<< std::endl;
	
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
        std::cout << std::endl;
        std::cout << "Available audio codecs:" << std::endl;
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
            std::cout << "   " << codecIdx+1 << ". " << audioCodec.plname
			<< std::endl;
        }
        std::cout << std::endl;
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
                std::cout << "ERROR: Code = " << error << " Invalid selection"
				<< std::endl;
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
    std::string str;

    std::cout << "************************************************"
              << std::endl;
    std::cout << "The call will use the following settings: " << std::endl;
    std::cout << "\tIP: " << IP << std::endl;
    std::cout << "\tVideo Capture Device: " << videoCaptureDeviceName
              << std::endl;
    std::cout << "\t\tName: " << videoCaptureDeviceName << std::endl;
    std::cout << "\t\tUniqueId: " << videoCaptureUniqueId << std::endl;
    std::cout << "\tVideo Codec: " << std::endl;
    std::cout << "\t\tplName: " << videoCodec.plName << std::endl;
    std::cout << "\t\tplType: " << (int)videoCodec.plType << std::endl;
    std::cout << "\t\twidth: " << videoCodec.width << std::endl;
    std::cout << "\t\theight: " << videoCodec.height << std::endl;
    std::cout << "\t Video Tx Port: " << videoTxPort << std::endl;
    std::cout << "\t Video Rx Port: " << videoRxPort << std::endl;
    std::cout << "\tAudio Capture Device: " << audioCaptureDeviceName
              << std::endl;
    std::cout << "\tAudio Playback Device: " << audioPlaybackDeviceName
              << std::endl;
    std::cout << "\tAudio Codec: " << std::endl;
    std::cout << "\t\tplname: " << audioCodec.plname << std::endl;
    std::cout << "\t\tpltype: " << (int)audioCodec.pltype << std::endl;
    std::cout << "\t Audio Tx Port: " << audioTxPort << std::endl;
    std::cout << "\t Audio Rx Port: " << audioRxPort << std::endl;
    std::cout << "************************************************"
              << std::endl;
}


