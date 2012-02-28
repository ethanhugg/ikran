// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_impl.h"

#include <limits>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "media/base/filter_collection.h"
#include "media/base/limits.h"
#include "media/base/media_log.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "media/base/video_frame.h"
#include "media/filters/null_audio_renderer.h"
#include "media/filters/video_renderer_base.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebVideoFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebRect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebSize.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebURL.h"
#include "v8/include/v8.h"
#include "webkit/media/buffered_data_source.h"
#include "webkit/media/filter_helpers.h"
#include "webkit/media/simple_data_source.h"
#include "webkit/media/webmediaplayer_delegate.h"
#include "webkit/media/webmediaplayer_proxy.h"
#include "webkit/media/webvideoframe_impl.h"

using WebKit::WebCanvas;
using WebKit::WebRect;
using WebKit::WebSize;
using media::NetworkEvent;
using media::PipelineStatus;

namespace {

// Amount of extra memory used by each player instance reported to V8.
// It is not exact number -- first, it differs on different platforms,
// and second, it is very hard to calculate. Instead, use some arbitrary
// value that will cause garbage collection from time to time. We don't want
// it to happen on every allocation, but don't want 5k players to sit in memory
// either. Looks that chosen constant achieves both goals, at least for audio
// objects. (Do not worry about video objects yet, JS programs do not create
// thousands of them...)
const int kPlayerExtraMemory = 1024 * 1024;
const int kSuhas = 1;
int MAX_SESSION_RETRIES = 3;

// Limits the range of playback rate.
//
// TODO(kylep): Revisit these.
//
// Vista has substantially lower performance than XP or Windows7.  If you speed
// up a video too much, it can't keep up, and rendering stops updating except on
// the time bar. For really high speeds, audio becomes a bottleneck and we just
// use up the data we have, which may not achieve the speed requested, but will
// not crash the tab.
//
// A very slow speed, ie 0.00000001x, causes the machine to lock up. (It seems
// like a busy loop). It gets unresponsive, although its not completely dead.
//
// Also our timers are not very accurate (especially for ogg), which becomes
// evident at low speeds and on Vista. Since other speeds are risky and outside
// the norms, we think 1/16x to 16x is a safe and useful range for now.
const float kMinRate = 0.0625f;
const float kMaxRate = 16.0f;

// Platform independent method for converting and rounding floating point
// seconds to an int64 timestamp.
//
// Refer to https://bugs.webkit.org/show_bug.cgi?id=52697 for details.
base::TimeDelta ConvertSecondsToTimestamp(float seconds) {
  float microseconds = seconds * base::Time::kMicrosecondsPerSecond;
  float integer = ceilf(microseconds);
  float difference = integer - microseconds;

  // Round down if difference is large enough.
  if ((microseconds > 0 && difference > 0.5f) ||
      (microseconds <= 0 && difference >= 0.5f)) {
    integer -= 1.0f;
  }

  // Now we can safely cast to int64 microseconds.
  return base::TimeDelta::FromMicroseconds(static_cast<int64>(integer));
}

}  // namespace

namespace webkit_media {

WebMediaPlayerImpl::WebMediaPlayerImpl(
    WebKit::WebFrame* frame,
    WebKit::WebMediaPlayerClient* client,
    base::WeakPtr<WebMediaPlayerDelegate> delegate,
    media::FilterCollection* collection,
    WebKit::WebAudioSourceProvider* audio_source_provider,
    media::MessageLoopFactory* message_loop_factory,
    MediaStreamClient* media_stream_client,
    media::MediaLog* media_log)
    : frame_(frame),
      network_state_(WebKit::WebMediaPlayer::Empty),
      ready_state_(WebKit::WebMediaPlayer::HaveNothing),
      main_loop_(MessageLoop::current()),
      filter_collection_(collection),
      started_(false),
      message_loop_factory_(message_loop_factory),
      paused_(true),
      seeking_(false),
      playback_rate_(0.0f),
      pending_seek_(false),
      pending_seek_seconds_(0.0f),
      client_(client),
      proxy_(new WebMediaPlayerProxy(main_loop_, this)),
      delegate_(delegate),
      media_stream_client_(media_stream_client),
      media_log_(media_log),
      is_accelerated_compositing_active_(false),
      incremented_externally_allocated_memory_(false),
      audio_source_provider_(audio_source_provider),
      sipcc_renderer_(0),
      isSipSession(false),
      isInSession(false) {
  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_CREATED));

  MessageLoop* pipeline_message_loop =
      message_loop_factory_->GetMessageLoop("PipelineThread");
  CHECK(pipeline_message_loop) << "Failed to create a new thread";
  pipeline_ = new media::Pipeline(pipeline_message_loop, media_log_);
  pipeline_->Init(
      base::Bind(&WebMediaPlayerProxy::PipelineEndedCallback,
                 proxy_.get()),
      base::Bind(&WebMediaPlayerProxy::PipelineErrorCallback,
                 proxy_.get()),
      base::Bind(&WebMediaPlayerProxy::NetworkEventCallback,
                 proxy_.get()));

  // Let V8 know we started new thread if we did not did it yet.
  // Made separate task to avoid deletion of player currently being created.
  // Also, delaying GC until after player starts gets rid of starting lag --
  // collection happens in parallel with playing.
  //
  // TODO(enal): remove when we get rid of per-audio-stream thread.
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&WebMediaPlayerImpl::IncrementExternallyAllocatedMemory,
                 AsWeakPtr()));

  is_accelerated_compositing_active_ =
      frame->view()->isAcceleratedCompositingActive();

  // Also we want to be notified of |main_loop_| destruction.
  main_loop_->AddDestructionObserver(this);

  // Create default video renderer.
  scoped_refptr<media::VideoRendererBase> video_renderer =
      new media::VideoRendererBase(
          base::Bind(&WebMediaPlayerProxy::Repaint, proxy_),
          base::Bind(&WebMediaPlayerProxy::SetOpaque, proxy_.get()));
  filter_collection_->AddVideoRenderer(video_renderer);
  proxy_->set_frame_provider(video_renderer);

  // Create default audio renderer.
  filter_collection_->AddAudioRenderer(new media::NullAudioRenderer());

  //Initiliaze sip renderer
   sipcc_renderer_ = SipccRendererImpl::GetInstance();

   sipcc_renderer_->Initialize(main_loop_);
   session_retry_counter = 0;
}

