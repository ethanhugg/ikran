/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _USE_CPVE

#include "CC_Common.h"

#include "string.h"
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "CSFGipsMediaProvider.h"
#include "CSFGipsAudioProvider.h"
#include "CSFGipsToneGenerator.h"
#include "CSFGipsRingGenerator.h"
#include "GIPSVEFile.h"
#include "GIPSVEHardware.h"
#include "GIPSVEErrors.h"
#include "GIPSVENetwork.h"
#include "GIPSVEVQE.h"
#include "CSFGipsLogging.h"

#include "CSFLog.h"
static const char* logTag = "CSFGipsAudioProvider";
using namespace std;

extern "C" void config_get_value (int id, void *buffer, int length);


namespace CSF {

#define EXTRACT_DYNAMIC_PAYLOAD_TYPE(PTYPE) ((PTYPE)>>16)
#define CLEAR_DYNAMIC_PAYLOAD_TYPE(PTYPE)   (PTYPE & 0x0000FFFF)
#define CHECK_DYNAMIC_PAYLOAD_TYPE(PTYPE)   (PTYPE & 0xFFFF0000)


#ifdef WIN32
bool IsUserAdmin();
bool IsVistaOrNewer();
#endif

class GipsAudioStream
{
public:
	GipsAudioStream(int _streamId, int _channelId):
		streamId(_streamId), channelId(_channelId),
		isRxStarted(false), isTxStarted(false), isAlive(false)
		{}
	int streamId;
	int channelId;
	bool isRxStarted;
	bool isTxStarted;
	bool isAlive;
};

GipsAudioProvider::GipsAudioProvider( GipsMediaProvider* provider )
: provider(provider), 
  gipsVoice(NULL),
  gipsBase(NULL), 
  gipsFile(NULL),
  gipsHw(NULL), 
  gipsDTMF(NULL), 
  gipsNetwork(NULL), 
  gipsVolumeControl(NULL), 
  gipsVoiceQuality(NULL), 
  gipsEncryption(NULL),
  toneGen(NULL), 
  ringGen(NULL), 
  startPort(1024), 
  endPort(65535), 
  defaultVolume(100), 
  ringerVolume(100), 
  DSCPValue(0), 
  VADEnabled(false),
  stopping(false)
{
	LOG_GIPS_INFO( logTag, "GipsAudioProvider::GipsAudioProvider");
}

int GipsAudioProvider::init()
{
	LOG_GIPS_INFO( logTag, "GipsAudioProvider::init");

	if (gipsVoice != NULL)
	{
		LOG_GIPS_ERROR( logTag, "GipsAudioProvider::init : already initialized");
		return -1;
	}

    gipsVoice = GIPSVoiceEngine::Create();
	if (gipsVoice== NULL)
	{
		LOG_GIPS_ERROR( logTag, "GipsAudioProvider(): GIPSVoiceEngine::Create failed");
		return -1;
	}

	gipsBase = GIPSVEBase::GIPSVE_GetInterface( gipsVoice );
	if (gipsBase== NULL)
	{
		LOG_GIPS_ERROR( logTag, "GipsAudioProvider(): GIPSVEBase::GIPSVE_GetInterface failed");
		return -1;
	}

	const int VERSIONBUFLEN=1024;
	char versionbuf[VERSIONBUFLEN];
	gipsBase->GIPSVE_GetVersion(versionbuf,VERSIONBUFLEN);
	LOG_GIPS_INFO( logTag, "%s", versionbuf );
//	gipsBase->GIPSVE_SetObserver( *this );
#ifdef ENABLE_GIPS_AUDIO_TRACE
#if GIPS_VER < 3510
	gipsBase->GIPSVE_SetTraceFile( "GIPSaudiotrace.out" );
#elif GIPS_VER >= 3510
	gipsVoice->SetTraceFile( "GIPSaudiotrace.out" );
	gipsVoice->SetTraceCallback(this);
#endif	
#endif
	gipsDTMF = GIPSVEDTMF::GetInterface( gipsVoice );
	gipsFile = GIPSVEFile::GetInterface( gipsVoice );
	gipsHw = GIPSVEHardware::GetInterface( gipsVoice );
	gipsNetwork = GIPSVENetwork::GetInterface( gipsVoice );
	gipsVolumeControl = GIPSVEVolumeControl::GetInterface( gipsVoice );
	gipsVoiceQuality = GIPSVEVQE::GetInterface( gipsVoice );
    gipsEncryption = GIPSVEEncryption::GetInterface(gipsVoice);

	if ((!gipsDTMF) || (!gipsFile) || (!gipsHw) ||(!gipsNetwork) || (!gipsVolumeControl) || (!gipsVoiceQuality) || (!gipsEncryption)) 
	{
		LOG_GIPS_ERROR( logTag, "GipsAudioProvider(): GIPSVE_GetInterface failed gipsDTMF=%p gipsFile=%p gipsHw=%p gipsNetwork=%p gipsVolumeControl=%p gipsVoiceQuality=%p gipsEncryption=%p",
		gipsDTMF,gipsFile,gipsHw,gipsNetwork,gipsVolumeControl,gipsVoiceQuality,gipsEncryption);
		return -1;
	}

	codecSelector.init(gipsVoice, false, true);
	gipsBase->GIPSVE_Init();
	gipsNetwork->GIPSVE_SetDeadOrAliveObserver(this);
	localRingChannel = gipsBase->GIPSVE_CreateChannel();
	localToneChannel = gipsBase->GIPSVE_CreateChannel();

/*	
	Set up Voice Quality Enhancement Parameters
*/
#if GIPS_VER >= 3510
//	Extra code for 3.5.X currently just used on the Mac
// Should be enabled when we move to 3.5 on Windows
	GIPS_AGC_config  webphone_agc_config={targetLeveldBOvdefault,digitalCompressionGaindBdefault,limiterEnableon};
	gipsVoiceQuality->GIPSVE_SetAGCConfig(webphone_agc_config); 
#endif

#if GIPS_VER >= 3510
	gipsVoiceQuality->GIPSVE_SetAGCStatus(true, AGC_FIXED_DIGITAL);
#else
	gipsVoiceQuality->GIPSVE_SetAGCStatus(true, AGC_STANDALONE_DIGITAL);
#endif
	gipsVoiceQuality->GIPSVE_SetNSStatus(true, NS_MODERATE_SUPPRESSION);
//	gipsVoiceQuality->GIPSVE_SetECStatus(true, EC_AEC);

	// get default device names
	char name[128];
	if ( gipsHw->GIPSVE_GetRecordingDeviceName( -1, name, sizeof(name) ) == 0 )
		recordingDevice = name;

	if ( gipsHw->GIPSVE_GetPlayoutDeviceName( -1, name, sizeof(name) ) == 0 )
		playoutDevice = name;

    LOG_GIPS_DEBUG( logTag, "local IP: \"%s\"", localIP.c_str());
    LOG_GIPS_DEBUG( logTag, "RecordingDeviceName: \"%s\"", recordingDevice.c_str());
    LOG_GIPS_DEBUG( logTag, "PlayoutDeviceName: \"%s\"", playoutDevice.c_str());
	// success
	return 0;
}

GipsAudioProvider::~GipsAudioProvider()
{
	LOG_GIPS_INFO( logTag, "GipsAudioProvider::~GipsAudioProvider");

	int num_ifs=0;
	stopping = true;
	// tear down in reverse order, for symmetry
	codecSelector.release();

	gipsNetwork->GIPSVE_SetDeadOrAliveObserver(NULL);

	gipsBase->GIPSVE_DeleteChannel( localToneChannel );
	gipsBase->GIPSVE_DeleteChannel( localRingChannel );
	gipsBase->GIPSVE_Terminate();
	gipsBase->GIPSVE_Release();

	
	if ((num_ifs=gipsDTMF->Release())!=0)
		LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): gipsDTMF->Release failed, num_ifs left= %d",num_ifs);
	if ((num_ifs=gipsFile->Release())!=0)
		LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): gipsFile->Release failed, num_ifs left= %d ",num_ifs);
	if((num_ifs=gipsHw->Release())!=0)
		LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): gipsHw->Release failed, num_ifs left= %d " ,num_ifs);
	if((num_ifs=gipsNetwork->Release())!=0)
		LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): gipsNetwork->Release() failed, num_ifs left= %d" ,num_ifs);
	if((num_ifs=gipsVolumeControl->Release())!=0)
		LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): gipsVolumeControl->Release() failed, num_ifs left= %d" ,num_ifs);
	if((num_ifs=gipsVoiceQuality->Release())!=0)
		LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): gipsVoiceQuality->Release() failed, num_ifs left= %d ",num_ifs );
    if((num_ifs=gipsEncryption->Release())!=0)
        LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): gipsEncryption->Release() failed, num_ifs left= %d ",num_ifs );
	if(GIPSVoiceEngine::Delete( gipsVoice )==false)
		LOG_GIPS_ERROR( logTag, "~GipsAudioProvider(): GIPSVoiceEngine::Delete failed" );

	delete toneGen;
	toneGen = NULL;

	delete ringGen;
	ringGen = NULL;

	// Clear all our pointers
	gipsFile = NULL;
	gipsHw = NULL;
	gipsDTMF = NULL;
	gipsNetwork = NULL;
	gipsVolumeControl = NULL;
	gipsVoiceQuality = NULL;
	gipsVoice = NULL;
	gipsBase = NULL;
    gipsEncryption = NULL;

	LOG_GIPS_INFO( logTag, "GipsAudioProvider::shutdown done");
}

