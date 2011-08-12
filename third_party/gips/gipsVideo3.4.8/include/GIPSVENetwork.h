// GIPSVENetwork.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_NETWORK_H__)
#define __GIPSVE_NETWORK_H__

#include "GIPSVECommon.h"


class VOICEENGINE_DLLEXPORT GIPSVEConnectionObserver
{
public:
    virtual void OnPeriodicDeadOrAlive(int channel, bool alive) = 0;
protected:
    virtual ~GIPSVEConnectionObserver() {}
};

class GIPSVoiceEngine;

class VOICEENGINE_DLLEXPORT GIPSVENetwork
{
public:
    static GIPSVENetwork* GetInterface(GIPSVoiceEngine* voiceEngine);

    // External transport
    virtual int GIPSVE_SetExternalTransport(int channel, bool enable, GIPS_transport* transport) = 0;
    virtual int GIPSVE_ReceivedRTPPacket(int channel, const void* data, unsigned int length) = 0;
    virtual int GIPSVE_ReceivedRTCPPacket(int channel, const void* data, unsigned int length) = 0;

    // Get info
    virtual int GIPSVE_GetSourceInfo(int channel, int& rtpPort, int& rtcpPort, char* ipaddr, unsigned int ipaddrLength) = 0;
    virtual int GIPSVE_GetLocalIP(char* ipaddr, unsigned int ipaddrLength, bool ipv6 = false) = 0;

    // IPv6
    virtual int GIPSVE_EnableIPv6(int channel) = 0;
    virtual bool GIPSVE_IPv6IsEnabled(int channel) = 0;

    // Source IP address and port filter
    virtual int GIPSVE_SetSourceFilter(int channel, int rtpPort, int rtcpPort = 0, const char* ipaddr = NULL) = 0;
    virtual int GIPSVE_GetSourceFilter(int channel, int& rtpPort, int& rtcpPort, char* ipaddr, unsigned int ipaddrLength) = 0;

    // TOS (Windows and Linux only)
    virtual int GIPSVE_SetSendTOS(int channel, int DSCP, bool useSetSockopt = false) = 0;
    virtual int GIPSVE_GetSendTOS(int channel, int& DSCP, bool& useSetSockopt) = 0;

	// GQoS (Windows only)
    virtual int GIPSVE_SetSendGQoS(int channel, bool enable, int serviceType, int overrideDSCP = 0) = 0;
    virtual int GIPSVE_GetSendGQoS(int channel, bool& enabled, int& serviceType, int& overrideDSCP) = 0;

    // Packet timeout notification
    virtual int GIPSVE_SetPacketTimeoutNotification(int channel, bool enable, int timeoutSeconds) = 0;

	// Periodic dead-or-alive reports
	virtual int GIPSVE_SetDeadOrAliveObserver(GIPSVEConnectionObserver* observer) = 0;
	virtual int GIPSVE_SetPeriodicDeadOrAliveStatus(int channel, bool enable, int sampleTimeSeconds = 2) = 0;

    // Send packet using the User Datagram Protocol (UDP)
    virtual int GIPSVE_SendUDPPacket(int channel, const void* data, unsigned int length, int& transmittedBytes, bool useRtcpSocket = false) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVENetwork();
};

#endif    // #if !defined(__GIPSVE_NETWORK_H__)