WebMediaPlayerImpl::~WebMediaPlayerImpl() {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  Destroy();
  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_DESTROYED));

  if (delegate_)
    delegate_->PlayerGone(this);

  // Finally tell the |main_loop_| we don't want to be notified of destruction
  // event.
  if (main_loop_) {
    main_loop_->RemoveDestructionObserver(this);
  }
}

namespace {

// Helper enum for reporting scheme histograms.
enum URLSchemeForHistogram {
  kUnknownURLScheme,
  kMissingURLScheme,
  kHttpURLScheme,
  kHttpsURLScheme,
  kFtpURLScheme,
  kChromeExtensionURLScheme,
  kJavascriptURLScheme,
  kFileURLScheme,
  kBlobURLScheme,
  kDataURLScheme,
  kMaxURLScheme = kDataURLScheme  // Must be equal to highest enum value.
};

URLSchemeForHistogram URLScheme(const GURL& url) {
  if (!url.has_scheme()) return kMissingURLScheme;
  if (url.SchemeIs("http")) return kHttpURLScheme;
  if (url.SchemeIs("https")) return kHttpsURLScheme;
  if (url.SchemeIs("ftp")) return kFtpURLScheme;
  if (url.SchemeIs("chrome-extension")) return kChromeExtensionURLScheme;
  if (url.SchemeIs("javascript")) return kJavascriptURLScheme;
  if (url.SchemeIs("file")) return kFileURLScheme;
  if (url.SchemeIs("blob")) return kBlobURLScheme;
  if (url.SchemeIs("data")) return kDataURLScheme;
  return kUnknownURLScheme;
}

}  // anonymous namespace

void WebMediaPlayerImpl::load(const WebKit::WebURL& url) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  GURL gurl(url);
  UMA_HISTOGRAM_ENUMERATION("Media.URLScheme", URLScheme(gurl), kMaxURLScheme);

  // Handle any volume/preload changes that occured before load().
  setVolume(GetClient()->volume());
  setPreload(GetClient()->preload());

  SetNetworkState(WebKit::WebMediaPlayer::Loading);
  SetReadyState(WebKit::WebMediaPlayer::HaveNothing);
  media_log_->AddEvent(media_log_->CreateLoadEvent(url.spec()));



  // Media streams pipelines can start immediately.
  //if (BuildMediaStreamCollection(url, media_stream_client_,
   //                              message_loop_factory_.get(),
    //                             filter_collection_.get(),
     //                            sipcc_renderer_)) {
	 //let's kick off the registration
    //LOG(INFO) << "Suhas:WebMediaPlayer:Load: Let's SIP Register ";
    //SipRegistrationCallbackObj = base::Bind(&WebMediaPlayerImpl::SipRegistrationCallback,base::Unretained(this));
    //SipSessionCallbackObj = base::Bind(&WebMediaPlayerImpl::SipSessionCallback,base::Unretained(this));
    //sipcc_renderer_->AddRegistrationCallback(&SipRegistrationCallbackObj);
    //sipcc_renderer_->AddSessionCallback(&SipSessionCallbackObj);
    //sipcc_renderer_->Register("snandaku","snandaku","172.27.190.5");

    //StartPipeline(gurl);
    return;
  //}

  // Media source pipelines can start immediately.
  if (BuildMediaSourceCollection(url, GetClient()->sourceURL(), proxy_,
                                 message_loop_factory_.get(),
                                 filter_collection_.get())) {
    StartPipeline(gurl);
    return;
  }

  // Otherwise it's a regular request which requires resolving the URL first.
  scoped_refptr<WebDataSource> data_source;
  if (gurl.SchemeIs(kDataScheme)) {
    data_source = new SimpleDataSource(main_loop_, frame_);
  } else {
    data_source = new BufferedDataSource(main_loop_, frame_, media_log_);
  }
  proxy_->set_data_source(data_source);
  data_source->Initialize(url, base::Bind(
      &WebMediaPlayerImpl::DataSourceInitialized,
      base::Unretained(this), gurl));
}

