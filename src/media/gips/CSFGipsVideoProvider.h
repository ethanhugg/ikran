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

#ifndef CSFGipsVideoProvider_H_
#define CSFGipsVideoProvider_H_

#ifndef _USE_CPVE

#include <CSFVideoControl.h>
#include <CSFVideoTermination.h>

#include <GIPSVEEncryption.h>

#ifdef WIN32
#include <GIPSVideoEngineWindows.h>
typedef HWND                    GipsPlatformWindow;
#endif
#ifdef LINUX
#include <GipsVideoEngineLinux.h>
typedef void*                    GipsPlatformWindow;    // temporary placeholder
#endif
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <GipsVideoEngineMacCarbon.h>
typedef void*                    GipsPlatformWindow;
#endif

#include <string>
#include <map>

namespace CSF
{

	class GipsVideoStream
	{
	public:
		GipsVideoStream(int _streamId, int _channelId):
			streamId(_streamId), channelId(_channelId),	isMuted(false), txInitialised(false)
			{}
        int streamId;
        int channelId;
        bool isMuted;
        bool txInitialised;
	};

    class GipsMediaProvider;

    DECLARE_PTR(GipsVideoStream);

    class GipsVideoProvider : public VideoControl, VideoTermination, GIPSVideoChannelCallback
    {
    public:
        GipsVideoProvider( GipsMediaProvider* provider );
        ~GipsVideoProvider();

        // initialize members and GIPS API
        // return 0 if initialzation succeeded
        int init();

        // VideoMediaControl
        VideoControl* getMediaControl() { return this; }

        void setVideoMode( bool enable );

        std::vector<std::string> getCaptureDevices();

        std::string getCaptureDevice() { return captureDevice; }
        bool setCaptureDevice( const std::string& name );

        // VideoTermination
        VideoTermination* getMediaTermination() { return this; }

        //VideoTermination APIs
        void setRemoteWindow( int streamId, VideoWindowHandle window );

        // window type is platform-specific
        void setPreviewWindow( VideoWindowHandle window, int top, int left, int bottom, int right, RenderScaling style );
        void showPreviewWindow( bool show ) {}    // TODO

        void sendIFrame    ( int streamId );

        // MediaTermination APIs
        int  getCodecList( CodecRequestType requestType );

        int  rxAlloc    ( int groupId, int streamId, int requestedPort );
        int  rxOpen     ( int groupId, int streamId, int requestedPort, int listenIp, bool isMulticast );
        int  rxStart    ( int groupId, int streamId, int payloadType, int packPeriod, int localPort, int rfc2833PayloadType,
                          EncryptionAlgorithm algorithm, unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party);
        void rxClose    ( int groupId, int streamId );
        void rxRelease  ( int groupId, int streamId, int port );
        int  txStart    ( int groupId, int streamId, int payloadType, int packPeriod, bool vad, short tos,
                          char* remoteIpAddr, int remotePort, int rfc2833PayloadType, EncryptionAlgorithm algorithm,
                          unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party );
        void txClose    ( int groupId, int streamId );

        void setLocalIP    ( const char* addr ) { localIP = addr; }
        void setMediaPorts ( int startPort, int endPort ) { this->startPort = startPort; this->endPort = endPort; }
        void setDSCPValue (int value){this->DSCPValue = value;}
        //void Print(const GIPS::TraceLevel level, const char* message, const int length);

        bool mute(int streamID, bool muteVideo);
        bool isMuted(int streamID);

        bool setFullScreen(int streamId, bool fullScreen);
        void setRenderWindowForStreamIdFromMap(int streamId);
        void setAudioStreamId( int streamId );

    protected:
        // GIPSVideoChannelCallback
        void RequestNewKeyFrame		(int channel);
        void SendRate                ( int channel, int frameRate, int bitrate );
        void IncomingRate            ( int channel, int frameRate, int bitrate );
        void IncomingCodecChanged    ( int channel, int payloadType, int width, int height );
        void IncomingCSRCChanged  	 ( int channel, unsigned int csrc, bool added );

        GipsVideoStreamPtr getStreamByChannel( int channel );

        int getChannelForStreamId( int streamId );
		GipsVideoStreamPtr getStream( int streamId );
		void setMuteForStreamId( int streamId, bool muteVideo );
		void setTxInitiatedForStreamId( int streamId, bool txInitiatedValue );

        class RenderWindow
        {
        public:
            RenderWindow( GipsPlatformWindow window )
                : window(window) {}

            GipsPlatformWindow window;
        };

        void setRenderWindow( int streamId, GipsPlatformWindow window);
        const RenderWindow* getRenderWindow( int streamId );

    private:
        GipsVideoEngine* gipsVideo;
        GipsMediaProvider* provider;
        GIPSVEEncryption* gipsEncryption;
        std::map<int, RenderWindow> streamIdToWindow;
        std::map<int, GipsVideoStreamPtr> streamMap;
        std::string captureDevice;
        std::string localIP;
        bool videoMode;
        int startPort;
        int endPort;
        RenderWindow* previewWindow;
        int DSCPValue;

        // Synchronisation (to avoid data corruption and worse given that so many threads call the media provider)
        // Never use this mutex in a callback from GIPS - high probability of deadlock.
        base::Lock m_lock;
        // This mutex is to be held only for the narrowest possible scope while accessing the stream map
        // (but not while inspecting or changing a stream object).
        // Might be used in northbound and southbound calls.
        base::Lock streamMapMutex;
        int audioStreamId;
    };

} // namespace


#endif
#endif /* CSFGipsVideoProvider_H_ */
