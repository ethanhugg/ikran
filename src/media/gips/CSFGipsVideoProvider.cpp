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
#ifndef NO_GIPS_VIDEO

#include "CC_Common.h"
#ifdef LINUX
#include "X11/Xlib.h"
#endif
#include "VcmSIPCCBinding.h"
#include "CSFGipsMediaProvider.h"
#include "CSFGipsAudioProvider.h"
#include "CSFGipsVideoProvider.h"
#include "CSFGipsLogging.h"
#include "GIPSVEEncryption.h"

#include "base/synchronization/lock.h"

#ifdef WIN32
typedef GipsVideoEngineWindows    GipsVideoEnginePlatform;
#elif LINUX
void DeleteGipsVideoEngine(GipsVideoEngine *videoEngine) {}
typedef GipsVideoEngineLinux GipsVideoEnginePlatform;
#else

typedef GipsVideoEngineMacCarbon   GipsVideoEnginePlatform;
// temporary
void DeleteGipsVideoEngine(GipsVideoEngine *videoEngine) {}
#endif

using namespace std;
#include "string.h"

extern "C" void config_get_value (int id, void *buffer, int length);

static const char* logTag = "CSFGipsVideoProvider";

#ifdef LINUX
static    clock_t currentTime, lastRequestTime;
#endif