std::vector<std::string> GipsAudioProvider::getRecordingDevices()
{
	base::AutoLock lock(m_lock);
	char name[128];
	int nPlay = 0, nRec = 0;
	std::vector<std::string> deviceList;

	gipsHw->GIPSVE_GetNumOfSoundDevices( nPlay, nRec );
	for ( int i = 0; i < nRec; i++ )
	{
		if ( gipsHw->GIPSVE_GetRecordingDeviceName( i, name, csf_countof(name) ) == 0 )
			deviceList.push_back( name );
	}
	return deviceList;
}

std::vector<std::string> GipsAudioProvider::getPlayoutDevices()
{
	base::AutoLock lock(m_lock);
	char name[128];
	int nPlay = 0, nRec = 0;
	std::vector<std::string> deviceList;

	gipsHw->GIPSVE_GetNumOfSoundDevices( nPlay, nRec );
	for ( int i = 0; i < nPlay; i++ )
	{
		if ( gipsHw->GIPSVE_GetPlayoutDeviceName( i, name, csf_countof(name) ) == 0 )
			deviceList.push_back( name );
	}
	return deviceList;
}

bool GipsAudioProvider::setRecordingDevice( const std::string& device )
{
	base::AutoLock lock(m_lock);
	char name[128];
	int nPlay = 0, nRec = 0;
	int playoutIndex, recordingIndex;

	gipsHw->GIPSVE_GetNumOfSoundDevices( nPlay, nRec );

	// find requested recording device
	for ( recordingIndex = 0; recordingIndex < nRec; recordingIndex++ )
	{
		if ( gipsHw->GIPSVE_GetRecordingDeviceName( recordingIndex, name, csf_countof(name) ) == 0 )
		{
			if ( device.compare( name ) == 0 ) break;
		}
	}
	if ( recordingIndex == nRec )	// we didn't find the requested device, fail
	{
		return false;
	}

	// find existing playout device, to preserve its index
	// search downward until we reach the default device
	name[0] = '\0';
	for ( playoutIndex = nPlay - 1; playoutIndex >= -1; playoutIndex-- )
	{
		if ( gipsHw->GIPSVE_GetPlayoutDeviceName( playoutIndex, name, sizeof(name) ) == 0 )
		{
			if ( playoutDevice.compare( name ) == 0 ) break;
		}
		else name[0] = '\0';
	}
	if ( playoutIndex < -1 )	// we didn't find the current device, use default
	{
		playoutIndex = -1;
	}

	if ( gipsHw->GIPSVE_SetSoundDevices( recordingIndex, playoutIndex ) == 0 )
	{
		recordingDevice = device;
		playoutDevice = name;	// the current name
		return true;
	}
	return false;
}

