// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/sip/sipcc_renderer_impl.h"

#include <math.h>

#include "base/bind.h"
#include "base/stl_util.h"
#include "content/common/child_process.h"
#include "content/common/sip/sip_messages.h"
#include "content/renderer/sip/sipcc_message_filter.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/render_thread.h"
#include "media/base/filter_host.h"
#include "content/renderer/render_thread_impl.h"
#include "content/browser/renderer_host/sip/sipcc_controller.h"

SipccRendererImpl* SipccRendererImpl::_instance = 0;

struct SipccRendererImpl::DIBBuffer {
 public:
  DIBBuffer(
      base::SharedMemory* d,
      media::VideoCapture::VideoFrameBuffer* ptr)
      : dib(d),
        mapped_memory(ptr),
        references(0) {
  }
  ~DIBBuffer() {}

  scoped_ptr<base::SharedMemory> dib;
  scoped_refptr<media::VideoCapture::VideoFrameBuffer> mapped_memory;

  // Number of clients which hold this DIB.
  int references;
};

SipccRendererImpl* SipccRendererImpl::GetInstance()
{
    if(_instance == 0)
    {
        _instance = new SipccRendererImpl();
    }
    return _instance;
}


SipccRendererImpl::SipccRendererImpl() : capture_video_renderer_(0)
										 ,receive_video_renderer_(0)
										 ,reg_state_(SIP_NOREGISTRAR)
										 ,session_state_(SIP_NOSESSION)
										 ,session_id_(-1)
										 ,initDone(false) 
										 ,hasLocalCache(false)
										 ,hasRemoteCache(false)
{
	LOG(ERROR) <<" SipccRendererImpl: Constructor ";
    filter_ = RenderThreadImpl::current()->sipcc_message_filter();
	registration_callback_.reset(NULL);
    session_callback_.reset(NULL);
}

SipccRendererImpl::~SipccRendererImpl() 
{
     LOG(ERROR) <<" SipccRendererImpl: DESTRUCTOR";
	if(hasLocalCache == true)
	    STLDeleteContainerPairSecondPointers(capture_cached_dibs_.begin(),
   			                                   capture_cached_dibs_.end());

	if(hasRemoteCache == true)
	    STLDeleteContainerPairSecondPointers(receive_cached_dibs_.begin(),
   			                                   receive_cached_dibs_.end());
	initDone = false;
}

void SipccRendererImpl::Initialize(MessageLoop* loop) 
{

	LOG(INFO) << "SipccRendererImpl::Initialize: Added message_loop " << loop;
	if(initDone == false)
	{
		render_loop_ = loop;
	
		base::MessageLoopProxy* io_message_loop_proxy =
   	   ChildProcess::current()->io_message_loop_proxy();

  		if (!io_message_loop_proxy->BelongsToCurrentThread()) {
   		 	io_message_loop_proxy->PostTask(FROM_HERE,
   		     base::Bind(&SipccRendererImpl::AddDelegateOnIOThread,
                   base::Unretained(this)));
   		 return;
  		}
	}
}

void SipccRendererImpl::AddRegistrationCallback(SipccOperationCallback* callback)
{
	if(!registration_callback_.get())
		registration_callback_.reset(callback);	
}

void SipccRendererImpl::AddSessionCallback(SipccOperationCallback* callback)
{
	if(!session_callback_.get())
		session_callback_.reset(callback);	
}

void SipccRendererImpl::AddDelegateOnIOThread() {
  LOG(INFO) << " SipccRendererImpl::AddDelegateOnIOThread: Registering myself as delegate ";
  filter_->AddDelegate(this);
  initDone = true;
}

void SipccRendererImpl::RemoveDelegateOnIOThread(base::Closure task) {
  filter_->RemoveDelegate(this);
//  ml_proxy_->PostTask(FROM_HERE, task);
}


void SipccRendererImpl::SetCaptureVideoRenderer(scoped_refptr<SIPCCVideoDecoder> video_decoder)
{
	capture_video_renderer_ = video_decoder.get();
    LOG(INFO) << " SipccRendererImpl:: Setting capture video renderer to " << capture_video_renderer_; 
}

void SipccRendererImpl::SetReceiveVideoRenderer(scoped_refptr<SIPCCVideoDecoder> video_decoder)
{
	receive_video_renderer_ = video_decoder.get();
    LOG(INFO) << " SipccRendererImpl:: Setting receive renderer to " << receive_video_renderer_; 
}