//Suhas: RTCWeb Session Support
void WebMediaPlayerImpl::load(const WebKit::WebURL& url,
							  const WebKit::WebCString src,
                              const WebKit::WebCString aor,
                              const WebKit::WebCString creds,
                              const WebKit::WebCString proxy,
			     			  const WebKit::WebCString dn ) {

  DCHECK(MessageLoop::current() == main_loop_);
  GURL gurl(url);

  // Handle any volume/preload changes that occured before load().
  setVolume(GetClient()->volume());
  setPreload(GetClient()->preload());
  sip_reg_state_ = WebKit::WebMediaPlayer::NoRegistrar;
  sip_session_state_ = WebKit::WebMediaPlayer::NoSession;

  SetNetworkState(WebKit::WebMediaPlayer::Loading);
  SetReadyState(WebKit::WebMediaPlayer::HaveNothing);
  media_log_->AddEvent(media_log_->CreateLoadEvent(url.spec()));
  LOG(INFO) << "Webmediaplayer Session Src " << src.data();
  LOG(INFO) << "Webmediaplayer Session Aor " << aor.data();
  LOG(INFO) << "Webmediaplayer Session Cred " << creds.data();
  LOG(INFO) << "Webmediaplayer Session Proxy" << proxy.data();
  LOG(INFO) << "Webmediaplayer Session DN " << dn.data();

  isSipSession = true;
  dial_number_ = dn.data();
  std::string session_src = src.data();
  bool isLocal = true;
  size_t foundLocal = session_src.find("local");
  size_t foundRemote = session_src.find("remote");

  if(foundLocal != std::string::npos){
	LOG(INFO)<<"WebMediaPlaer: Found Local label on the stream ";
	isLocal = true;
  }
  else if (foundRemote != std::string::npos) {
	LOG(INFO)<<"WebMediaPlaer: Found Remote label on the stream ";
	isLocal = false;
  }

  // Media streams pipelines can start immediately.
  if (BuildMediaStreamCollection(url, media_stream_client_,
                                 message_loop_factory_.get(),
                                 filter_collection_.get(),
                                 sipcc_renderer_,isLocal)) {
     //let's kick off the registration
    LOG(INFO) << "Suhas:WebMediaPlayer:Load: Let's SIP Register ";
    SipRegistrationCallbackObj = base::Bind(&WebMediaPlayerImpl::SipRegistrationCallback,base::Unretained(this));
    SipSessionCallbackObj = base::Bind(&WebMediaPlayerImpl::SipSessionCallback,base::Unretained(this));
    sipcc_renderer_->AddRegistrationCallback(&SipRegistrationCallbackObj);
    sipcc_renderer_->AddSessionCallback(&SipSessionCallbackObj);
    sipcc_renderer_->Register(aor.data(),creds.data(),proxy.data(),isLocal);
    StartPipeline(gurl);
    return;
  }

  LOG(INFO) << "Suhas:WebMediaPlayer:Load: Done";

}

void WebMediaPlayerImpl::cancelLoad() {
  DCHECK_EQ(main_loop_, MessageLoop::current());
}

void WebMediaPlayerImpl::play() {
  DCHECK_EQ(main_loop_, MessageLoop::current());
   LOG(INFO) << " Suhas: WebMediaPlayer:: Play ";
   LOG(INFO) << " Suhas: WebMediaPlayer:: Play: Attempting to place call ";
   sipcc_renderer_->PlaceCall("7222");
  paused_ = false;
  pipeline_->SetPlaybackRate(playback_rate_);
  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));

  if (delegate_)
    delegate_->DidPlay(this);

  /***
	if(isSipSession == false)
	{
  		paused_ = false;
  		pipeline_->SetPlaybackRate(playback_rate_);
	} else
	{
        LOG(INFO) << " Suhas: WebMediaPlayer:: Play: Attempting to place call ";
        sipcc_renderer_->PlaceCall("7222");

	}

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));

  if (delegate_)
    delegate_->DidPlay(this);
  **/
}

void WebMediaPlayerImpl::openSession(const WebKit::WebCString dial_number) 
{
  DCHECK(MessageLoop::current() == main_loop_);
 LOG(ERROR) << " WebMediaPlayer : OpenSession: Current Sip Session State : " << sip_session_state_;

 if(sip_session_state_ ==  WebKit::WebMediaPlayer::AcceptingSession)
 {
 	LOG(ERROR) << " WebMediaPlayer : OpenSession: Accepting Incoming Call  " ;
	sipcc_renderer_->AnswerCall(); 
  	paused_ = false;
 	pipeline_->SetPlaybackRate(playback_rate_);
  	media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));

	if(delegate_)
		delegate_->DidPlay(this);

 } else {
 	LOG(ERROR)<<"WebmediaPlayer:: openession : Calling place call ";
   	sipcc_renderer_->PlaceCall(dial_number.data());
  	paused_ = false;
 	pipeline_->SetPlaybackRate(playback_rate_);
  	media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));

	if(delegate_)
		delegate_->DidPlay(this);

 } 

}