bool GipsAudioProvider::setPlayoutDevice( const std::string& device )
{
	base::AutoLock lock(m_lock);
	char name[128];
	int nPlay = 0, nRec = 0;
	int playoutIndex, recordingIndex;

	gipsHw->GIPSVE_GetNumOfSoundDevices( nPlay, nRec );

	// find requested playout device
	for ( playoutIndex = 0; playoutIndex < nPlay; playoutIndex++ )
	{
		if ( gipsHw->GIPSVE_GetPlayoutDeviceName( playoutIndex, name, sizeof(name) ) == 0 )
		{
			if ( device.compare( name ) == 0 ) break;
		}
	}
	if ( playoutIndex == nPlay )	// we didn't find the requested device, fail
	{
		return false;
	}

	// find existing recording device, to preserve its index
	// search downward until we reach the default device
	name[0] = '\0';
	for ( recordingIndex = nRec - 1; recordingIndex >= -1; recordingIndex-- )
	{
		if ( gipsHw->GIPSVE_GetRecordingDeviceName( recordingIndex, name, sizeof(name) ) == 0 )
		{
			if ( recordingDevice.compare( name ) == 0 ) break;
		}
		else name[0] = '\0';
	}
	if ( recordingIndex < -1 )	// we didn't find the current device, use default
	{
		recordingIndex = -1;
	}

	if ( gipsHw->GIPSVE_SetSoundDevices( recordingIndex, playoutIndex ) == 0 )
	{
		playoutDevice = device;
		recordingDevice = name;	// the current name
		return true;
	}
	return false;
}