void SipccRendererImpl::PlaceCall(std::string dial_number)
{
	LOG(INFO) <<" SipccRendererImpl:: PlaceCall : Number : " << dial_number;

    base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

    io_message_loop_proxy->PostTask(FROM_HERE,
       base::Bind(&SipccRendererImpl::PerformSetupCallTask,
                 base::Unretained(this), dial_number));

} 


void SipccRendererImpl::Register(std::string aor, std::string creds, std::string proxy, bool isLocal)
{

     base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

     io_message_loop_proxy->PostTask(FROM_HERE,
       base::Bind(&SipccRendererImpl::PerformRegisterTask,
                 base::Unretained(this), aor, creds, proxy, isLocal));

} 

void SipccRendererImpl::UnRegister()
{
	base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

    io_message_loop_proxy->PostTask(FROM_HERE,
       base::Bind(&SipccRendererImpl::PerformUnRegisterTask,
                 base::Unretained(this)));


}

void SipccRendererImpl::HangUp()
{
	base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

    io_message_loop_proxy->PostTask(FROM_HERE,
       base::Bind(&SipccRendererImpl::PerformHangupCallTask,
                 base::Unretained(this)));

}

void SipccRendererImpl::AnswerCall()
{
	base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

    io_message_loop_proxy->PostTask(FROM_HERE,
       base::Bind(&SipccRendererImpl::PerformAnswerCallTask,
                 base::Unretained(this)));

}




void SipccRendererImpl::PerformSetupCallTask(std::string dial_number) 
{
    LOG(INFO) << "SipccRendererImpl:: PerformSetupCallTask Called ";
	filter_->Send(new SipHostMsg_PlaceCall(dial_number));
}


void SipccRendererImpl::PerformRegisterTask(std::string aor, 
											std::string creds, 
											std::string proxy,
											bool isLocal) 
{
    LOG(INFO) << " PerformRegisterTask Called ";
	filter_->Send(new SipHostMsg_Register(aor,creds,proxy,isLocal));
}

void SipccRendererImpl::PerformUnRegisterTask() 
{
    LOG(INFO) << " PerformUnRegisterTask Called ";
	filter_->Send(new SipHostMsg_UnRegister());
	
}

void SipccRendererImpl::PerformHangupCallTask()
{
    LOG(INFO) << " PerformHangup Called ";
	filter_->Send(new SipHostMsg_Hangup());
}


void SipccRendererImpl::PerformAnswerCallTask()
{
    LOG(INFO) << " PerformAnswerCall Called ";
	filter_->Send(new SipHostMsg_AnswerCall());
}

// This function gets invoked on the io_loop by default
void SipccRendererImpl::OnCaptureVideoBufferCreated(
    base::SharedMemoryHandle handle,
    int length, int buffer_id) {
  LOG(INFO) << " SipccRendererImpl: OnCaptureVideoBufferCreated: Posting to decoder loop";
  hasLocalCache = true;
  capture_decoder_loop_->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::DoCaptureVideoBufferCreated,
                base::Unretained(this), handle, length, buffer_id));
  LOG(INFO) << " SipccRendererImpl: OnCaptureVideoBufferCreated: Done Posting to decoder loop";
}

void SipccRendererImpl::OnReceiveVideoBufferCreated(
    base::SharedMemoryHandle handle,
    int length, int buffer_id) {
  LOG(INFO) << " SipccRendererImpl: OnReceiveVideoBufferCreated: Posting to decoder loop";
  hasRemoteCache = true;
  receive_decoder_loop_->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::DoReceiveVideoBufferCreated,
                base::Unretained(this), handle, length, buffer_id));
  LOG(INFO) << " SipccRendererImpl: OnReceiveVideoBufferCreated: Done Posting to decoder loop";
}

// This function is invoked on io_loop 
void SipccRendererImpl::OnCaptureVideoBufferReceived(int buffer_id, unsigned int timestamp) {
 LOG(INFO) << "SipccRendererImpl::OnCaptureVideoBufferReceived: Posting to decoder loop";
 capture_decoder_loop_->PostTask(FROM_HERE,
     base::Bind(&SipccRendererImpl::DoCaptureVideoBufferReceived,
                base::Unretained(this), buffer_id, timestamp));

 LOG(INFO) << "SipccRendererImpl::OnCaptureVideoBufferReceived: Done POST to decoder loop";
}


void SipccRendererImpl::OnReceiveVideoBufferReceived(int buffer_id, unsigned int timestamp) {
 LOG(INFO) << "SipccRendererImpl::OnReceiveVideoBufferReceived: Posting to decoder loop";
 receive_decoder_loop_->PostTask(FROM_HERE,
     base::Bind(&SipccRendererImpl::DoReceiveVideoBufferReceived,
                base::Unretained(this), buffer_id, timestamp));

 LOG(INFO) << "SipccRendererImpl::OnReceiveVideoBufferReceived: Done POST to decoder loop";
}


