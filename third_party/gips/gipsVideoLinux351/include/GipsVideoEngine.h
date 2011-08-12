/**
*   GipsVideoEngine.h
*   Created by: Patrik Westin
*   Date      : 2005-05-09
*
*   Public API for GIPS VideoEngine on a PC platform.
* 
*   Copyright (c) 2005-2008
*   Global IP Sound AB, Organization number: 5565739017
*   Ölandsgatan 42, SE-116 63 Stockholm, Sweden
*   All rights reserved.
*   
**/

#ifndef PUBLIC_GIPS_VIDEO_ENGINE_H
#define PUBLIC_GIPS_VIDEO_ENGINE_H

#ifdef GIPS_EXPORT
#define VIDEOENGINE_DLLEXPORT _declspec(dllexport)
#elif GIPS_DLL
#define VIDEOENGINE_DLLEXPORT _declspec(dllimport)
#else
#define VIDEOENGINE_DLLEXPORT
#endif

#include "GIPS_common_types.h"
#include "GIPS_common_video_types.h"
#include "GIPSVideoCodecTypes.h"

// forward decalaration
class GIPSVoiceEngine;

#ifndef NULL
    #define NULL 0
#endif

class GIPSVideoCallback
{
public:
    virtual void PerformanceAlarm(int value) = 0;
    virtual void BrightnessAlarm(int value) = 0;
    virtual void LocalFrameRate(int frameRate) = 0; 
    virtual void MotionUpdate(unsigned char value) = 0;
    // same callback method is being used to raise also to clear.
    // true - raise, false - clear
    virtual void NoPictureAlarm(bool active = true) = 0;

    virtual ~GIPSVideoCallback() {};
};

class GIPSVideoChannelCallback
{
public:
    virtual void IncomingRate(int channel, int frameRate, int bitrate) = 0;
    virtual void IncomingCodecChanged(int channel, int payloadType, int width, int height) = 0;
    virtual void IncomingCSRCChanged(int channel, unsigned int csrc, bool added) = 0;    
    virtual void RequestNewKeyFrame(int channel) = 0;  
    virtual void SendRate(int channel, int frameRate, int bitrate) = 0;
    virtual ~GIPSVideoChannelCallback() {};
};

class GIPSEffectFilter
{
public:
    virtual int Transform(int size, unsigned char* frameBuffer, unsigned int timeStamp90KHz) = 0;
    virtual ~GIPSEffectFilter() {};
};

class GIPSVideoRenderCallback
{
public:
    virtual int FrameSizeChange(int width, int height, int numberOfStreams) = 0; 
    virtual int DeliverFrame(unsigned char* buffer, int bufferSize, unsigned int timeStamp90KHz) = 0;
    virtual ~GIPSVideoRenderCallback() {};
};

struct GIPSCameraCapability
{
    int width;
    int height;
    int maxFPS;
    GIPSVideoType type;
};

struct GIPSVideo_Picture
{
	unsigned char*	data;
	unsigned int	size;
	unsigned int	width;
	unsigned int	height;
	GIPSVideoType	type;
    
    GIPSVideo_Picture()
    {
        data = NULL;
        size = 0;
        width = 0;
        height = 0;
        type = GIPS_UNKNOWN;
    }
};

enum GIPSRTCPMode
{
    GIPS_RTCP_NONE     = 0,
    GIPS_RTCP_COMPOUND_RFC4585     = 1,
    GIPS_RTCP_NON_COMPOUND_RFC5506 = 2  
};


#define GIPSTraceCallbackObject void*
typedef void (*GIPSTraceCallbackFunction)(GIPSTraceCallbackObject, char *, int);