GipsAudioStreamPtr GipsAudioProvider::getStreamByChannel( int channel )
{
	base::AutoLock lock(streamMapMutex);
	for( std::map<int, GipsAudioStreamPtr>::const_iterator it = streamMap.begin(); it != streamMap.end(); it++ )
	{
		GipsAudioStreamPtr stream = it->second;
		if(stream->channelId == channel)
			return stream;
	}
	return GipsAudioStreamPtr();
}

GipsAudioStreamPtr GipsAudioProvider::getStream( int streamId )
{
	base::AutoLock lock(streamMapMutex);
	std::map<int, GipsAudioStreamPtr>::const_iterator it = streamMap.find( streamId );
	return ( it != streamMap.end() ) ? it->second : GipsAudioStreamPtr();
}

int GipsAudioProvider::getChannelForStreamId( int streamId )
{
	GipsAudioStreamPtr stream = getStream(streamId);
	return ( stream != NULL ) ? stream->channelId : -1;
}

int GipsAudioProvider::getCodecList( CodecRequestType requestType )
{
	base::AutoLock lock(m_lock);
	return codecSelector.advertiseCodecs(requestType);
}

int GipsAudioProvider::rxAlloc( int groupId, int streamId, int requestedPort )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "rxAllocAudio: groupId=%d, streamId=%d, requestedPort=%d", groupId, streamId, requestedPort  );
	int channel = gipsBase->GIPSVE_CreateChannel();
	if ( channel == -1 )
	{
		LOG_GIPS_ERROR( logTag, "rxAllocAudio: CreateChannel failed, error %d", gipsBase->GIPSVE_LastError() );
		return 0;
	}
	LOG_GIPS_DEBUG( logTag, "rxAllocAudio: Created channel %d", channel );
	gipsNetwork->GIPSVE_SetPeriodicDeadOrAliveStatus(channel, true);

	int beginPort;		// where we started
	int tryPort;		// where we are now

	if ( requestedPort == 0  || requestedPort < startPort || requestedPort >= endPort )
		tryPort = startPort;
	else
		tryPort = requestedPort;

	beginPort = tryPort;

	const char * pLocalAddr = NULL;

	if (localIP.size() > 0)
	{
		pLocalAddr = localIP.c_str();
	}

	do
	{
		if ( gipsBase->GIPSVE_SetLocalReceiver( channel, tryPort, GIPS_DEFAULT, pLocalAddr ) == 0 )
		{
			int port, RTCPport;
			char ipaddr[32];
			// retrieve local receiver settings for channel
			gipsBase->GIPSVE_GetLocalReceiver(channel, port, RTCPport, ipaddr, 32);
			localIP = ipaddr;
			LOG_GIPS_DEBUG( logTag, "rxAllocAudio: IPAddr: %d", ipaddr );
			LOG_GIPS_DEBUG( logTag, "rxAllocAudio: Allocated port %d", tryPort );
			GipsAudioStreamPtr stream(new GipsAudioStream(streamId, channel));
			{
				base::AutoLock lock(streamMapMutex);
				streamMap[streamId] = stream;
			}
			setVolume(streamId, defaultVolume);
			return tryPort;
		}

		int errCode = gipsBase->GIPSVE_LastError();
		if ( errCode == VE_SOCKET_ERROR ||			
			 errCode == VE_BINDING_SOCKET_TO_LOCAL_ADDRESS_FAILED ||
			errCode == VE_RTCP_SOCKET_ERROR )	    
        {
	        tryPort += 2;
	        if ( tryPort >= endPort )
	        	tryPort = startPort;
        }
		else
		{
			LOG_GIPS_ERROR( logTag, "rxAllocAudio: SetLocalReceiver returned error %d", errCode );
			gipsBase->GIPSVE_DeleteChannel( channel );
			return 0;
		}
	}
	while ( tryPort != beginPort );

	LOG_GIPS_WARN( logTag, "rxAllocAudio: No ports available?" );
	gipsBase->GIPSVE_DeleteChannel( channel );
	return 0;
}

int GipsAudioProvider::rxOpen( int groupId, int streamId, int requestedPort, int listenIp, bool isMulticast )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "rxOpenAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 )
	{
		LOG_GIPS_DEBUG( logTag, "rxOpenAudio: return requestedPort=%d", requestedPort );
		return requestedPort;
	}
	return 0;
}