namespace CSF {

static GIPSVideo_CodecInst H264template;

GipsVideoProvider::GipsVideoProvider( GipsMediaProvider* provider )
: provider(provider), 
  gipsEncryption(NULL),
  videoMode(true), 
  startPort(1024), 
  endPort(65535), 
  previewWindow(NULL), 
  DSCPValue(0)
{
    gipsVideo = &GetGipsVideoEngine();
    gipsVideo->GIPSVideo_Init( provider->getGipsVoiceEngine() );
    //gipsVideo->GIPSVideo_SetTraceCallback( this );
#ifdef ENABLE_GIPS_VIDEO_TRACE
    gipsVideo->GIPSVideo_SetTraceFileName( "GIPSvideotrace.out" );
#endif

#ifdef WIN32
    ((GipsVideoEngineWindows *)gipsVideo)->GIPSVideo_EnableDirectDraw( true );
#endif

    int nCodecs = gipsVideo->GIPSVideo_GetNofCodecs();
    for ( int i = 0; i < nCodecs; i++ )
    {
        gipsVideo->GIPSVideo_GetCodec( i, &H264template );
        LOG_GIPS_DEBUG( logTag, "codec %d %s pltype %d level %d", i, H264template.plname, H264template.pltype, H264template.level );
        if ( strcmp( H264template.plname, "H264" ) == 0 )
        {
            LOG_GIPS_DEBUG( logTag, "codec #%d is H.264", i );
            break;
        }
    }
#ifdef LINUX
    currentTime=lastRequestTime=clock();
#endif
    char name[64];
    memset (name,0,64);
    // for now just use first device, if any
    if ( gipsVideo->GIPSVideo_GetCaptureDevice( 0, name, sizeof(name) ) == 0 )
    {
        LOG_GIPS_DEBUG( logTag, "Using capture device %s", name);
        gipsVideo->GIPSVideo_SetCaptureDevice( name,  sizeof(name) );
        GIPSCameraCapability cap;
        for ( int j = 0; j<5; j++ )
        {
            if ( gipsVideo->GIPSVideo_GetCaptureCapabilities( j, &cap ) != 0 ) break;
            LOG_GIPS_DEBUG( logTag, "type=%d width=%d height=%d maxFPS=%d", cap.type, cap.width, cap.height, cap.maxFPS );
        }
    }
    else
    {
        LOG_GIPS_WARN( logTag, "No camera found");
    }

}

int GipsVideoProvider::init()
{
    GIPSVoiceEngine* voiceEngine = provider->getGipsVoiceEngine();
    if(voiceEngine)
    {
        gipsEncryption = GIPSVEEncryption::GetInterface(voiceEngine);
        if (gipsEncryption == NULL)
        {
            LOG_GIPS_ERROR( logTag, "GipsVideoProvider(): GIPSVEEncryption::GetInterface failed");
            return -1;
        }  
    }
    else
    {
        LOG_GIPS_ERROR( logTag, "GipsVideoProvider(): No GIPS voice engine");
        return -1;
    }

    return 0;
}

GipsVideoProvider::~GipsVideoProvider()
{
    if(gipsEncryption)
    {
        gipsEncryption->Release();
        gipsEncryption = NULL;
    }

    gipsVideo->GIPSVideo_Terminate();
    DeleteGipsVideoEngine( gipsVideo );
}

void GipsVideoProvider::setVideoMode( bool enable )
{
    videoMode = enable;
}

void GipsVideoProvider::setRenderWindow( int streamId, GipsPlatformWindow window )
{
	LOG_GIPS_DEBUG( logTag, "setRenderWindow: streamId= %d, window=%p", streamId, window);
	base::AutoLock lock(streamMapMutex);
	// we always want to erase the old one. If window is non-null, it will be replaced,
	// otherwise passing in a null window value to this function leads to no mapping for this stream.
	streamIdToWindow.erase( streamId );
    if ( window != NULL )
    {
    	streamIdToWindow.insert( std::make_pair(streamId, RenderWindow( window)) );
    	// now that the window is in the map for this stream, refresh the stream with the window
    	setRenderWindowForStreamIdFromMap(streamId);
    	// the assumption is that the rendering may have been stopped when the previous window was destroyed.
    	//gipsVideo->GIPSVideo_StartRender( getChannelForStreamId( streamId ) );
    }
    else
    {
    	// for a null window we want to stop rendering. 
    	gipsVideo->GIPSVideo_StopRender( getChannelForStreamId( streamId ) );
    }
   
}

const GipsVideoProvider::RenderWindow* GipsVideoProvider::getRenderWindow( int streamId )
{
	//base::AutoLock lock(streamMapMutex);
    std::map<int, RenderWindow>::const_iterator it = streamIdToWindow.find( streamId );
    return ( it != streamIdToWindow.end() ) ? &it->second : NULL;
}

void GipsVideoProvider::setPreviewWindow( void* window, int top, int left, int bottom, int right, RenderScaling style )
{
	base::AutoLock lock(m_lock);
	if(this->previewWindow != NULL)
	{
#ifdef WIN32
		//setRenderWindow( 0, (GipsPlatformWindow)window, top, left, bottom, right, style );
		((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddLocalRenderer( NULL, 0, 0.0f, 0.0f, 1.0f, 1.0f );
#elif LINUX
		((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddLocalRenderer( NULL, 0.0f, 0.0f, 1.0f, 1.0f );
#endif
		delete this->previewWindow;
		this->previewWindow = NULL;
	}

	if(window != NULL)
	{
		// Set new window.
#ifdef WIN32
		//setRenderWindow( 0, (GipsPlatformWindow)window, top, left, bottom, right, style );
		((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddLocalRenderer( (HWND)window, 0, 0.0f, 0.0f, 1.0f, 1.0f );
#elif LINUX
		((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddLocalRenderer( (Window)window, 0.0f, 0.0f, 1.0f, 1.0f );
#endif

		this->previewWindow = new RenderWindow( (GipsPlatformWindow)window);
	}
}

void GipsVideoProvider::setRemoteWindow( int streamId, VideoWindowHandle window)
{
	base::AutoLock lock(m_lock);
    setRenderWindow( streamId, (GipsPlatformWindow)window);
}

std::vector<std::string> GipsVideoProvider::getCaptureDevices()
{
	base::AutoLock lock(m_lock);
    char name[128];
    std::vector<std::string> deviceList;

    // '100' is an arbitrary maximum, to defend against an endless loop;
    // in practice, GetCaptureDevice() should return -1 well before that
    for ( int i = 0; i < 100; i++ )
    {
        if ( gipsVideo->GIPSVideo_GetCaptureDevice( i, name, sizeof(name) ) != 0 ) break;
        deviceList.push_back( name );
    }
    return deviceList;
}

bool GipsVideoProvider::setCaptureDevice( const std::string& name )
{
	base::AutoLock lock(m_lock);
    return ( gipsVideo->GIPSVideo_SetCaptureDevice( name.c_str(), (int) name.length() ) == 0 );
}

int GipsVideoProvider::getCodecList( CodecRequestType requestType )
{
	base::AutoLock lock(m_lock);
    return VideoCodecMask_H264;
}

GipsVideoStreamPtr GipsVideoProvider::getStreamByChannel( int channel )
{
	//base::AutoLock lock(streamMapMutex);
    for( std::map<int, GipsVideoStreamPtr>::const_iterator it = streamMap.begin(); it != streamMap.end(); it++ )
    {
        GipsVideoStreamPtr stream = it->second;
        if(stream->channelId == channel)
            return stream;
    }
    return GipsVideoStreamPtr();
}

int GipsVideoProvider::getChannelForStreamId( int streamId )
{
    for( std::map<int, GipsVideoStreamPtr>::const_iterator it = streamMap.begin(); it != streamMap.end(); it++ )
    {
    	GipsVideoStreamPtr stream = it->second;
        if(stream->streamId == streamId)
            return stream->channelId;
    }
    return -1;
}

GipsVideoStreamPtr GipsVideoProvider::getStream( int streamId )
{
	base::AutoLock lock(streamMapMutex);
	std::map<int, GipsVideoStreamPtr>::const_iterator it = streamMap.find( streamId );
	return ( it != streamMap.end() ) ? it->second : GipsVideoStreamPtr();
}

void GipsVideoProvider::setMuteForStreamId( int streamId, bool muteVideo )
{
	GipsVideoStreamPtr stream = getStream(streamId);
	if(muteVideo == true && stream != NULL)
	{
		stream->isMuted = true;
	}
	else if (muteVideo == false && stream != NULL)
	{
		stream->isMuted = false;
	}
}

void GipsVideoProvider::setTxInitiatedForStreamId( int streamId, bool txInitiatedValue )
{
	GipsVideoStreamPtr stream = getStream(streamId);
	if(txInitiatedValue == true && stream != NULL)
	{
		stream->txInitialised = true;
	}
	else if (txInitiatedValue == false && stream != NULL)
	{
		stream->txInitialised = false;
	}
}

int GipsVideoProvider::rxAlloc( int groupId, int streamId, int requestedPort )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "rxAllocVideo: groupId=%d, streamId=%d, requestedPort=%d", groupId, streamId, requestedPort  );
    int channel = gipsVideo->GIPSVideo_CreateChannel();
    if ( channel == -1 )
    {
        LOG_GIPS_DEBUG( logTag, "rxAllocVideo: CreateChannel failed, error %d", gipsVideo->GIPSVideo_GetLastError() );
        return 0;
    }
    if ( gipsVideo->GIPSVideo_SetChannelCallback( channel, this ) != 0 )
    {
        LOG_GIPS_DEBUG( logTag, "rxAllocVideo: SetChannelCallback failed on channel %d, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        return 0;
    }
    gipsVideo->GIPSVideo_EnableKeyFrameRequestCallback( true );

    if ( previewWindow != NULL )
    {
#ifdef WIN32    // temporary
        // TODO: implement render scaling
    	// NDM not sure if this is available on linux - needs to check GIPS v4
    	// maybe this call should be moved to setPreviewWindw, since it does not seem
    	// to be linked to the current stream in any way
        //((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddLocalRenderer( previewWindow, 0, 0.0f, 0.0f, 1.0f, 1.0f );
#endif
    }
    LOG_GIPS_INFO( logTag, "rxAllocVideo: Created channel %d", channel );

    int beginPort;        // where we started
    int tryPort;        // where we are now

    if ( requestedPort == 0  || requestedPort < startPort || requestedPort >= endPort )
        tryPort = startPort;
    else
        tryPort = requestedPort;

    beginPort = tryPort;

    do
    {
        if ( gipsVideo->GIPSVideo_SetLocalReceiver( channel, tryPort, (char *)localIP.c_str() ) == 0 )
        {
            LOG_GIPS_DEBUG( logTag, "rxAllocVideo: Allocated port %d", tryPort );
			GipsVideoStreamPtr stream(new GipsVideoStream(streamId, channel));
			{
				base::AutoLock lock(streamMapMutex);
				streamMap[streamId] = stream;
				LOG_GIPS_DEBUG( logTag, "rxAllocVideo: created stream" );
			}
            return tryPort;
        }

        int errCode = gipsVideo->GIPSVideo_GetLastError();
        if ( errCode == 12061 /* Can't bind socket */ )        
        {
            tryPort += 2;
            if ( tryPort >= endPort )
                tryPort = startPort;
        }
        else
        {
            LOG_GIPS_ERROR( logTag, "rxAllocVideo: SetLocalReceiver returned error %d", errCode );
            gipsVideo->GIPSVideo_DeleteChannel( channel );
            return 0;
        }
    }
    while ( tryPort != beginPort );

    LOG_GIPS_WARN( logTag, "rxAllocVideo: No ports available?" );
    gipsVideo->GIPSVideo_DeleteChannel( channel );
    return 0;
}

int GipsVideoProvider::rxOpen( int groupId, int streamId, int requestedPort, int listenIp, bool isMulticast )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_ERROR( logTag, "rxOpen: groupId=%d, streamId=%d", groupId, streamId);

    int channel = getChannelForStreamId( streamId );
    if ( channel >= 0 )
    {
        int audioChannel = provider->pAudio->getChannelForStreamId(audioStreamId);
        if ( audioChannel >= 0 )
        {
            if ( gipsVideo->GIPSVideo_SetAudioChannel( channel, audioChannel ) != 0 )    // for lip sync
            {
                LOG_GIPS_ERROR( logTag, "rxOpen: SetAudioChannel failed on channel %d, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
            }
        }
        else
        {
            LOG_GIPS_ERROR( logTag, "rxOpen: getChannelForStreamId returned %d", audioChannel);
        }
        return requestedPort;
    }

    return 0;
}

int GipsVideoProvider::rxStart ( int groupId, int streamId, int payloadType, int packPeriod, int localPort, int rfc2833PayloadType,
                                 EncryptionAlgorithm algorithm, unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "rxStartVideo: groupId=%d, streamId=%d, pt=%d", groupId, streamId, payloadType );
    int channel = getChannelForStreamId( streamId );
    if ( channel >= 0 )
    {
        GIPSVideo_CodecInst h264 = H264template;
        h264.pltype = payloadType;
        h264.maxBitRate = 500;
		h264.minBitRate = 300;
		h264.bitRate = 300;
		h264.frameRate = 15;
		h264.level = 12;
        h264.codecSpecific = 0;    
        h264.quality = GIPS_QUALITY_DEFAULT;

        if ( gipsVideo->GIPSVideo_SetReceiveCodec( channel, &h264 ) != 0 )
        {
            LOG_GIPS_ERROR( logTag, "rxStartVideo: SetReceiveCodec on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        }
        if ( gipsVideo->GIPSVideo_SetSendCodec( channel, &h264, true ) != 0 )
        {
 
            LOG_GIPS_ERROR( logTag, "txStartVideo: GIPSVideo_SetSendCodec on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        }
#ifdef LINUX
        // There's a problem with setting certain cameras e.g Cisco VT2 to 15 fps on Linux
        // If we fail then retrying seems to fix it
        else
        {
    		h264.frameRate = 15;
            if (gipsVideo->GIPSVideo_SetSendCodec( channel, &h264, true ) != 0 )
            {
                LOG_GIPS_ERROR( logTag, "txStartVideo: GIPSVideo_SetSendCodec at 30 fps on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
            }
        }
#endif
        switch(algorithm)
        {
            case EncryptionAlgorithm_NONE:
                LOG_GIPS_DEBUG( logTag, "rxStartVideo: using non-secure RTP for channel %d", channel);
                break;

            case EncryptionAlgorithm_AES_128_COUNTER:
            {
                unsigned char key[GIPS_KEY_LENGTH];

                LOG_GIPS_DEBUG( logTag, "rxStartVideo: using secure RTP for channel %d", channel);

                if(!provider->getKey(key, keyLen, salt, saltLen, key, sizeof(key)))
                {
                    LOG_GIPS_ERROR( logTag, "rxStartVideo: failed to generate key on channel %d", channel );
                    return -1;
                }

                if(gipsEncryption->GIPSVE_EnableSRTPReceive(channel,
                    CIPHER_AES_128_COUNTER_MODE,
                    GIPS_CIPHER_LENGTH,
                    AUTH_NULL, 0, 0, ENCRYPTION, key) != 0)
                {
                    LOG_GIPS_ERROR( logTag, "rxStartVideo: GIPSVE_EnableSRTPReceive on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
                    memset(key, 0x00, sizeof(key));
                    return -1;
                }

                memset(key, 0x00, sizeof(key));

                break;
            }  
        }
		//setRenderWindowForStreamIdFromMap(streamId);
        if ( gipsVideo->GIPSVideo_StartRender( channel ) != 0 )
        {
            LOG_GIPS_ERROR( logTag, "rxStartVideo: StartRender on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        }
        else 
        {
        	LOG_GIPS_DEBUG( logTag, "rxStartVideo: Rendering on channel %d", channel );
        }
        gipsVideo->GIPSVideo_Run();

        return 0;
    }
    return -1;
}

void GipsVideoProvider::setRenderWindowForStreamIdFromMap(int streamId)
{
	LOG_GIPS_DEBUG( logTag, "setRenderWindowForStreamIdFromMap, streamId: %d", streamId);
	int channel = getChannelForStreamId( streamId );
    const RenderWindow* remote = getRenderWindow(streamId );
    if ( remote != NULL )
    {
#ifdef WIN32    // temporary
        // TODO: implement render scaling
        if ( ((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddRemoteRenderer( channel, remote->window, 1, 0.0f, 0.0f, 1.0f, 1.0f ) != 0 )
        {
            LOG_GIPS_ERROR( logTag, "setRenderWindowForStreamIdFromMap: AddRemoteRenderer on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        }
#endif
#ifdef LINUX
        // TODO: implement render scaling
        if ( ((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddRemoteRenderer( channel, (Window)remote->window, 0.0f, 0.0f, 1.0f, 1.0f ) != 0 )
        {
            LOG_GIPS_ERROR( logTag, "setRenderWindowForStreamIdFromMap: AddRemoteRenderer on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        }
#endif
#ifdef __APPLE__
                                                     //GIPSVideo_AddRemoteRenderer(int channel, WindowRef remoteWindow, int zOrder, float left, float top, float right, float bottom)
        if ( ((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddRemoteRenderer( channel, (WindowRef)remote->window, 1, 0.0f, 0.0f, 1.0f, 1.0f ) != 0 )
        {
            LOG_GIPS_ERROR( logTag, "setRenderWindowForStreamIdFromMap: AddRemoteRenderer on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        }
#endif
    }
    else
    {
    	LOG_GIPS_ERROR( logTag, "Remote window is NULL" );
    }
}

void GipsVideoProvider::rxClose( int groupId, int streamId)
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "rxCloseVideo: groupId=%d, streamId=%d", groupId, streamId);
    int channel = getChannelForStreamId( streamId );
    if ( channel >= 0 )
    {
        gipsVideo->GIPSVideo_StopRender( channel );
        LOG_GIPS_INFO( logTag, "rxCloseVideo: Stop render on channel %d", channel );
//        gipsVideo->GIPSVideo_Stop();
    }
}

void GipsVideoProvider::rxRelease( int groupId, int streamId, int port )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "rxReleaseVideo: groupId=%d, streamId=%d", groupId, streamId);
    int channel = getChannelForStreamId( streamId );
    if ( channel >= 0 )
    {
        gipsVideo->GIPSVideo_DeleteChannel( channel );
        {
        	base::AutoLock lock(streamMapMutex);
        	streamMap.erase(streamId);
        }
        LOG_GIPS_DEBUG( logTag, "rxReleaseVideo: Delete channel %d, release port %d", channel, port);
    }
}

int GipsVideoProvider::txStart( int groupId, int streamId, int payloadType, int packPeriod, bool vad, short tos,
                                char* remoteIpAddr, int remotePort, int rfc2833PayloadType, EncryptionAlgorithm algorithm,
                                unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party  )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "txStartVideo: groupId=%d, streamId=%d, pt=%d", groupId, streamId, payloadType );
    int channel = getChannelForStreamId( streamId );
    if ( channel >= 0 )
    {
/*        GIPSVideo_CodecInst h264 = H264template;
        h264.pltype =  payloadType;
        h264.maxBitRate = 1700;
		h264.minBitRate = 1000;
		h264.bitRate = 1000;
 		h264.level = 20;
        h264.codecSpecific = 0;    
        h264.quality = GIPS_QUALITY_DEFAULT;

        if ( gipsVideo->GIPSVideo_SetSendCodec( channel, &h264, true ) != 0 )
        {
 
            LOG_GIPS_ERROR( logTag, "txStartVideo: GIPSVideo_SetSendCodec on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
        }
*/
        switch(algorithm)
        {
            case EncryptionAlgorithm_NONE:
                LOG_GIPS_DEBUG( logTag, "txStartVideo: using non-secure RTP for channel %d", channel);
                break;

            case EncryptionAlgorithm_AES_128_COUNTER:
            {
                unsigned char key[GIPS_KEY_LENGTH];

                LOG_GIPS_DEBUG( logTag, "txStartVideo: using secure RTP for channel %d", channel);

                if(!provider->getKey(key, keyLen, salt, saltLen, key, sizeof(key)))
                {
                    LOG_GIPS_ERROR( logTag, "txStartVideo: failed to generate key on channel %d", channel );
                    return -1;
                }

                if(gipsEncryption->GIPSVE_EnableSRTPSend(channel,
                    CIPHER_AES_128_COUNTER_MODE,
                    GIPS_CIPHER_LENGTH,
                    AUTH_NULL, 0, 0, ENCRYPTION, key) != 0)
                {
                    LOG_GIPS_ERROR( logTag, "txStartVideo: GIPSVE_EnableSRTPSend on channel %d failed, error %d", channel, gipsVideo->GIPSVideo_GetLastError() );
                    memset(key, 0x00, sizeof(key));
                    return -1;
                }

                memset(key, 0x00, sizeof(key));

                break;
            }  
        }

        gipsVideo->GIPSVideo_SetSendDestination( channel, remotePort, remoteIpAddr );
        // We might be muted - for example in the case where the call is being resumed, so respect that setting
		GipsVideoStreamPtr stream = getStream(streamId);
		if (stream != NULL && ! stream->isMuted)
    	{
    		gipsVideo->GIPSVideo_StartSend( channel );
    	}
        setTxInitiatedForStreamId(streamId, true);
        LOG_GIPS_DEBUG( logTag, "txStartVideo: Sending to %s:%d on channel %d", remoteIpAddr, remotePort, channel );
        return 0;
    }
    return -1;
}

void GipsVideoProvider::txClose( int groupId, int streamId )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "txCloseVideo: groupId=%d, streamId=%d", groupId, streamId );
    int channel = getChannelForStreamId( streamId );
    if ( channel >= 0 )
    {
        gipsVideo->GIPSVideo_StopSend( channel );
        LOG_GIPS_DEBUG( logTag, "txCloseVideo: Stop transmit on channel %d", channel );
        setTxInitiatedForStreamId(streamId, false);
    }
}

bool GipsVideoProvider::mute(int streamId, bool muteVideo)
{
	base::AutoLock lock(m_lock);
    int channel = getChannelForStreamId( streamId );
    bool returnVal = false;
	GipsVideoStreamPtr stream = getStream(streamId);
    LOG_GIPS_INFO( logTag, "mute: streamId=%d, mute=%d", streamId, muteVideo );
    	
    if ( channel >= 0 )
    {
    	if( muteVideo == true )
    	{
			if (stream->txInitialised)
			{
    			if (gipsVideo->GIPSVideo_StopSend( channel ) != -1)
    			{
    				returnVal= true;
    			}
    			else
    			{
    				LOG_GIPS_ERROR( logTag, "GIPS returned failure from GIPSVideo_StopSend");
    			}
			}
			else
			{
    			returnVal= true;				
			}
    	}
        else
        {
			if (stream->txInitialised)
			{
				if (gipsVideo->GIPSVideo_StartSend( channel ) != -1)
    			{
    				returnVal= true;
    			}
    			else
    			{
    				LOG_GIPS_ERROR( logTag, "GIPS returned failure from GIPSVideo_StartSend");
    			}
			}
			else
			{
				returnVal= true;
			}
        }
    	if (returnVal == true)
    	{
    		setMuteForStreamId( streamId, muteVideo );
    	}
    }
    return returnVal;
}

bool GipsVideoProvider::isMuted(int streamId)
{
	base::AutoLock lock(m_lock);
	GipsVideoStreamPtr stream = getStream(streamId);
	bool returnVal = false;

	if ((stream != NULL) && (stream->isMuted == true))
	{
		returnVal = true;
	}

	return returnVal;
}

bool GipsVideoProvider::setFullScreen(int streamId, bool fullScreen)
{
	base::AutoLock lock(m_lock);
	int returnVal = -1;

#ifdef WIN32
	const RenderWindow* renderWindow = getRenderWindow( streamId );
	if ( renderWindow != NULL )
	{
		if (fullScreen)
		{
			returnVal = ((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddFullScreenRender((HWND)renderWindow->window);
		}
		else
		{
			returnVal = ((GipsVideoEnginePlatform *)gipsVideo)->GIPSVideo_AddFullScreenRender(NULL);  // turn full screen off
		}
	}
#endif
	return (returnVal == 0) ? true : false;
}

void GipsVideoProvider::sendIFrame( int streamId )
{
	base::AutoLock lock(m_lock);
    LOG_GIPS_INFO( logTag, "Remote end requested I-frame %d: " ,streamId );
    int channel = getChannelForStreamId( streamId );
    if ( channel >= 0 )
    {
        gipsVideo->GIPSVideo_SendKeyFrame( channel );
    }
    else
    {
    	LOG_GIPS_INFO( logTag, "sendIFrame: getChannelForStreamId returned %d", channel );
    }
}

void GipsVideoProvider::RequestNewKeyFrame		(int channel)
{
#ifdef LINUX

    double elapsed;

    currentTime = clock();
    elapsed = ((double) (currentTime-lastRequestTime)) / CLOCKS_PER_SEC;
    // If last request was sent less than a second ago we debounce
    if (elapsed <=1)
    {
        LOG_GIPS_INFO(logTag, "Last request was sent less than a second ago - do nothing" );
        return;
    }
#endif

    LOG_GIPS_INFO(logTag, "Send Request for I-frame to originator" );
    GipsVideoStreamPtr stream= getStreamByChannel(channel);
    MediaProviderObserver *mpobs = VcmSIPCCBinding::getMediaProviderObserver();
    if (mpobs != NULL)
        mpobs->onKeyFrameRequested(stream->streamId);
#ifdef LINUX
    lastRequestTime = clock();
#endif
}

void GipsVideoProvider::IncomingRate(int channel, int frameRate, int bitrate)
{
    //LOG_GIPS_INFO( logTag, "IncomingRate channel %d, frameRate %d, bitrate %d", channel, frameRate, bitrate );
}

void GipsVideoProvider::IncomingCodecChanged(int channel, int payloadType, int width, int height)
{
    LOG_GIPS_INFO( logTag, "IncomingCodecChanged channel %d, payloadType %d, width %d, height %d", channel, payloadType, width, height );
}

void GipsVideoProvider::IncomingCSRCChanged(int channel, unsigned int csrc, bool added)
{
//    LOG_GIPS_INFO( logTag, "IncomingRate channel %d, csrc %d, added %d", channel, csrc, added );
}

void GipsVideoProvider::SendRate(int channel, int frameRate, int bitrate)
{
//    LOG_GIPS_INFO( logTag, "SendRate channel %d, frameRate %d, bitrate %d", channel, frameRate, bitrate );
}

void GipsVideoProvider:: setAudioStreamId(int streamId)
{
    audioStreamId=streamId;
}

} // namespace CSF

#endif // NO_GIPS_VIDEO
#endif