class VIDEOENGINE_DLLEXPORT GipsVideoEngine
{
public:
    virtual int GIPSVideo_Authenticate(char *auth_string, int len) = 0;
    virtual int GIPSVideo_Init(GIPSVoiceEngine* obj, int month = 0, int day = 0, int year = 0) = 0; 
    virtual int GIPSVideo_InitThreadContext() = 0; 
    virtual int GIPSVideo_SetVoiceEngine(GIPSVoiceEngine* ) = 0; 
    virtual int GIPSVideo_Terminate() = 0;
    virtual int GIPSVideo_GetLastError() = 0;
    virtual int GIPSVideo_GetVersion(char *version, int buflen) = 0;

    // start / stop the engine
    virtual int GIPSVideo_Run() = 0;    
    virtual int GIPSVideo_Stop() = 0;   

    // Channels
    virtual int GIPSVideo_GetNoOfChannels() = 0;
    virtual int GIPSVideo_CreateChannel(int audioChannel = -1) = 0;
    virtual int GIPSVideo_DeleteChannel(int videoChannel) = 0;
    virtual int GIPSVideo_SetAudioChannel(int videoChannel, int audioChannel) = 0;

    // Conferencing
    virtual int GIPSVideo_Conferencing(int channel, bool enable) = 0;
    virtual int GIPSVideo_ConferenceDemuxing(int channel, bool enable) = 0;
    virtual int GIPSVideo_SetLayout(GIPSVideoLayouts layout) = 0;
    virtual int GIPSVideo_GetRenderStreamID(int channel, unsigned int csrc) = 0;

    // Codecs
    virtual int GIPSVideo_GetCodec(int listnr, GIPSVideo_CodecInst *codec_inst) = 0;
    virtual int GIPSVideo_GetNofCodecs() = 0;
    virtual int GIPSVideo_SetSendCodec(int channel, GIPSVideo_CodecInst *codec_inst, bool def) = 0;
    virtual int GIPSVideo_GetSendCodec(int channel,GIPSVideo_CodecInst *codec_inst) = 0;
    virtual int GIPSVideo_SetReceiveCodec(int channel, GIPSVideo_CodecInst *codec_inst, bool force = false) = 0;
    virtual int GIPSVideo_GetReceiveCodec(int channel, GIPSVideo_CodecInst *codec_inst) = 0;

	// Traffic shaping
	virtual int GIPSVideo_EnablePacketLossBitrateAdaption(int channel, bool enable) = 0;

	// Force a key frame
	virtual int GIPSVideo_SendKeyFrame(int channel) = 0;
    
	// H263 
	virtual int GIPSVideo_SetInverseH263Logic(int channel, bool enable) = 0;

	// LSVX
    virtual int GIPSVideo_EnableRemoteResize(int channel, bool enable) = 0;

    // Capture device
    virtual int GIPSVideo_GetCaptureDevice(int listNr, char* deviceName, int size) = 0;  
    virtual int GIPSVideo_SetCaptureDevice(const char* deviceName, int size) = 0; 
    virtual int GIPSVideo_SetCaptureDelay(int cameraDelay)=0;
    virtual int GIPSVideo_GetCaptureCapabilities(int listNr, GIPSCameraCapability* ) = 0;
    virtual int GIPSVideo_GetCaptureCapabilities(const char* deviceName, int length, int listNr, GIPSCameraCapability* ) = 0;
    virtual int GIPSVideo_GetCaptureDeviceId(int listNr, char* deviceNameUTF8, int sizeDeviceName, char* uniqueIdUTF8, int sizeUniqueId) = 0;
    virtual int GIPSVideo_SetCaptureDeviceId(const char* uniqueIdUTF8, int sizeUniqueId) = 0; 

    // Start/Stop
    virtual int GIPSVideo_StartRender(int channel) = 0;
    virtual int GIPSVideo_StartSend(int channel) = 0;
    virtual int GIPSVideo_StopRender(int channel) = 0;
    virtual int GIPSVideo_StopSend(int channel) = 0;

    // Record the played video
    virtual int GIPSVideo_StartRecording(int channel, const char* fileName, GIPSVideo_CodecInst *inst = NULL, int file_format = FILE_FORMAT_AVI_FILE, bool wide_band = true) = 0;
    virtual int GIPSVideo_StopRecording(int channel) = 0;