int GipsAudioProvider::rxStart( int groupId, int streamId, int payloadType, int packPeriod, int localPort,
		int rfc2833PayloadType, EncryptionAlgorithm algorithm, unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "rxStartAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 )
	{
		int dynamicPayloadType = -1;

	    if (CHECK_DYNAMIC_PAYLOAD_TYPE(payloadType))
	    {
	        dynamicPayloadType = EXTRACT_DYNAMIC_PAYLOAD_TYPE(payloadType);
	        payloadType = CLEAR_DYNAMIC_PAYLOAD_TYPE(payloadType);
	    }

	    if (dynamicPayloadType != -1)
	    {
			GIPS_CodecInst codec;

			if (codecSelector.select(payloadType, dynamicPayloadType, packPeriod, codec) != 0)
			{
				LOG_GIPS_ERROR( logTag, "rxStartAudio cannot select codec (payloadType=%d, packPeriod=%d)",
						payloadType, packPeriod );
				return -1;
			}

			if (codecSelector.setReceive(channel, codec) != 0)
			{
				LOG_GIPS_ERROR( logTag, "rxStartAudio cannot set receive codec to channel=%d", channel );
				return -1;
			}
	    }

        switch(algorithm)
        {
            case EncryptionAlgorithm_NONE:
                LOG_GIPS_DEBUG( logTag, "rxStartAudio: using non-secure RTP for channel %d", channel);
                break;

            case EncryptionAlgorithm_AES_128_COUNTER:
            {
                unsigned char key[GIPS_KEY_LENGTH];

                LOG_GIPS_DEBUG( logTag, "rxStartAudio: using secure RTP for channel %d", channel);

                if(!provider->getKey(key, keyLen, salt, saltLen, key, sizeof(key)))
                {
                    LOG_GIPS_ERROR( logTag, "rxStartAudio: failed to generate key on channel %d", channel );
                    return -1;
                }

                if(gipsEncryption->GIPSVE_EnableSRTPReceive(channel,
                    CIPHER_AES_128_COUNTER_MODE,
                    GIPS_CIPHER_LENGTH,
                    AUTH_NULL, 0, 0, ENCRYPTION, key) != 0)
                {
                    LOG_GIPS_ERROR( logTag, "rxStartAudio: GIPSVE_EnableSRTPReceive on channel %d failed", channel );
                    memset(key, 0x00, sizeof(key));
                    return -1;
                }

                memset(key, 0x00, sizeof(key));

                break;
            }  
        }

	    if (gipsBase->GIPSVE_StartListen( channel ) == -1)
	    {
	    	LOG_GIPS_ERROR( logTag, "rxStartAudio: cannot start listen on channel %d", channel );
	    	return -1;
	    }

	    LOG_GIPS_DEBUG( logTag, "rxStartAudio: Listening on channel %d", channel );

		if (gipsBase->GIPSVE_StartPlayout( channel ) == -1)
		{
			LOG_GIPS_ERROR( logTag, "rxStartAudio: cannot start playout on channel %d, stop listen", channel );
			gipsBase->GIPSVE_StopListen( channel );
		}

		GipsAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isRxStarted = true;
		LOG_GIPS_DEBUG( logTag, "rxStartAudio: Playing on channel %d", channel );
		return 0;
	}
	else
	{
		LOG_GIPS_ERROR( logTag, "rxStartAudio: getChannelForStreamId failed streamId %d",streamId );
	}
	return -1;
}

void GipsAudioProvider::rxClose( int groupId, int streamId )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "rxCloseAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 )
	{
		GipsAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isRxStarted = false;
		gipsBase->GIPSVE_StopPlayout( channel );
		LOG_GIPS_DEBUG( logTag, "rxCloseAudio: Stop playout on channel %d", channel );
	}
}

void GipsAudioProvider::rxRelease( int groupId, int streamId, int port )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "rxReleaseAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 )
	{
		gipsBase->GIPSVE_StopListen( channel );
		gipsBase->GIPSVE_DeleteChannel( channel );
		{
			base::AutoLock lock(streamMapMutex);
			streamMap.erase(streamId);
		}

		LOG_GIPS_DEBUG( logTag, "rxReleaseAudio: Delete channel %d, release port %d", channel, port);
	}
	else
	{
		LOG_GIPS_ERROR( logTag, "rxReleaseAudio: getChannelForStreamId failed streamId %d",streamId );
	}
}

const unsigned char m_iGQOSServiceType =0x00000003;

