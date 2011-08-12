// GIPSVERTP_RTCP.h
//
// Copyright (c) 2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_RTP_RTCP_H__)
#define __GIPSVE_RTP_RTCP_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

enum GIPS_RTPDirections
{
    RTP_INCOMING = 0,   // incoming (received) RTP/RTCP session
    RTP_OUTGOING        // outgoing (transmitted) RTP/RTCP session
};

class VOICEENGINE_DLLEXPORT GIPSVERTPObserver
{
public:
    virtual void OnIncomingCSRCChanged(const int channel, const unsigned int CSRC, const bool added) = 0;
	virtual void OnIncomingSSRCChanged(const int channel, const unsigned int SSRC) = 0;
protected:
    virtual ~GIPSVERTPObserver() {}
};

class VOICEENGINE_DLLEXPORT GIPSVERTCPObserver
{
public:
    virtual void OnApplicationDataReceived(const int channel, const unsigned char subType, const unsigned int name, const char* data, const unsigned short dataLengthInBytes) = 0;
protected:
    virtual ~GIPSVERTCPObserver() {}
};

struct GIPS_CallStatistics
{
    unsigned short fractionLost;
    unsigned int cumulativeLost;
    unsigned int extendedMax;
    unsigned int jitterSamples;
    int rttMs;
    int bytesSent;
    int packetsSent;
    int bytesReceived;
    int packetsReceived;
};

class VOICEENGINE_DLLEXPORT GIPSVERTP_RTCP
{
public:
    static GIPSVERTP_RTCP* GetInterface(GIPSVoiceEngine* voiceEngine);

    // Register observers for RTP and RTCP
    virtual int GIPSVE_SetRTPObserver(GIPSVERTPObserver* observer) = 0;
    virtual int GIPSVE_SetRTCPObserver(GIPSVERTCPObserver* observer) = 0;

    // SSRC
    virtual int GIPSVE_SetSendSSRC(int channel, unsigned int ssrc) = 0;
    virtual int GIPSVE_GetSendSSRC(int channel, unsigned int& ssrc) = 0;
    virtual int GIPSVE_GetRemoteSSRC(int channel, unsigned int& ssrc) = 0;

    // CSRC & Energy
    virtual int GIPSVE_GetRemoteCSRCs(int channel, unsigned int arrCSRC[15]) = 0;
    virtual int GIPSVE_GetRemoteEnergy(int channel, unsigned char arrEnergy[15]) = 0;

    // RTCP
    virtual int GIPSVE_SetRTCPStatus(int channel, bool enable) = 0;
	virtual int GIPSVE_GetRTCPStatus(int channel, bool& enabled) = 0;
    virtual int GIPSVE_SetRTCP_CNAME(int channel, const char* cname) = 0;
    virtual int GIPSVE_GetRemoteRTCP_CNAME(int channel, char* cname) = 0;
    virtual int GIPSVE_GetRemoteRTCPData(int channel, unsigned int& NTPHigh, unsigned int& NTPLow, unsigned int& timestamp,
        unsigned int& playoutTimestamp, unsigned int* jitter = NULL, unsigned short* fractionLost = NULL) = 0;
    virtual int GIPSVE_SendApplicationDefinedRTCPPacket(int channel, const unsigned char subType, unsigned int name, 
        const char* data, unsigned short dataLengthInBytes) = 0;

    // Statistics
    virtual int GIPSVE_GetRTPStatistics(int channel, unsigned int& averageJitterMs, unsigned int& maxJitterMs,
        unsigned int& discardedPackets) = 0;
    virtual int GIPSVE_GetRTCPStatistics(int channel, unsigned short& fractionLost, unsigned int& cumulativeLost,
        unsigned int& extendedMax, unsigned int& jitterSamples, int& rttMs) = 0;
    virtual int GIPSVE_GetRTCPStatistics(int channel, GIPS_CallStatistics& stats) = 0;

    // FEC
    virtual int GIPSVE_SetFECStatus(int channel, bool enable, int redPayloadtype = -1) = 0;

    // RTP keepalive mechanism (maintains NAT mappings associated to RTP flows)
    virtual int GIPSVE_SetRTPKeepaliveStatus(int channel, bool enable, int unknownPayloadType, int deltaTransmitTimeSeconds = 15) = 0;
    virtual int GIPSVE_GetRTPKeepaliveStatus(int channel, bool& enabled, int& unknownPayloadType, int& deltaTransmitTimeSeconds) = 0;

    // Store RTP and RTCP packets and dump to file (compatible with rtpplay)
    virtual int GIPSVE_StartRTPDump(int channel, const char* fileNameUTF8, GIPS_RTPDirections direction = RTP_INCOMING) = 0;
    virtual int GIPSVE_StopRTPDump(int channel, GIPS_RTPDirections direction = RTP_INCOMING) = 0;
    virtual int GIPSVE_RTPDumpIsActive(int channel, GIPS_RTPDirections direction = RTP_INCOMING) = 0;

	// Insert (and transmits) extra RTP packet into active RTP audio stream
    virtual int GIPSVE_InsertExtraRTPPacket(int channel, unsigned char payloadType, bool markerBit, const char* payloadData,  unsigned short payloadSize) = 0;

    virtual int Release() = 0;

protected:
    virtual ~GIPSVERTP_RTCP();
};

#endif    // #if !defined(__GIPSVE_RTP_RTCP_H__)