    // Record the local video
    virtual int GIPSVideo_StartRecordingCamera(const char* fileName,GIPSVideo_CodecInst *codec_inst, bool wide_band = true) = 0;
    virtual int GIPSVideo_StopRecordingCamera() = 0;

    // Record outgoing video
    virtual int GIPSVideo_StartRecording(const char* fileName, GIPSVideo_CodecInst *inst = NULL, int file_format = FILE_FORMAT_AVI_FILE, bool wide_band = true) = 0;
    virtual int GIPSVideo_StopRecording() = 0;

    // Play video file. 
    virtual int GIPSVideo_StartPlayFile(int channel, const char * fileName, bool loop = false, int file_format = FILE_FORMAT_AVI_FILE, bool tryToSendPreEncoded = false ) = 0;
    virtual bool GIPSVideo_IsPlayingFile(int channel) = 0;
    virtual int GIPSVideo_StopPlayingFile(int channel) = 0;

    // Play video file as camera
    virtual int GIPSVideo_StartPlayFileAsCamera(const char * fileName, bool loop = false ,int file_format = FILE_FORMAT_AVI_FILE) = 0;
    virtual bool GIPSVideo_IsPlayingFileAsCamera() = 0;
    virtual int GIPSVideo_StopPlayingFileAsCamera() = 0;

	// File duration
	virtual int GIPSVideo_GetFileInfo(const char* fileName, int& durationMs, int file_format, GIPSVideo_CodecInst *codecInst = NULL) =0;
    
    // File conversion
    virtual int GIPSVideo_ConvertGMFToAVI(const char* fileNameGMF, const char* fileNameAVI, GIPSVideo_CodecInst *codec_inst) = 0;

    // Snapshot from local video in JPEG
    virtual int GIPSVideo_GetSnapshotCamera(const char* fileName) = 0;

    // Snapshot from played video in JPEG
    virtual int GIPSVideo_GetSnapshot(int channel, const char* fileName) = 0;

    virtual int GIPSVideo_GetSnapshotCamera(GIPSVideo_Picture& picture) = 0;
    virtual int GIPSVideo_GetSnapshot(int channel, GIPSVideo_Picture& picture) = 0;
    virtual int GIPSVideo_FreePicture(GIPSVideo_Picture& picture) = 0;

    // Set background image in JPEG
    virtual int GIPSVideo_SetBackgroundImage(int channel, const char* fileName, unsigned int timeInMS = 0) = 0;
    virtual int GIPSVideo_SetBackgroundImage(int channel, const GIPSVideo_Picture& picture, unsigned int timeInMS = 0) = 0;

    // Picture enhancements
    virtual int GIPSVideo_EnableDeflickering(bool enable) = 0;
    virtual int GIPSVideo_EnableDenoising(bool enable) = 0;
    virtual int GIPSVideo_EnableColorEnhancement(int channel, bool enable) = 0;
    virtual int GIPSVideo_EnableFrameUpScale(bool enable) = 0;
    virtual int GIPSVideo_EnableInterpolateScaling(bool enable) = 0;

    // IP/UDP
    virtual int GIPSVideo_SetLocalReceiver(int channel, unsigned short portnr, char ip[64] = NULL,unsigned short rtcpPortnr = 0, char multiCastAddr[64] = NULL) = 0;
    virtual int GIPSVideo_GetLocalReceiver(int channel, unsigned short& rtpPport, unsigned short& rctpPport, char ip[64]) = 0;
    virtual int GIPSVideo_SetSendDestination(int channel, unsigned short portnr, char ipadr[64], unsigned short rtcpPortnr = 0) = 0;
    virtual int GIPSVideo_GetSendDestination(int channel, unsigned short& rtpPport, unsigned short& rctpPport, char ipadr[64]) = 0;
    // IPV6
    virtual int GIPSVideo_EnableIPv6(int channel) = 0;    

