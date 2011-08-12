// GIPSVEEncryption.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_ENCRYPTION_H__)
#define __GIPSVE_ENCRYPTION_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

enum GIPS_CipherTypes
{
    CIPHER_NULL = 0,
    CIPHER_AES_128_COUNTER_MODE
};

enum GIPS_AuthenticationTypes
{
    AUTH_NULL = 0,
    AUTH_HMAC_SHA1 = 3
};

enum GIPS_SecurityLevels
{
    NO_PROTECTION = 0,
    ENCRYPTION,
    AUTHENTICATION,
    ENCRYPTION_AND_AUTHENTICATION
};

class VOICEENGINE_DLLEXPORT GIPSVEEncryption
{
public:
    static GIPSVEEncryption* GetInterface(GIPSVoiceEngine* voiceEngine);

    // SRTP
    virtual int GIPSVE_EnableSRTPSend(int channel, GIPS_CipherTypes cipherType, unsigned int cipherKeyLength, GIPS_AuthenticationTypes authType, unsigned int authKeyLength, unsigned int authTagLength, GIPS_SecurityLevels level, const unsigned char* key) = 0;
    virtual int GIPSVE_DisableSRTPSend(int channel) = 0;
    virtual int GIPSVE_EnableSRTPReceive(int channel, GIPS_CipherTypes cipherType, unsigned int cipherKeyLength, GIPS_AuthenticationTypes authType, unsigned int authKeyLength, unsigned int authTagLength, GIPS_SecurityLevels level, const unsigned char* key) = 0;
    virtual int GIPSVE_DisableSRTPReceive(int channel) = 0;

    // External encryption
    virtual int GIPSVE_InitEncryption(GIPS_encryption* encryptionObject) = 0;
    virtual int GIPSVE_SetEncryptionStatus(int channel, bool enable) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVEEncryption();
};

#endif    // #if !defined(__GIPSVE_ENCRYPTION_H__)