// Should be invoked on the decoder loop only
void SipccRendererImpl::DoCaptureVideoBufferCreated(
    base::SharedMemoryHandle handle,
    int length, int buffer_id) {
  LOG(INFO) << "SipccRendererImpl::DoCaptureVideoBufferCreated:: Creating Shared Memory Cache";
  media::VideoCapture::VideoFrameBuffer* buffer;
  DCHECK(capture_cached_dibs_.find(buffer_id) == capture_cached_dibs_.end());

  base::SharedMemory* dib = new base::SharedMemory(handle, false);
  dib->Map(length);
  buffer = new media::VideoCapture::VideoFrameBuffer();
  buffer->memory_pointer = static_cast<uint8*>(dib->memory());
  buffer->buffer_size = length;
  buffer->width = 640;
  buffer->height = 480;

  DIBBuffer* dib_buffer = new DIBBuffer(dib, buffer);
  capture_cached_dibs_[buffer_id] = dib_buffer;
  LOG(INFO) << "SipccRendererImpl::DoCaptureVideoBufferCreated:: Memory Created";
}

void SipccRendererImpl::DoReceiveVideoBufferCreated(
    base::SharedMemoryHandle handle,
    int length, int buffer_id) {
  LOG(INFO) << "SipccRendererImpl::DoReceiveVideoBufferCreated:: Creating Shared Memory Cache";
  media::VideoCapture::VideoFrameBuffer* buffer;
  DCHECK(receive_cached_dibs_.find(buffer_id) == receive_cached_dibs_.end());

  base::SharedMemory* dib = new base::SharedMemory(handle, false);
  dib->Map(length);
  buffer = new media::VideoCapture::VideoFrameBuffer();
  buffer->memory_pointer = static_cast<uint8*>(dib->memory());
  buffer->buffer_size = length;
  buffer->width = 640;
  buffer->height = 480;

  DIBBuffer* dib_buffer = new DIBBuffer(dib, buffer);
  receive_cached_dibs_[buffer_id] = dib_buffer;
  LOG(INFO) << "SipccRendererImpl::DoReceiveVideoBufferCreated:: Memory Created";
}

void SipccRendererImpl::DoCaptureVideoBufferReceived(int buffer_id, unsigned timestamp) {
  LOG(INFO) << "SipccRendererImpl::DoCaptureVideoBufferReceived:: Video Frame Recieved , Buffer Id" << buffer_id;

  media::VideoCapture::VideoFrameBuffer* buffer;
  DCHECK(capture_cached_dibs_.find(buffer_id) != capture_cached_dibs_.end());
  buffer = capture_cached_dibs_[buffer_id]->mapped_memory;
  //let our renderer know so that pipeline can paint it
  capture_cached_dibs_[buffer_id]->references = 1;
  capture_video_renderer_->DeliverFrame(buffer);
  LOG(INFO) << "SipccRendererImpl::DoCaptureVideoBufferReceived: DeliverFrame to the decoder";

  //let's give the buffer back
  CachedDIB::iterator it;
  for (it = capture_cached_dibs_.begin(); it != capture_cached_dibs_.end(); it++) {
    if (buffer == it->second->mapped_memory)
      break;
  }

  
  DCHECK(it != capture_cached_dibs_.end());
  DCHECK_GT(it->second->references, 0);
  it->second->references--;


  base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

   io_message_loop_proxy->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::PerformCaptureBufferReadyTask,
   	            base::Unretained(this), buffer_id));

 LOG(INFO) << " SipccRenderImpl::DoCaptureVideoBufferRecieved: Handled Video Buffer ";

}

void SipccRendererImpl::DoReceiveVideoBufferReceived(int buffer_id, unsigned timestamp) {
  LOG(INFO) << "SipccRendererImpl::DoReceiveVideoBufferReceived:: Video Frame Recieved , Buffer Id" << buffer_id;

  media::VideoCapture::VideoFrameBuffer* buffer;
  DCHECK(receive_cached_dibs_.find(buffer_id) != receive_cached_dibs_.end());
  buffer = receive_cached_dibs_[buffer_id]->mapped_memory;
  //let our renderer know so that pipeline can paint it
  receive_cached_dibs_[buffer_id]->references = 1;
  receive_video_renderer_->DeliverFrame(buffer);
  LOG(INFO) << "SipccRendererImpl::DoReceiveVideoBufferReceived: DeliverFrame to the decoder";

  //let's give the buffer back
  CachedDIB::iterator it;
  for (it = receive_cached_dibs_.begin(); it != receive_cached_dibs_.end(); it++) {
    if (buffer == it->second->mapped_memory)
      break;
  }

 
  DCHECK(it != receive_cached_dibs_.end());
  DCHECK_GT(it->second->references, 0);
  it->second->references--;


  base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

   io_message_loop_proxy->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::PerformReceiveBufferReadyTask,
                base::Unretained(this), buffer_id));

 LOG(INFO) << " SipccRenderImpl::DoReceiveVideoBufferRecieved: Handled Video Buffer ";

}