    virtual int GIPSVideo_SetSendTOS(int channel, int TOS, bool useSetSockopt = false) = 0;
    virtual int GIPSVideo_GetSendTOS(int channel, int& TOS, bool& useSetSockopt) = 0;
    virtual int GIPSVideo_SetSendGQOS(int channel, bool enable, int servicetype, int overrideDSCP = 0) = 0;
    virtual int GIPSVideo_GetSendGQOS(int channel, bool& enable, int& servicetype, int& overrideDSCP) = 0;

    virtual int GIPSVideo_GetFromPort(int channel, unsigned short& rtpPort, unsigned short& rtcpPort)=0;
    virtual int GIPSVideo_SetSrcPort(int channel, unsigned short portnr,unsigned short rtcpPortnr = 0) = 0;

    virtual int GIPSVideo_SetMTU(int channel, int mtu) = 0;
    virtual int GIPSVideo_SetMaxPacketBurstSize(int channel, int maxNumberOfPackets) = 0; 

    virtual int GIPSVideo_SetSendSSRC(int channel, unsigned long int ssrc) = 0;
    virtual int GIPSVideo_GetSendSSRC(int channel, unsigned long int &ssrc) = 0;

    virtual int GIPSVideo_SetSendRTPSequenceNumber(int channel, unsigned short seqNum) = 0;

    virtual int GIPSVideo_SetRTPKeepaliveStatus(int channel, bool enable, int unknownPayloadType, int deltaTransmitTimeSeconds = 15) = 0;
    virtual int GIPSVideo_GetRTPKeepaliveStatus(int channel, bool& enabled, int& unknownPayloadType, int& deltaTransmitTimeSeconds) = 0;

   	// Send extra packet over RTP / RTCP channel (no RTP headers added)
    virtual int GIPSVideo_SendExtraRTPPacket(int channel, const char* data, unsigned int length, unsigned short portnr = 0, const char ip[64] = NULL) = 0;
    virtual int GIPSVideo_SendExtraRTCPPacket(int channel, const char* data, unsigned int length, unsigned short portnr = 0, const char ip[64] = NULL) = 0;

	// RTP signaling (RFC 2032)
	virtual int GIPSVideo_EnableIntraRequest(int channel, bool enable) = 0;

    // RTCP
    virtual int GIPSVideo_EnableRTCP(int channel, GIPSRTCPMode rtcpMode) = 0;
    virtual int GIPSVideo_SetRTCPCNAME(int channel, char str[256]) = 0;
    virtual int GIPSVideo_GetRemoteRTCPCNAME(int channel, char str[256]) = 0;
    virtual int GIPSVideo_RTCPStat(int channel, 
                                   GIPSVideo_CallStatistics *incomingStats,
                                   GIPSVideo_CallStatistics *outgoingStats) =0;

	// RTCP signaling (RFC 4585)
	virtual int GIPSVideo_EnableNACK(int channel, bool enable) = 0;
    virtual int GIPSVideo_EnablePLI(int channel, bool enable) = 0;
        
    //FEC as described in RFC 5109
    virtual int GIPSVideo_EnableFEC(int channel, bool enable, unsigned char payloadTypeRED, unsigned char payloadTypeFEC) = 0;

	// RTCP signaling (RFC 5104)
    virtual int GIPSVideo_EnableTMMBR(int channel, bool enable) = 0;

    // Packet loss signaling
    virtual int GIPSVideo_EnableSignalKeyPacketLoss(int channel, bool enable) = 0;

    // STATISTICS
    virtual int GIPSVideo_GetUpdate(int channel, int* frameRate, int* bitrate) = 0;
    virtual int GIPSVideo_GetLocalUpdate(int channel, int* frameRate, int* bitrate) = 0;
    virtual int GIPSVideo_GetFrameStatistics(int channel, GIPSVideo_FrameStatistics* inst) = 0;

    virtual int GIPSVideo_GetAvailableBandwidth(int channel, int& bandwidth) = 0;