void WebMediaPlayerImpl::SipRegistrationCallback() 
{
	LOG(ERROR) << " WebmediaPlayer:: SipRegistrationCallback ";
    switch( sipcc_renderer_->sipRegState() )
    {
	case 0 :
		LOG(ERROR) << " WebmediaPlayer:: SipRegistrationCallback:  WebKit::WebMediaPlayer::NoRegistrar";
		sip_reg_state_ =  WebKit::WebMediaPlayer::NoRegistrar;
	break;
	case 1:
		LOG(ERROR) << " WebmediaPlayer:: SipRegistrationCallback:  WebKit::WebMediaPlayer::Registering";
		sip_reg_state_ =  WebKit::WebMediaPlayer::Registering;
	break;
	case 2:
	LOG(ERROR) << " WebmediaPlayer:: SipRegistrationCallback:  WebKit::WebMediaPlayer::Registered ";
		sip_reg_state_ =  WebKit::WebMediaPlayer::Registered;
	break;
	case 3:
		LOG(ERROR) << " WebmediaPlayer:: SipRegistrationCallback:  WebKit::WebMediaPlayer::RegistrationFailed";
		sip_reg_state_ =  WebKit::WebMediaPlayer::RegistrationFailed;
	break;	
	case 4:
		LOG(ERROR) << " WebmediaPlayer:: SipRegistrationCallback:  WebKit::WebMediaPlayer::RegistrationInvalid";
		sip_reg_state_ =  WebKit::WebMediaPlayer::RegistrationInvalid;
	break;

    }

    GetClient()->sipRegStateChanged();
}


void WebMediaPlayerImpl::SipSessionCallback() {
    LOG(ERROR) << " WebmediaPlayer:: SipSessionCallback";
	switch( sipcc_renderer_->sipSessionState()) {
    case 0:
   		LOG(ERROR) << "WebMediaPlayerImpl::SipSessionCallback:NoSession ";
		sip_session_state_ =  WebKit::WebMediaPlayer::NoSession;
 	break;
    case 1:
   		LOG(ERROR) << "WebMediaPlayerImpl::SipSessionCallback:OpeningSession";
		sip_session_state_ =  WebKit::WebMediaPlayer::OpeningSession;
 	break;
    case 2:
   		LOG(ERROR) << "WebMediaPlayerImpl::SipSessionCallback: Incoming Session";
		sip_session_state_ =  WebKit::WebMediaPlayer::AcceptingSession;
        caller_ = sipcc_renderer_->callerName();
        if(caller_.length() == 0)
		caller_="No Name";
        caller_number_ = sipcc_renderer_->callerNumber(); 

 	break;
    case 3:
        LOG(ERROR) << "WebMediaPlayerImpl::SipSessionCallback: In Session";
		sip_session_state_ =  WebKit::WebMediaPlayer::InSession;
 	break;
    case 4:
   		 LOG(ERROR) << "WebMediaPlayerImpl::SipSessionCallback: ClosingSession";
		sip_session_state_ =  WebKit::WebMediaPlayer::ClosingSession;
 	break;
    case 5:
   		 LOG(ERROR) << "WebMediaPlayerImpl::SipSessionCallback: SessionInvalid";
		sip_session_state_ =  WebKit::WebMediaPlayer::SessionInvalid;
    }

    GetClient()->sipSessionStateChanged();
}


void WebMediaPlayerImpl::closeSession() {
  DCHECK(MessageLoop::current() == main_loop_);
  LOG(ERROR) << "WebMediaPlayerImpl::closeSession ";
  sipcc_renderer_->HangUp();

}

void WebMediaPlayerImpl::clearSession() {
  DCHECK(MessageLoop::current() == main_loop_);
  LOG(ERROR) << "WebMediaPlayerImpl::clearSession ";
  sipcc_renderer_->UnRegister();
}


void WebMediaPlayerImpl::pause() {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  LOG(INFO) << "Suhas:WebMediaPlayer:: Pause ";
  paused_ = true;
  pipeline_->SetPlaybackRate(0.0f);
  paused_time_ = pipeline_->GetCurrentTime();

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PAUSE));

  if (delegate_)
    delegate_->DidPause(this);
}

bool WebMediaPlayerImpl::supportsFullscreen() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  return true;
}

bool WebMediaPlayerImpl::supportsSave() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  return true;
}

void WebMediaPlayerImpl::seek(float seconds) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // WebKit fires a seek(0) at the very start, however pipeline already does a
  // seek(0) internally.  Avoid doing seek(0) the second time because this will
  // cause extra pre-rolling and will break servers without range request
  // support.
  //
  // We still have to notify WebKit that time has changed otherwise
  // HTMLMediaElement gets into an inconsistent state.
  if (pipeline_->GetCurrentTime().ToInternalValue() == 0 && seconds == 0) {
    GetClient()->timeChanged();
    return;
  }

  if (seeking_) {
    pending_seek_ = true;
    pending_seek_seconds_ = seconds;
    return;
  }

  media_log_->AddEvent(media_log_->CreateSeekEvent(seconds));

  base::TimeDelta seek_time = ConvertSecondsToTimestamp(seconds);

  // Update our paused time.
  if (paused_)
    paused_time_ = seek_time;

  seeking_ = true;

  proxy_->DemuxerFlush();

  // Kick off the asynchronous seek!
  pipeline_->Seek(
      seek_time,
      base::Bind(&WebMediaPlayerProxy::PipelineSeekCallback,
                 proxy_.get()));
}