void SipccRendererImpl::PerformCaptureBufferReadyTask(int buffer_id)
{
    filter_->Send(new SipHostMsg_CaptureBufferReady(buffer_id));
}

void SipccRendererImpl::PerformReceiveBufferReadyTask(int buffer_id)
{
    filter_->Send(new SipHostMsg_ReceiveBufferReady(buffer_id));
}

/*webmediaplayer callbacks delegation */
void SipccRendererImpl::NotifyRegistrationStateChange() {
	LOG(INFO) << " SipccRendererImpl:: NotifyRegistrationStateChange";
	if(registration_callback_.get())
		registration_callback_->Run();
	
}

void SipccRendererImpl::NotifySessionStateChange() {
	LOG(INFO) << " SipccRendererImpl:: NotifySessionStateChange";
	if(session_callback_.get())
		session_callback_->Run();
}

void SipccRendererImpl::OnSipIncomingCall(std::string callingPartyName,
											std::string callingPartyNum,
											std::string state) 
{

   LOG(ERROR) << "SipccRendererImpl::OnSipIncomingCall. caller :" << callingPartyName << " " << callingPartyNum ;
   caller_ = callingPartyName;
   caller_number_ = callingPartyNum;
   if( state.compare("SIP_ACCEPTINGSESSION") == 0)
   {
		session_state_ = SIP_ACCEPTINGSESSION;
   }
	session_state_ = SIP_ACCEPTINGSESSION;
   LOG(ERROR) << "Asking render loop to notify on Incoming call and session change ";
    render_loop_->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::NotifySessionStateChange,
                base::Unretained(this)));

}

void SipccRendererImpl::OnSipRegStateChanged(const std::string reg_state) {

   LOG(ERROR) << "SipccRendererImpl::OnSipRegStateChanged. State :" << reg_state;
   //capture the state
	if( reg_state.compare("SIP_NOREGISTRAR") == 0)
	{
		reg_state_ = SIP_NOREGISTRAR;	
	}
	else if( reg_state.compare("SIP_REGISTERING") == 0)
	{
      reg_state_ = SIP_REGISTERING;
	}
	else if( reg_state.compare("SIP_REGISTERED") == 0)
	{
      reg_state_ =  SIP_REGISTERED;
		
	}
	else if( reg_state.compare("SIP_REGISTRATIONFAILED") == 0)
	{
      reg_state_ = SIP_REGISTRATIONFAILED;
		
	}
	else if( reg_state.compare("SIP_REGINVALID") == 0)
	{
      reg_state_ = SIP_REGINVALID;
		
	}
 LOG(ERROR) << " Asking render_loop to call our callback ";
	render_loop_->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::NotifyRegistrationStateChange,
                base::Unretained(this)));


}

void SipccRendererImpl::OnSipSessionStateChanged(const std::string session_state) 
{

   	LOG(ERROR) << "SipccRendererImpl::OnSipSessionStateChanged. State :" << session_state;
	if( session_state.compare("SIP_NOSESSION") == 0)
	{
		session_state_ = SIP_NOSESSION;
	}
	else if(session_state.compare("SIP_OPENINGSESSION") == 0)
	{
		session_state_ = SIP_OPENINGSESSION;
	}
	else if(session_state.compare("SIP_ACCEPTINGSESSION") == 0)
	{
		session_state_ = SIP_ACCEPTINGSESSION;
	}
	else if(session_state.compare("SIP_INSESSION") == 0)
	{
		session_state_ = SIP_INSESSION;
	}
	else if(session_state.compare("SIP_CLOSINGSESSION") == 0)
	{
		session_state_ = SIP_CLOSINGSESSION;
	}
	else if(session_state.compare("SIP_SESSIONINVALID") == 0)
	{
		session_state_ = SIP_SESSIONINVALID;
	}

  //callback the webmediaplayer entry
   LOG(ERROR) << "Asking render loop to notify on session change ";
	render_loop_->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::NotifySessionStateChange,
                base::Unretained(this)));
}