    // Trace
    virtual int GIPSVideo_SetTraceFilter(unsigned int filter) = 0;
    virtual int GIPSVideo_SetTraceFileName(char* fileName) = 0; 
    virtual int GIPSVideo_SetDebugTraceFileName(char* fileName) = 0; 
    virtual int GIPSVideo_SetTraceCallback(GIPSTraceCallback* obj) = 0;

    // Callbacks
    virtual int GIPSVideo_SetCallback(GIPSVideoCallback* obj) = 0; 
    virtual int GIPSVideo_SetChannelCallback(int channel, GIPSVideoChannelCallback* obj) = 0;
    virtual int GIPSVideo_EnableBrightnessAlarm(bool enable) = 0;
    virtual int GIPSVideo_EnableMotionUpdate(bool enable) = 0;
    virtual int GIPSVideo_EnableKeyFrameRequestCallback(bool enable) = 0;

    virtual int GIPSVideo_RegisterRenderCallback(int channel, GIPSEffectFilter* obj) = 0;
    virtual int GIPSVideo_RegisterSendCallback(int channel, GIPSEffectFilter* obj) = 0;
    virtual int GIPSVideo_RegisterCaptureCallback( GIPSEffectFilter* obj) = 0;

    // External capture device
    virtual int GIPSVideo_IncomingCapturedFrame(GIPSVideoType incomingVideoType, unsigned char* incomingFrame, int width, int height,int bufferLength=0,unsigned long timeStamp=0) = 0;

    // External rendering
    virtual int GIPSVideo_AddLocalRenderer(GIPSVideoType videoFormat, GIPSVideoRenderCallback* obj) = 0;
    virtual int GIPSVideo_AddRemoteRenderer(int channel, GIPSVideoType videoFormat, GIPSVideoRenderCallback* obj, int renderWidth = 0, int renderHeight = 0) = 0;

    // External Transport
    // Use these function calls ONLY when a customer specific transport protocol is going to be used
    virtual int GIPSVideo_SetSendTransport(int channel, GIPS_transport* transport) = 0;
    virtual int GIPSVideo_ReceivedRTPPacket(int channel, const char* data, int length) = 0;
    virtual int GIPSVideo_ReceivedRTCPPacket(int channel, const char* data, int length) = 0;

    // SRTP 
    virtual int GIPSVideo_EnableSRTPSend(int channel,int cipher_type,int cipher_key_len,int auth_type, int auth_key_len,int auth_tag_len, int security, const unsigned char* key, bool applyToRTCP) = 0;
    virtual int GIPSVideo_DisableSRTPSend(int channel) = 0;
    virtual int GIPSVideo_EnableSRTPReceive(int channel,int cipher_type,int cipher_key_len,int auth_type, int auth_key_len,int auth_tag_len, int security, const unsigned char* key,bool applyToRTCP) = 0;
    virtual int GIPSVideo_DisableSRTPReceive(int channel) = 0;

    // External Encryption
    virtual int GIPSVideo_InitEncryption (GIPS_encryption* encr_obj) = 0;
    virtual int GIPSVideo_EnableEncryption(int channel,bool applyToRTCP) = 0;
    virtual int GIPSVideo_DisableEncryption(int channel) = 0;

	// Store RTP and RTCP packets and dump to file (compatible with rtpplay)
	virtual int GIPSVideo_StartRTPDump(int channel, const char* fileNameUTF8, bool inPacket) = 0;
	virtual int GIPSVideo_StopRTPDump(int channel, bool inPacket) = 0;
	virtual int GIPSVideo_RTPDumpIsActive(int channel, bool inPacket) = 0;

    // performace
    virtual int GIPSVideo_SetThresholdToSignalRemoteResize(int cpuThreshold, int renderWidth, int renderHeight) = 0;

    // extra effects
    virtual int GIPSVideo_MirrorLocalPreview(bool enable) = 0;

    virtual ~GipsVideoEngine();
};

#endif