int GipsAudioProvider::txStart( int groupId, int streamId, int payloadType, int packPeriod, bool vad, short tos,
		char* remoteIpAddr, int remotePort, int rfc2833PayloadType, EncryptionAlgorithm algorithm, unsigned char* key, int keyLen,
		unsigned char* salt, int saltLen, int mode, int party )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "txStartAudio: groupId=%d, streamId=%d", groupId, streamId);
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 )
	{
		int dynamicPayloadType = -1;

	    if (CHECK_DYNAMIC_PAYLOAD_TYPE(payloadType))
	    {
	        dynamicPayloadType = EXTRACT_DYNAMIC_PAYLOAD_TYPE(payloadType);
	        payloadType = CLEAR_DYNAMIC_PAYLOAD_TYPE(payloadType);
	    }

	    GIPS_CodecInst codec;

		// select codec from payload type
		if (codecSelector.select(payloadType, dynamicPayloadType, packPeriod, codec) != 0)
		{
			LOG_GIPS_ERROR( logTag, "txStartAudio cannot select codec (payloadType=%d, packPeriod=%d)",
					payloadType, packPeriod );
			return -1;
		}

		// apply codec to channel
		if (codecSelector.setSend(channel, codec,payloadType,vad) != 0)
		{
			LOG_GIPS_ERROR( logTag, "txStartAudio cannot set send codec on channel=%d", channel );
			return -1;
		}

        switch(algorithm)
        {
            case EncryptionAlgorithm_NONE:
                LOG_GIPS_DEBUG( logTag, "txStartAudio: using non-secure RTP for channel %d", channel);
                break;

            case EncryptionAlgorithm_AES_128_COUNTER:
            {
                unsigned char key[GIPS_KEY_LENGTH];

                LOG_GIPS_DEBUG( logTag, "txStartAudio: using secure RTP for channel %d", channel);

                if(!provider->getKey(key, keyLen, salt, saltLen, key, sizeof(key)))
                {
                    LOG_GIPS_ERROR( logTag, "txStartAudio: failed to generate key on channel %d", channel );
                    return -1;
                }

                if(gipsEncryption->GIPSVE_EnableSRTPSend(channel,
                    CIPHER_AES_128_COUNTER_MODE,
                    GIPS_CIPHER_LENGTH,
                    AUTH_NULL, 0, 0, ENCRYPTION, key) != 0)
                {
                    LOG_GIPS_ERROR( logTag, "txStartAudio: GIPSVE_EnableSRTPSend on channel %d failed", channel );
                    memset(key, 0x00, sizeof(key));
                    return -1;
                }

                memset(key, 0x00, sizeof(key));

                break;
            }  
        }

        unsigned char dscpSixBit = DSCPValue>>2;
		gipsBase->GIPSVE_SetSendDestination( channel, remotePort, remoteIpAddr );
#ifdef WIN32
		if (IsVistaOrNewer())
		{
			LOG_GIPS_DEBUG( logTag, "Vista or later");
			if(gipsNetwork->GIPSVE_SetSendTOS(channel, dscpSixBit, false )==-1)
			{
				LOG_GIPS_DEBUG( logTag, "openIngressChannel():GIPSVE_SetSendTOS() returned error");
			}
			LOG_GIPS_DEBUG( logTag, " CGipsVeWrapper::openIngressChannel:- GIPSVE_SetSendTOS(), useSetSockOpt = false");
		}
		else
		{
			if(gipsNetwork->GIPSVE_SetSendTOS(channel, dscpSixBit, true )==-1)
			{
				LOG_GIPS_DEBUG( logTag, "openIngressChannel():GIPSVE_SetSendTOS() returned error");
			}
			LOG_GIPS_DEBUG( logTag, "CGipsVeWrapper::openIngressChannel:- GIPSVE_SetSendTOS(), useSetSockOpt = true");
		}
#else
		gipsNetwork->GIPSVE_SetSendTOS(channel, dscpSixBit, true );
#endif
		gipsBase->GIPSVE_StartSend( channel );
		GipsAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isTxStarted = true;
		LOG_GIPS_DEBUG( logTag, "txStartAudio: Sending to %s:%d on channel %d", remoteIpAddr, remotePort, channel );
		return 0;
	}
	else
	{
			LOG_GIPS_ERROR( logTag, "txStartAudio: getChannelForStreamId failed streamId %d",streamId );
			return -1;
	}

}

void GipsAudioProvider::txClose( int groupId, int streamId )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "txCloseAudio: groupId=%d, streamId=%d", groupId, streamId);
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 )
	{
		GipsAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isTxStarted = false;
		gipsBase->GIPSVE_StopSend( channel );
		LOG_GIPS_DEBUG( logTag, "txCloseAudio: Stop transmit on channel %d", channel );
	}
	else
	{
		LOG_GIPS_ERROR( logTag, "txClose: getChannelForStreamId failed streanId %d",streamId );

	}
}
int GipsAudioProvider::toneStart( ToneType type, ToneDirection direction, int alertInfo, int groupId, int streamId, bool useBackup )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "mediaToneStart: tone=%d, direction=%d, groupId=%d, streamId=%d", type, direction, groupId, streamId );
	if(toneGen != NULL)
	{
		LOG_GIPS_INFO( logTag, "mediaToneStart: tone already in progress - stop current tone [using dodgy parameters] and replace it." );
		toneStop(type, groupId, streamId);
	}
	toneGen = new GipsToneGenerator( type );
	gipsBase->GIPSVE_StartPlayout( localToneChannel );
	gipsFile->GIPSVE_StartPlayingFileLocally( localToneChannel, toneGen, FILE_PCM_8KHZ );
	return 0;
}