void WebMediaPlayerImpl::setEndTime(float seconds) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // TODO(hclam): add method call when it has been implemented.
  return;
}

void WebMediaPlayerImpl::setRate(float rate) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // TODO(kylep): Remove when support for negatives is added. Also, modify the
  // following checks so rewind uses reasonable values also.
  if (rate < 0.0f)
    return;

  // Limit rates to reasonable values by clamping.
  if (rate != 0.0f) {
    if (rate < kMinRate)
      rate = kMinRate;
    else if (rate > kMaxRate)
      rate = kMaxRate;
  }

  playback_rate_ = rate;
  if (!paused_) {
    pipeline_->SetPlaybackRate(rate);
  }
}

void WebMediaPlayerImpl::setVolume(float volume) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  pipeline_->SetVolume(volume);
}

void WebMediaPlayerImpl::setVisible(bool visible) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // TODO(hclam): add appropriate method call when pipeline has it implemented.
  return;
}

#define COMPILE_ASSERT_MATCHING_ENUM(webkit_name, chromium_name) \
    COMPILE_ASSERT(static_cast<int>(WebKit::WebMediaPlayer::webkit_name) == \
                   static_cast<int>(media::chromium_name), \
                   mismatching_enums)
COMPILE_ASSERT_MATCHING_ENUM(None, NONE);
COMPILE_ASSERT_MATCHING_ENUM(MetaData, METADATA);
COMPILE_ASSERT_MATCHING_ENUM(Auto, AUTO);

void WebMediaPlayerImpl::setPreload(WebKit::WebMediaPlayer::Preload preload) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  pipeline_->SetPreload(static_cast<media::Preload>(preload));
}

bool WebMediaPlayerImpl::totalBytesKnown() {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  return pipeline_->GetTotalBytes() != 0;
}

bool WebMediaPlayerImpl::hasVideo() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  return pipeline_->HasVideo();
}

bool WebMediaPlayerImpl::hasAudio() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  return pipeline_->HasAudio();
}

WebKit::WebSize WebMediaPlayerImpl::naturalSize() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  gfx::Size size;
  pipeline_->GetNaturalVideoSize(&size);
  return WebKit::WebSize(size);
}

bool WebMediaPlayerImpl::paused() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  return pipeline_->GetPlaybackRate() == 0.0f;
}

bool WebMediaPlayerImpl::seeking() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  if (ready_state_ == WebKit::WebMediaPlayer::HaveNothing)
    return false;

  return seeking_;
}

float WebMediaPlayerImpl::duration() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  base::TimeDelta duration = pipeline_->GetMediaDuration();

  // Return positive infinity if the resource is unbounded.
  // http://www.whatwg.org/specs/web-apps/current-work/multipage/video.html#dom-media-duration
  if (duration == media::kInfiniteDuration())
    return std::numeric_limits<float>::infinity();

  return static_cast<float>(duration.InSecondsF());
}

float WebMediaPlayerImpl::currentTime() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  if (paused_)
    return static_cast<float>(paused_time_.InSecondsF());
  return static_cast<float>(pipeline_->GetCurrentTime().InSecondsF());
}

int WebMediaPlayerImpl::dataRate() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // TODO(hclam): Add this method call if pipeline has it in the interface.
  return 0;
}

WebKit::WebMediaPlayer::NetworkState WebMediaPlayerImpl::networkState() const {
  return network_state_;
}

WebKit::WebMediaPlayer::ReadyState WebMediaPlayerImpl::readyState() const {
  return ready_state_;
}

WebKit::WebMediaPlayer::SipRegistrationState WebMediaPlayerImpl::sipRegistrationState() const {
  LOG(ERROR) << "WebMediaPlayer::SipRegistrationState -- " << sip_reg_state_;
  return sip_reg_state_;
}

WebKit::WebMediaPlayer::SipSessionState WebMediaPlayerImpl::sipSessionState() const {
  LOG(ERROR) << "WebMediaPlayer::sipSessionState-- " << sip_session_state_;
  return sip_session_state_;
}

WebKit::WebCString WebMediaPlayerImpl::callingPartyName() const {

  LOG(ERROR) << "WebMediaPlayer::callingPartyName-- " << caller_;
  WebKit::WebCString caller_name(caller_.c_str(), caller_.length());
  	return caller_name;
}


WebKit::WebCString WebMediaPlayerImpl::callingPartyNumber() const {

  LOG(ERROR) << "WebMediaPlayer::callingPartyNumber-- " << caller_number_;
  WebKit::WebCString caller_number(caller_number_.c_str(), caller_number_.length());
  	return caller_number;

}


const WebKit::WebTimeRanges& WebMediaPlayerImpl::buffered() {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // Update buffered_ with the most recent buffered time.
  if (buffered_.size() > 0) {
    float buffered_time = static_cast<float>(
        pipeline_->GetBufferedTime().InSecondsF());
    if (buffered_time >= buffered_[0].start)
      buffered_[0].end = buffered_time;
  }

  return buffered_;
}

