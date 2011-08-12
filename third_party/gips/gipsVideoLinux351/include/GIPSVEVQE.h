// GIPSVEVQE.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_VQE_H__)
#define __GIPSVE_VQE_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

class VOICEENGINE_DLLEXPORT GIPSVERxVadCallback
{
public:
    virtual void OnRxVad(int channel, int vadDecision) = 0;
protected:
    virtual ~GIPSVERxVadCallback() { } 
};


enum GIPS_NSmodes                   // type of Noise Suppression
{
	NS_UNCHANGED = 0,				// previously set mode
    NS_DEFAULT,                     // platform default
	NS_CONFERENCE,					// conferencing default
    NS_LOW_SUPPRESSION,             // lowest suppression
    NS_MODERATE_SUPPRESSION,
    NS_HIGH_SUPPRESSION,
    NS_VERY_HIGH_SUPPRESSION,       // highest suppression
};

enum GIPS_AGCmodes                  // type of Automatic Gain Control
{   
	AGC_UNCHANGED = 0,			    // previously set mode
    AGC_DEFAULT,                    // platform default
    AGC_ADAPTIVE_ANALOG,			// adaptive mode for use when analog volume control exists (e.g. for PC softphone)
	AGC_ADAPTIVE_DIGITAL,			// scaling takes place in the digital domain (e.g. for conference servers and embedded devices)
	AGC_FIXED_DIGITAL				// can be used on embedded devices where the the capture signal is level is predictable
};

enum GIPS_ECmodes                   // type of Echo Control
{
	EC_UNCHANGED = 0,			    // previously set mode
	EC_DEFAULT,                     // platform default
	EC_CONFERENCE,				    // conferencing default (aggressive AEC)
    EC_AEC,                         // Acoustic Echo Cancellation
    EC_AES,                         // Acoustic Echo Suppression
    EC_AECM,                        // AEC mobile
    EC_NEC_IAD                      // Network Echo Cancellation (VoiceEngine ATA only)
};

enum GIPS_AESmodes                  // type of AES
{
    AES_DEFAULT = 0,				// platform default
    AES_NORMAL,                     // normal
    AES_HIGH,                       // high (more aggressive)
    AES_ATTENUATE,                  // attenuate (less aggressive)
    AES_NORMAL_SOFT_TRANS,          // normal with soft transition switching
    AES_HIGH_SOFT_TRANS,            // high with soft transition switching
    AES_ATTENUATE_SOFT_TRANS        // attenuate with soft transition switching 
};

typedef struct
{
    unsigned short targetLeveldBOv;
    unsigned short digitalCompressionGaindB;
    bool           limiterEnable;            
} GIPS_AGC_config;					// AGC configuration parameters

class VOICEENGINE_DLLEXPORT GIPSVEVQE
{
public:
    static GIPSVEVQE* GetInterface(GIPSVoiceEngine* voiceEngine);

    // Noise Suppression (NS)
    virtual int GIPSVE_SetNSStatus(bool enable, GIPS_NSmodes mode = NS_UNCHANGED) = 0;
    virtual int GIPSVE_GetNSStatus(bool& enabled, GIPS_NSmodes& mode) = 0;

    // Automatic Gain Control (AGC)
    virtual int GIPSVE_SetAGCStatus(bool enable, GIPS_AGCmodes mode = AGC_UNCHANGED) = 0;
    virtual int GIPSVE_GetAGCStatus(bool& enabled, GIPS_AGCmodes& mode) = 0;
	virtual int GIPSVE_SetAGCConfig(const GIPS_AGC_config config) = 0;
	virtual int GIPSVE_GetAGCConfig(GIPS_AGC_config& config) = 0;

    // Echo Control (EC)
    virtual int GIPSVE_SetECStatus(bool enable, GIPS_ECmodes mode = EC_UNCHANGED, GIPS_AESmodes AESmode = AES_DEFAULT, int AESattn = 28) = 0;
    virtual int GIPSVE_GetECStatus(bool& enabled, GIPS_ECmodes& mode, GIPS_AESmodes& AESmode, int& AESattn) = 0;

    // VAD/DTX indication
    virtual int GIPSVE_InitRxVad(GIPSVERxVadCallback* rxVadCallback) = 0;
    virtual int GIPSVE_VoiceActivityIndicator(int channel) = 0;

	// Instantaneous Speech, Noise and Echo metrics
	virtual int GIPSVE_SetMetricsStatus(bool enable) = 0;
	virtual int GIPSVE_GetMetricsStatus(bool& enabled) = 0;
	virtual int GIPSVE_GetSpeechMetrics(int& levelTx, int& levelRx) = 0;
	virtual int GIPSVE_GetNoiseMetrics(int& levelTx, int& levelRx) = 0;
	virtual int GIPSVE_GetEchoMetrics(int& ERL, int& ERLE, int& RERL, int& A_NLP) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVEVQE();
};

#endif    // #if !defined(__GIPSVE_VQE_H__)