int GipsAudioProvider::toneStop( ToneType type, int groupId, int streamId )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "mediaToneStop: tone=%d, groupId=%d, streamId=%d", type, groupId, streamId );
	if ( gipsFile->GIPSVE_IsPlayingFileLocally( localToneChannel ) == 1 )
		gipsBase->GIPSVE_StopPlayout( localToneChannel );
	//gipsFile->GIPSVE_StopPlayingFileLocally( localToneChannel );
	delete toneGen;
	toneGen = NULL;
	return 0;
}

int GipsAudioProvider::ringStart( int lineId, RingMode mode, bool once )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "mediaRingStart: line=%d, mode=%d, once=%d", lineId, mode, once );
	if(ringGen != NULL)
	{
		LOG_GIPS_INFO( logTag, "mediaRingStart: ringing already in progress - do nothing." );
		return 0;
		//ringStop(lineId);  No longer stopping ringer here because of DE2412.
		//                   Now we let the original ringer continue, and blame skittles for not stopping the ringer first.
	}
	ringGen = new GipsRingGenerator( mode, once );
	ringGen->SetScaleFactor(ringerVolume);
	gipsBase->GIPSVE_StartPlayout( localRingChannel );
	gipsFile->GIPSVE_StartPlayingFileLocally( localRingChannel, ringGen, FILE_PCM_8KHZ );
	return 0;
}

int GipsAudioProvider::ringStop( int lineId )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "mediaRingStop: line=%d", lineId );
	if ( gipsFile->GIPSVE_IsPlayingFileLocally( localRingChannel ) == 1 )
		gipsBase->GIPSVE_StopPlayout( localRingChannel );
	//gipsFile->GIPSVE_StopPlayingFileLocally( localToneChannel );
	delete ringGen;
	ringGen = NULL;
	return 0;
}

int GipsAudioProvider::sendDtmf( int streamId, int digit)
{
	base::AutoLock lock(m_lock);
	int rfc2833Payload=101;
	
    int channel = getChannelForStreamId( streamId );
	
	if(channel >= 0)
	{
		gipsDTMF->GIPSVE_SetDTMFFeedbackStatus(true);
		gipsDTMF->GIPSVE_SetSendDTMFPayloadType(channel, rfc2833Payload); // Need to get rfc2833Payload
		gipsDTMF->GIPSVE_SendDTMF(channel, digit);
		return 0;
	}
    else
    {
    	LOG_GIPS_INFO( logTag, "failed to map stream to channel");
    }

	return -1;
}

// returns -1 on failure
bool GipsAudioProvider::mute( int streamId, bool mute )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "audio mute: streamId=%d, mute=%d", streamId, mute );
    int channel = getChannelForStreamId( streamId );
    bool returnVal = false;

    if ( channel >= 0 )
    {
		if (gipsVolumeControl->GIPSVE_SetInputMute(channel, mute) != -1)
		{
			returnVal= true;
		}
		else
		{
			LOG_GIPS_INFO( logTag, "GIPS returned failure from GIPSVE_SetInputMute");
		}
    }
    else
    {
    	LOG_GIPS_INFO( logTag, "failed to map stream to channel");
    }
    return returnVal;
}

bool GipsAudioProvider::isMuted( int streamId )
{
	base::AutoLock lock(m_lock);
	bool mute=false;

	gipsVolumeControl->GIPSVE_GetInputMute(getChannelForStreamId(streamId), mute);
	return mute;
}

bool GipsAudioProvider::setDefaultVolume( int volume )
{
	base::AutoLock lock(m_lock);
	defaultVolume = volume;
    return true;
}

int GipsAudioProvider::getDefaultVolume()
{
	base::AutoLock lock(m_lock);
    return defaultVolume;
}

bool GipsAudioProvider::setRingerVolume( int volume )
{
	base::AutoLock lock(m_lock);
	LOG_GIPS_INFO( logTag, "setRingerVolume: volume=%d", volume );
	if (gipsVolumeControl->GIPSVE_SetChannelOutputVolumeScaling(localRingChannel, volume * 0.01f) != -1)
	{
		ringerVolume = volume;
		if(ringGen != NULL)
		{
			ringGen->SetScaleFactor(ringerVolume);
		}
		return true;
	}
	else
	{
		LOG_GIPS_INFO( logTag, "GIPS returned failure from GIPSVE_SetChannelOutputVolumeScaling");
		return false;
	}
}

