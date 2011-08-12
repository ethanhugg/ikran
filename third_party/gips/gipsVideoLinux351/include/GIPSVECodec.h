// GIPSVECodec.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_CODEC_H__)
#define __GIPSVE_CODEC_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

enum GIPS_AMRmodes
{
    AMR_RFC3267_BWEFFICIENT = 0,
    AMR_RFC3267_OCTETALIGNED,
    AMR_RFC3267_FILESTORAGE
};

enum GIPS_PayloadFrequencies 
{
    FREQ_8000_HZ = 8000,
    FREQ_16000_HZ = 16000,
	//TODO_CNG: add the cng swb support
	FREQ_32000_HZ = 32000
};

enum GIPS_VADmodes          // degree of bandwidth reduction
{
    VAD_CONVENTIONAL = 0,   // lowest reduction
    VAD_AGGRESSIVE_LOW,
    VAD_AGGRESSIVE_MID,
    VAD_AGGRESSIVE_HIGH     // highest reduction
};

class VOICEENGINE_DLLEXPORT GIPSVECodec
{
public:
    static GIPSVECodec* GetInterface(GIPSVoiceEngine* voiceEngine);

    // Codec-lookup functions
    virtual int GIPSVE_NumOfCodecs() = 0;
    virtual int GIPSVE_GetCodec(int index, GIPS_CodecInst& codec) = 0;

    // Codec-setting functions
    virtual int GIPSVE_SetSendCodec(int channel, const GIPS_CodecInst& codec) = 0;
    virtual int GIPSVE_GetSendCodec(int channel, GIPS_CodecInst& codec) = 0;
    virtual int GIPSVE_GetRecCodec(int channel, GIPS_CodecInst& codec) = 0;
    virtual int GIPSVE_SetSendCodecAuto(int channel, bool enable, int isacPT, int speexPT) = 0;

    // Extended AMR functions
    virtual int GIPSVE_SetAMREncFormat(int channel, GIPS_AMRmodes mode = AMR_RFC3267_BWEFFICIENT) = 0;
    virtual int GIPSVE_SetAMRDecFormat(int channel, GIPS_AMRmodes mode = AMR_RFC3267_BWEFFICIENT) = 0;

	// Extended iSAC functions
    virtual int GIPSVE_SetISACInitTargetRate(int channel, int rateBps, bool useFixedFrameSize = false) = 0;
    virtual int GIPSVE_SetISACMaxRate(int channel, int rateBps) = 0;
	virtual int GIPSVE_SetISACMaxPayloadSize(int channel, int sizeBytes) = 0;

    // Payload-type functions
    virtual int GIPSVE_SetRecPayloadType(int channel, const GIPS_CodecInst& codec) = 0;
    virtual int GIPSVE_SetSendCNPayloadType(int channel, int type, GIPS_PayloadFrequencies frequency = FREQ_8000_HZ) = 0;
	virtual int GIPSVE_GetRecPayloadType(int channel, GIPS_CodecInst& codec) = 0;

    // VAD/DTX status
    virtual int GIPSVE_SetVADStatus(int channel, bool enable, GIPS_VADmodes mode = VAD_CONVENTIONAL, bool disableDTX = false) = 0;
    virtual int GIPSVE_GetVADStatus(int channel, bool& enabled, GIPS_VADmodes& mode, bool& disabledDTX) = 0;

    // Bandwidth extension functions (valid for 16-kHz codecs only)
    virtual int GIPSVE_SetBandwidthExtensionStatus(bool enable) = 0;
    virtual int GIPSVE_GetBandwidthExtensionStatus(bool& enabled) = 0;

    // StringPair functions
    virtual int GIPSVE_ConfigureChannel(int channel, const char* mimeKeyValues) = 0;
    virtual int GIPSVE_GetChannelMIMEParameters(int channel, char* buf, unsigned int length) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVECodec();
};

#endif    // #if !defined(__GIPSVE_CODEC_H__)