float WebMediaPlayerImpl::maxTimeSeekable() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // If we are performing streaming, we report that we cannot seek at all.
  // We are using this flag to indicate if the data source supports seeking
  // or not. We should be able to seek even if we are performing streaming.
  // TODO(hclam): We need to update this when we have better caching.
  if (pipeline_->IsStreaming())
    return 0.0f;
  return static_cast<float>(pipeline_->GetMediaDuration().InSecondsF());
}

unsigned long long WebMediaPlayerImpl::bytesLoaded() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  return pipeline_->GetBufferedBytes();
}

unsigned long long WebMediaPlayerImpl::totalBytes() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  return pipeline_->GetTotalBytes();
}

void WebMediaPlayerImpl::setSize(const WebSize& size) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // Don't need to do anything as we use the dimensions passed in via paint().
}

void WebMediaPlayerImpl::paint(WebCanvas* canvas,
                               const WebRect& rect) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  DCHECK(proxy_);

#if WEBKIT_USING_SKIA
  proxy_->Paint(canvas, rect);
#elif WEBKIT_USING_CG
  // Get the current scaling in X and Y.
  CGAffineTransform mat = CGContextGetCTM(canvas);
  float scale_x = sqrt(mat.a * mat.a + mat.b * mat.b);
  float scale_y = sqrt(mat.c * mat.c + mat.d * mat.d);
  float inverse_scale_x = SkScalarNearlyZero(scale_x) ? 0.0f : 1.0f / scale_x;
  float inverse_scale_y = SkScalarNearlyZero(scale_y) ? 0.0f : 1.0f / scale_y;
  int scaled_width = static_cast<int>(rect.width * fabs(scale_x));
  int scaled_height = static_cast<int>(rect.height * fabs(scale_y));

  // Make sure we don't create a huge canvas.
  // TODO(hclam): Respect the aspect ratio.
  if (scaled_width > static_cast<int>(media::limits::kMaxCanvas))
    scaled_width = media::limits::kMaxCanvas;
  if (scaled_height > static_cast<int>(media::limits::kMaxCanvas))
    scaled_height = media::limits::kMaxCanvas;

  // If there is no preexisting platform canvas, or if the size has
  // changed, recreate the canvas.  This is to avoid recreating the bitmap
  // buffer over and over for each frame of video.
  if (!skia_canvas_.get() ||
      skia_canvas_->getDevice()->width() != scaled_width ||
      skia_canvas_->getDevice()->height() != scaled_height) {
    skia_canvas_.reset(
        new skia::PlatformCanvas(scaled_width, scaled_height, true));
  }

  // Draw to our temporary skia canvas.
  gfx::Rect normalized_rect(scaled_width, scaled_height);
  proxy_->Paint(skia_canvas_.get(), normalized_rect);

  // The mac coordinate system is flipped vertical from the normal skia
  // coordinates.  During painting of the frame, flip the coordinates
  // system and, for simplicity, also translate the clip rectangle to
  // start at 0,0.
  CGContextSaveGState(canvas);
  CGContextTranslateCTM(canvas, rect.x, rect.height + rect.y);
  CGContextScaleCTM(canvas, inverse_scale_x, -inverse_scale_y);

  // We need a local variable CGRect version for DrawToContext.
  CGRect normalized_cgrect =
      CGRectMake(normalized_rect.x(), normalized_rect.y(),
                 normalized_rect.width(), normalized_rect.height());

  // Copy the frame rendered to our temporary skia canvas onto the passed in
  // canvas.
  skia::DrawToNativeContext(skia_canvas_.get(), canvas, 0, 0,
                            &normalized_cgrect);

  CGContextRestoreGState(canvas);
#else
  NOTIMPLEMENTED() << "We only support rendering to skia or CG";
#endif
}

bool WebMediaPlayerImpl::hasSingleSecurityOrigin() const {
  if (proxy_)
    return proxy_->HasSingleOrigin();
  return true;
}

WebKit::WebMediaPlayer::MovieLoadType
    WebMediaPlayerImpl::movieLoadType() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // TODO(hclam): If the pipeline is performing streaming, we say that this is
  // a live stream. But instead it should be a StoredStream if we have proper
  // caching.
  if (pipeline_->IsStreaming())
    return WebKit::WebMediaPlayer::LiveStream;
  return WebKit::WebMediaPlayer::Unknown;
}

float WebMediaPlayerImpl::mediaTimeForTimeValue(float timeValue) const {
  return ConvertSecondsToTimestamp(timeValue).InSecondsF();
}

unsigned WebMediaPlayerImpl::decodedFrameCount() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  media::PipelineStatistics stats = pipeline_->GetStatistics();
  return stats.video_frames_decoded;
}

unsigned WebMediaPlayerImpl::droppedFrameCount() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  media::PipelineStatistics stats = pipeline_->GetStatistics();
  return stats.video_frames_dropped;
}

unsigned WebMediaPlayerImpl::audioDecodedByteCount() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  media::PipelineStatistics stats = pipeline_->GetStatistics();
  return stats.audio_bytes_decoded;
}

unsigned WebMediaPlayerImpl::videoDecodedByteCount() const {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  media::PipelineStatistics stats = pipeline_->GetStatistics();
  return stats.video_bytes_decoded;
}