int GipsAudioProvider::getRingerVolume()
{
	base::AutoLock lock(m_lock);
    return ringerVolume;
}

bool GipsAudioProvider::setVolume( int streamId, int volume )
{
	LOG_GIPS_INFO( logTag, "setVolume: streamId=%d, volume=%d", streamId, volume );
	int channel = getChannelForStreamId( streamId );
	bool returnVal = false;

	if ( channel >= 0 )
	{
		// Input is scaled 0-100.  GIPS scale is 0.0f - 1.0f.  (Larger values are allowable but liable to distortion)
		if (gipsVolumeControl->GIPSVE_SetChannelOutputVolumeScaling(channel, volume * 0.01f) != -1)
		{
			returnVal = true;
		}
		else
		{
			LOG_GIPS_INFO( logTag, "GIPS returned failure from GIPSVE_SetChannelOutputVolumeScaling");
		}
	}
	else
	{
		LOG_GIPS_INFO( logTag, "failed to map stream to channel");
	}
	return returnVal;
}

int  GipsAudioProvider::getVolume( int streamId )
{
	base::AutoLock lock(m_lock);
	float gipsVolume = 0;

	if(gipsVolumeControl->GIPSVE_GetChannelOutputVolumeScaling(getChannelForStreamId(streamId), gipsVolume) != -1)
	{
		// Output is scaled 0-100.  GIPS scale is 0.0f - 1.0f.
		float volume = gipsVolume * 100.0f; // Scale to 0-100 for presentation.
		return (int)(volume + 0.5f);		// And round neatly.
	}
	else
	{
		LOG_GIPS_INFO( logTag, "GIPS returned failure from GIPSVE_GetChannelOutputVolumeScaling");
		return -1;
	}
}

// GIPSVoiceEngineObserver
#if GIPS_VER < 3510
void GipsAudioProvider::CallbackOnTrace(const GIPS::TraceLevel level, const char* message, const int length)
{
	if (strstr(message, "eventNumber=") != NULL || strstr(message, "DTMF event ") != NULL)
		return;
	//if ( length > 0 ) LOG_GIPS_INFO( logTag, "GIPS %2d: %s", level, message );
}
#else
void GipsAudioProvider::Print(const GIPS::TraceLevel level, const char* message, const int length)
{
		if (strstr(message, "eventNumber=") != NULL || strstr(message, "DTMF event ") != NULL)
			return;
		//if ( length > 0 ) LOG_GIPS_INFO( logTag, "GIPS %s", message );
}
#endif
void GipsAudioProvider::CallbackOnError(const int errCode, const int channel)
{
	LOG_GIPS_ERROR( logTag, "GIPS ERROR %d on channel %d", errCode, channel );
}

void GipsAudioProvider::OnPeriodicDeadOrAlive(int channel, bool isAlive)
{
	GipsAudioStreamPtr stream = getStreamByChannel(channel);
	if(stream != NULL && (stream->isRxStarted || stream->isTxStarted))
	{
		if(stream->isAlive != isAlive)
		{
			LOG_GIPS_INFO( logTag, "GIPS channel %d is %s", channel, (isAlive ? "alive" : "dead") );
			stream->isAlive = isAlive;
			// TODO should use postEvent and rely on Engine to drive dispatch.
/*			Component::Event event;
			event.component = provider;
			event.context = (void*)stream->callId;
			stream->isAlive = isAlive;
			if(isAlive)
				event.id = eMediaRestored;
			else
				event.id = eMediaLost;
			provider->dispatchEvent( event );
*/
		}
	}
}
#ifdef WIN32 
bool IsUserAdmin()
{	
	BOOL bAdm = TRUE;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup; 
	bAdm = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup); 
	if(bAdm) 
	{
		if (!CheckTokenMembership( NULL, AdministratorsGroup, &bAdm)) 
		{
			bAdm = FALSE;
		} 
		//Free the memory for PSID structure;
		FreeSid(AdministratorsGroup); 
	}
	return (bAdm == TRUE);
	
}

bool IsVistaOrNewer()
{

    OSVERSIONINFOEX osVersion;
	// Initialize the OSVERSIONINFOEX structure.

	ZeroMemory(&osVersion, sizeof(OSVERSIONINFOEX));

    osVersion.dwOSVersionInfoSize   = sizeof(osVersion);

    if (GetVersionEx((LPOSVERSIONINFO)&osVersion)) {

        // Determine if this is Windows Vista or newer

        // Note the >= check

        if (osVersion.dwMajorVersion >= 6) {

            // Vista
            return TRUE;

        }

    } 

	//Lets proceed with XP as OS.
    return FALSE;

}
#endif
} // namespace CSF

#endif
