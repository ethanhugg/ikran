// GIPSVEBase.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPS_VE_BASE_H__)
#define __GIPS_VE_BASE_H__

#include "GIPSVECommon.h"

const int GIPS_DEFAULT = -1;

enum GIPS_NetEQModes		// NetEQ playout configurations
{
    NETEQ_DEFAULT = 0,		// Optimized trade-off between low delay and jitter robustness for two-way communication.
    NETEQ_STREAMING = 1,	// Improved jitter robustness at the cost of increased delay. Can be used in one-way communication.
	NETEQ_FAX = 2,			// Optimzed for decodability of fax signals rather than for perceived audio quality.
};

enum GIPS_NetEQBGNModes		// NetEQ Background Noise (BGN) configurations
{
    GIPS_BGN_ON = 0,		// BGN is always on and will be generated when the incoming RTP stream stops (default).
    GIPS_BGN_FADE = 1,	    // The BGN is faded to zero (complete silence) after a few seconds.
	GIPS_BGN_OFF = 2,		// BGN is not used at all. Silence is produced after speech extrapolation has faded.
};

enum GIPS_LinuxAudio		// type of Linux audio driver
{
    LINUX_AUDIO_OSS = 0,	// Open Sound System (OSS) audio driver
    LINUX_AUDIO_ALSA		// Advanced Linux Sound Architecture (ALSA) audio driver
};

enum GIPS_OnHoldModes		// On Hold direction
{
    HOLD_SEND_AND_PLAY = 0,	// Put both sending and playing in on-hold state.
    HOLD_SEND_ONLY,		    // Put only sending in on-hold state.
    HOLD_PLAY_ONLY          // Put only playing in on-hold state.
};

class VOICEENGINE_DLLEXPORT GIPSVoiceEngineObserver
{
public:
    virtual void CallbackOnError(const int errCode, const int channel) = 0;
protected:
    virtual ~GIPSVoiceEngineObserver() {}
};

// ----------------------------------------------------------------------------
//	GIPSVoiceEngine
// ----------------------------------------------------------------------------

class VOICEENGINE_DLLEXPORT GIPSVoiceEngine
{
public:
	// Resource allocation
    static GIPSVoiceEngine* Create();
	static GIPSVoiceEngine* Create(void* javaVM, void* context);
    static bool Delete(GIPSVoiceEngine*& voiceEngine, bool ignoreRefCounters = false);

	// Trace functions (internal trace implementation is a singleton)
	static int SetTraceFilter(const unsigned int filter);
    static int SetTraceFile(const char* fileNameUTF8, const bool addFileCounter = false); 
    static int SetEncryptedTraceFile(const char* fileNameUTF8, const bool addFileCounter = false); 
	static int SetTraceCallback(GIPSTraceCallback* callback);
protected:
    GIPSVoiceEngine();
    ~GIPSVoiceEngine();
};

// ----------------------------------------------------------------------------
//	GIPSVEBase
// ----------------------------------------------------------------------------

class VOICEENGINE_DLLEXPORT GIPSVEBase
{
public:
    static GIPSVEBase* GIPSVE_GetInterface(GIPSVoiceEngine* voiceEngine);

	// Error notification
    virtual int GIPSVE_SetObserver(GIPSVoiceEngineObserver& observer, bool clear = false) = 0;

    // Initialization and Termination functions
    virtual int GIPSVE_Authenticate(const char* key, unsigned int length) = 0;
    virtual int GIPSVE_Init(int month = 0, int day = 0, int year = 0, bool recordAEC = false, GIPS_LinuxAudio audiolib = LINUX_AUDIO_ALSA) = 0;
    virtual int GIPSVE_Terminate() = 0;

    // Channel functions
    virtual int GIPSVE_MaxNumOfChannels() = 0;
    virtual int GIPSVE_CreateChannel() = 0;
    virtual int GIPSVE_DeleteChannel(int channel) = 0;

    // Receiver & Destination functions
    virtual int GIPSVE_SetLocalReceiver(int channel, int port, int RTCPport = GIPS_DEFAULT, const char* ipaddr = NULL, const char* multiCastAddr = NULL) = 0;
    virtual int GIPSVE_GetLocalReceiver(int channel, int& port, int& RTCPport, char* ipaddr, unsigned int ipaddrLength) = 0;
    virtual int GIPSVE_SetSendDestination(int channel, int port, const char* ipaddr, int sourcePort = GIPS_DEFAULT, int RTCPport = GIPS_DEFAULT) = 0;
    virtual int GIPSVE_GetSendDestination(int channel, int& port, char* ipaddr, unsigned int ipaddrLength, int& sourcePort, int& RTCPport) = 0;

    // Media functions
    virtual int GIPSVE_StartListen(int channel) = 0;
    virtual int GIPSVE_StartPlayout(int channel) = 0;
    virtual int GIPSVE_StartSend(int channel) = 0;
    virtual int GIPSVE_StopListen(int channel) = 0;
    virtual int GIPSVE_StopPlayout(int channel) = 0;
    virtual int GIPSVE_StopSend(int channel) = 0;

    // Info functions
    virtual int GIPSVE_GetVersion(char* version, unsigned int length) = 0;
    virtual int GIPSVE_LastError() = 0;

	// NetEQ playout functions
	virtual int GIPSVE_SetNetEQPlayoutMode(int channel, GIPS_NetEQModes mode) = 0;
	virtual int GIPSVE_GetNetEQPlayoutMode(int channel, GIPS_NetEQModes& mode) = 0;

    // NetEQ Background Noise (BGN) functions
    virtual int GIPSVE_SetNetEQBGNMode(int channel, GIPS_NetEQBGNModes mode) = 0;
	virtual int GIPSVE_GetNetEQBGNMode(int channel, GIPS_NetEQBGNModes& mode) = 0;

    // Misc functions
    virtual int GIPSVE_SetConferenceStatus(int channel, bool enable, bool includeCSRCs = false, bool includeVoiceLevel = false) = 0;
    virtual int GIPSVE_PutOnHold(int channel, bool enable, GIPS_OnHoldModes mode = HOLD_SEND_AND_PLAY) = 0;

    virtual int GIPSVE_Release() = 0;
protected:
    virtual ~GIPSVEBase();
};

#endif    // !defined(__GIPS_VE_BASE_H__)
