// GIPSVESymbian.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_SYMBIAN_H__)
#define __GIPSVE_SYMBIAN_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

/*=============================================================================
 *	MVoiceEngineSymbianObserver
 *
 *	An interface class that notifies the user of the progress of GIPS
 *	Voice Engine for Symbian.
 *
 *	It must be implemented by users of the GipsVoiceEngineSymbian class.
 *
 *	An object implementing this interface is passed to the factory method
 *	GipsVoiceEngineSymbian::NewL().
 *=============================================================================
 */
class MVoiceEngineSymbianObserver
{
public:
	
/**
	A callback function that is called when GipsVoiceEngineLib::GIPSVE_Init()
	has completed, indicating that the voice engine has been initilized
	and is ready for use. MUST be implemented by the user.
*/
	virtual void GIPSVE_CallbackOnInit(TInt aError) = 0;
	
/**
	A callback functions that is called when the Voice Engine detects that
	an interfaces is coming up. MAY be implemented by the user.
*/	
	virtual void GIPSVE_CallbackOnInterfaceUp(TUint32 aIapId, const TDesC& aIapName, const TDesC& aIPaddr, const TDesC& aMACaddr) { };	
	
/**
	A callback functions that is called when the Voice Engine detects that
	an interfaces is going down. MAY be implemented by the user.
*/	
	virtual void GIPSVE_CallbackOnInterfaceDown(TUint32 aIapId) { };

/**
	A callback function that is called when GipsVoiceEngineLib::GIPSVE_TryToReconnect()
	has completed, indicating that a new connection is now active and ready for use.
	MAY be implemented by the user.
*/	
	virtual void GIPSVE_CallbackOnReconnected(TInt aError) { };
};

/*=============================================================================
 *	GIPSConvert
 *
 *	This class contains functions that simplify conversion between Symbian
 *	descriptors and ANSI C strings, i.e., null-terminated ASCII-valued strings. 
 *	Note that, the main VoiceEngine API uses C strings only.
 *=============================================================================
 */
class GIPSConvert
{
public:
	IMPORT_C static TInt Des16ToCString8(const TDesC16& des16, char* cstr8, TInt buflen);
	IMPORT_C static TInt CString8ToDes16(const char* cstr8, TDes16& des16);
};


/*=============================================================================
 *	GIPSVESymbian
 *
 *	Add info here...
 *=============================================================================
 */
class GIPSVESymbian
{
public:
    IMPORT_C static GIPSVESymbian* GetInterface(GIPSVoiceEngine* voiceEngine, MVoiceEngineSymbianObserver* observer);
    
    // interface methods
	IMPORT_C virtual TInt GIPSVE_ActivateInterfaceMonitoring(TBool aOnOff, TUint aUpdateIntervalMilliSecs = 2000) = 0;
	
	// connection methods
	IMPORT_C virtual TInt GIPSVE_SetConnectionIAP(TUint32 aIapId) = 0;
	IMPORT_C virtual TInt GIPSVE_GetConnectedIAP(TUint32& aIapId) = 0;
	IMPORT_C virtual TInt GIPSVE_TryToReconnect(TUint32 aIapId) = 0;
	
	// audio routing
	IMPORT_C virtual TInt GIPSVE_ActivateLoudspeaker(TBool aOnOff) = 0;
	
	// network methods
	IMPORT_C virtual TInt GIPSVE_GetPhoneSignalStrength(TInt& aSignalStrength, TInt8& aSignalBars) = 0;
	IMPORT_C virtual TInt GIPSVE_GetWLANNetworkInfo(CDesCArray& aNetwork, CArrayFix<TInt>& aSignalStrength) = 0;
	IMPORT_C virtual TInt GIPSVE_GetWLANSignalStrength(const TDesC& aIapName, TInt& aStrength) = 0;
	IMPORT_C virtual TInt GIPSVE_GetWLANTransferredData(const TDesC& aIapName, TInt& aDownlink, TInt& aUplink) = 0;

    IMPORT_C virtual int Release() = 0;

protected:
    virtual ~GIPSVESymbian();
};

#endif    // #if !defined(__GIPSVE_SYMBIAN_H__)