WebKit::WebVideoFrame* WebMediaPlayerImpl::getCurrentFrame() {
  scoped_refptr<media::VideoFrame> video_frame;
  proxy_->GetCurrentFrame(&video_frame);
  if (video_frame.get())
    return new WebVideoFrameImpl(video_frame);
  return NULL;
}

void WebMediaPlayerImpl::putCurrentFrame(
    WebKit::WebVideoFrame* web_video_frame) {
  if (web_video_frame) {
    scoped_refptr<media::VideoFrame> video_frame(
        WebVideoFrameImpl::toVideoFrame(web_video_frame));
    proxy_->PutCurrentFrame(video_frame);
    delete web_video_frame;
  } else {
    proxy_->PutCurrentFrame(NULL);
  }
}

bool WebMediaPlayerImpl::sourceAppend(const unsigned char* data,
                                      unsigned length) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  return proxy_->DemuxerAppend(data, length);
}

void WebMediaPlayerImpl::sourceEndOfStream(
    WebKit::WebMediaPlayer::EndOfStreamStatus status) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  media::PipelineStatus pipeline_status = media::PIPELINE_OK;

  switch(status) {
    case WebKit::WebMediaPlayer::EosNoError:
      break;
    case WebKit::WebMediaPlayer::EosNetworkError:
      pipeline_status = media::PIPELINE_ERROR_NETWORK;
      break;
    case WebKit::WebMediaPlayer::EosDecodeError:
      pipeline_status = media::PIPELINE_ERROR_DECODE;
      break;
    default:
      NOTIMPLEMENTED();
  }

  proxy_->DemuxerEndOfStream(pipeline_status);
}

void WebMediaPlayerImpl::WillDestroyCurrentMessageLoop() {
  Destroy();
  main_loop_ = NULL;
}

void WebMediaPlayerImpl::Repaint() {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  GetClient()->repaint();
}

void WebMediaPlayerImpl::OnPipelineInitialize(PipelineStatus status) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  if (status == media::PIPELINE_OK) {
    LOG(INFO) << "Suhas: WebMediaPlayerImpl: OnPipielineInitialize ";
    // Only keep one time range starting from 0.
    WebKit::WebTimeRanges new_buffered(static_cast<size_t>(1));
    new_buffered[0].start = 0.0f;
    new_buffered[0].end =
        static_cast<float>(pipeline_->GetMediaDuration().InSecondsF());
    buffered_.swap(new_buffered);

    if (hasVideo()) {
      UMA_HISTOGRAM_BOOLEAN("Media.AcceleratedCompositingActive",
                            is_accelerated_compositing_active_);
    } else {
      GetClient()->disableAcceleratedCompositing();
    }

    if (pipeline_->IsLocalSource()) {
	  LOG(INFO) << "Suhas:WebMediaPlayer::OnPipelineInitialize: Setting NETWORK State to loaded ";	
      SetNetworkState(WebKit::WebMediaPlayer::Loaded);
    }

    SetReadyState(WebKit::WebMediaPlayer::HaveMetadata);
    // Fire canplaythrough immediately after playback begins because of
    // crbug.com/106480.
    // TODO(vrk): set ready state to HaveFutureData when bug above is fixed.
	LOG(INFO) << "Suhas:WebMediaPlayer::OnPipelineInitialize: Setting READY State to HaveEnoughData";	
    SetReadyState(WebKit::WebMediaPlayer::HaveEnoughData);
  } else {
    // TODO(hclam): should use |status| to determine the state
    // properly and reports error using MediaError.
    // WebKit uses FormatError to indicate an error for bogus URL or bad file.
    // Since we are at the initialization stage we can safely treat every error
    // as format error. Should post a task to call to |webmediaplayer_|.
    SetNetworkState(WebKit::WebMediaPlayer::FormatError);
  }

  // Repaint to trigger UI update.
  Repaint();
}

void WebMediaPlayerImpl::OnPipelineSeek(PipelineStatus status) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  seeking_ = false;
  if (pending_seek_) {
    pending_seek_ = false;
    seek(pending_seek_seconds_);
    return;
  }

  if (status == media::PIPELINE_OK) {
    // Update our paused time.
    if (paused_)
      paused_time_ = pipeline_->GetCurrentTime();

    GetClient()->timeChanged();
  }
}

void WebMediaPlayerImpl::OnPipelineEnded(PipelineStatus status) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  LOG(INFO) << "Suhas:WebMediaPlayer::OnPipelineEnded ";
  if (status == media::PIPELINE_OK)
    GetClient()->timeChanged();
}

void WebMediaPlayerImpl::OnPipelineError(PipelineStatus error) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  switch (error) {
    case media::PIPELINE_OK:
      LOG(DFATAL) << "PIPELINE_OK isn't an error!";
      break;

    case media::PIPELINE_ERROR_NETWORK:
      SetNetworkState(WebMediaPlayer::NetworkError);
      break;

    case media::PIPELINE_ERROR_INITIALIZATION_FAILED:
    case media::PIPELINE_ERROR_REQUIRED_FILTER_MISSING:
    case media::PIPELINE_ERROR_COULD_NOT_RENDER:
    case media::PIPELINE_ERROR_URL_NOT_FOUND:
    case media::PIPELINE_ERROR_READ:
    case media::DEMUXER_ERROR_COULD_NOT_OPEN:
    case media::DEMUXER_ERROR_COULD_NOT_PARSE:
    case media::DEMUXER_ERROR_NO_SUPPORTED_STREAMS:
    case media::DEMUXER_ERROR_COULD_NOT_CREATE_THREAD:
    case media::DECODER_ERROR_NOT_SUPPORTED:
    case media::DATASOURCE_ERROR_URL_NOT_SUPPORTED:
      // Format error.
      SetNetworkState(WebMediaPlayer::FormatError);
      break;

    case media::PIPELINE_ERROR_DECODE:
    case media::PIPELINE_ERROR_ABORT:
    case media::PIPELINE_ERROR_OUT_OF_MEMORY:
    case media::PIPELINE_ERROR_AUDIO_HARDWARE:
    case media::PIPELINE_ERROR_OPERATION_PENDING:
    case media::PIPELINE_ERROR_INVALID_STATE:
      // Decode error.
      SetNetworkState(WebMediaPlayer::DecodeError);
      break;
  }

  // Repaint to trigger UI update.
  Repaint();
}

void WebMediaPlayerImpl::OnNetworkEvent(NetworkEvent type) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  switch(type) {
    case media::DOWNLOAD_CONTINUED:
      SetNetworkState(WebKit::WebMediaPlayer::Loading);
      break;
    case media::DOWNLOAD_PAUSED:
      SetNetworkState(WebKit::WebMediaPlayer::Idle);
      break;
    case media::CAN_PLAY_THROUGH:
      // Temporarily disable delayed firing of CAN_PLAY_THROUGH due to
      // crbug.com/106480.
      // TODO(vrk): uncomment code below when bug above is fixed.
      // SetReadyState(WebKit::WebMediaPlayer::HaveEnoughData);
      break;
    default:
      NOTREACHED();
  }
}

void WebMediaPlayerImpl::OnDemuxerOpened() {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  GetClient()->sourceOpened();
}

void WebMediaPlayerImpl::SetOpaque(bool opaque) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  GetClient()->setOpaque(opaque);
}

void WebMediaPlayerImpl::DataSourceInitialized(
    const GURL& gurl,
    media::PipelineStatus status) {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  if (status != media::PIPELINE_OK) {
    SetNetworkState(WebKit::WebMediaPlayer::FormatError);
    Repaint();
    return;
  }

  BuildDefaultCollection(proxy_->data_source(),
                         message_loop_factory_.get(),
                         filter_collection_.get());
  StartPipeline(gurl);
}

void WebMediaPlayerImpl::StartPipeline(const GURL& gurl) {
  started_ = true;
  pipeline_->Start(
      filter_collection_.Pass(),
      gurl.spec(),
      base::Bind(&WebMediaPlayerProxy::PipelineInitializationCallback,
                 proxy_.get()));
}

void WebMediaPlayerImpl::SetNetworkState(
    WebKit::WebMediaPlayer::NetworkState state) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  // Always notify to ensure client has the latest value.
  network_state_ = state;
  GetClient()->networkStateChanged();
}

void WebMediaPlayerImpl::SetReadyState(
    WebKit::WebMediaPlayer::ReadyState state) {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  // Always notify to ensure client has the latest value.
  ready_state_ = state;
  GetClient()->readyStateChanged();
}

void WebMediaPlayerImpl::Destroy() {
  DCHECK_EQ(main_loop_, MessageLoop::current());

  // Tell the data source to abort any pending reads so that the pipeline is
  // not blocked when issuing stop commands to the other filters.
  if (proxy_) {
    proxy_->AbortDataSource();
    proxy_->DemuxerShutdown();
  }

  // Make sure to kill the pipeline so there's no more media threads running.
  // Note: stopping the pipeline might block for a long time.
  if (started_) {
    media::PipelineStatusNotification note;
    pipeline_->Stop(note.Callback());
    note.Wait();
    started_ = false;
  }

  // Let V8 know we are not using extra resources anymore.
  if (incremented_externally_allocated_memory_) {
    v8::V8::AdjustAmountOfExternalAllocatedMemory(-kPlayerExtraMemory);
    incremented_externally_allocated_memory_ = false;
  }

  message_loop_factory_.reset();

  // And then detach the proxy, it may live on the render thread for a little
  // longer until all the tasks are finished.
  if (proxy_) {
    proxy_->Detach();
    proxy_ = NULL;
  }
}

WebKit::WebMediaPlayerClient* WebMediaPlayerImpl::GetClient() {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  DCHECK(client_);
  return client_;
}

WebKit::WebAudioSourceProvider* WebMediaPlayerImpl::audioSourceProvider() {
  return audio_source_provider_;
}

void WebMediaPlayerImpl::IncrementExternallyAllocatedMemory() {
  DCHECK_EQ(main_loop_, MessageLoop::current());
  incremented_externally_allocated_memory_ = true;
  v8::V8::AdjustAmountOfExternalAllocatedMemory(kPlayerExtraMemory);
}

}  // namespace webkit_media
