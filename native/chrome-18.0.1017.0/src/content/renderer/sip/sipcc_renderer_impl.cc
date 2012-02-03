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



SipccRendererImpl::SipccRendererImpl() : video_renderer_(0){
	LOG(ERROR) <<" SipccRendererImpl: Constructor ";
    filter_ = RenderThreadImpl::current()->sipcc_message_filter();

}

SipccRendererImpl::~SipccRendererImpl() {
     LOG(ERROR) <<" SipccRendererImpl: DESTRUCTOR";
//    DeleteContainerPairSecondPointers(cached_dibs_.begin(),
 //                                     cached_dibs_.end());

	initDone = false;
}

void SipccRendererImpl::Initialize(MessageLoop* loop) {

    render_loop_ = loop; 
	base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

  if (!io_message_loop_proxy->BelongsToCurrentThread()) {
    io_message_loop_proxy->PostTask(FROM_HERE,
        base::Bind(&SipccRendererImpl::AddDelegateOnIOThread,
                   base::Unretained(this)));
    return;
  }

  AddDelegateOnIOThread();

}

/*
void SipccRendererImpl::AddRegistrationCallback(SipccOperationCallback* callback)
{
	registration_callback_.reset(callback); 
}

void SipccRendererImpl::AddSessionCallback(SipccOperationCallback* callback)
{
	session_callback_.reset(callback); 
}
*/

void SipccRendererImpl::AddDelegateOnIOThread() {
  LOG(INFO) << " SipccRendererImpl::AddDelegateOnIOThread: Registering myself as delegate ";
  filter_->AddDelegate(this);
}

void SipccRendererImpl::RemoveDelegateOnIOThread(base::Closure task) {
  filter_->RemoveDelegate(this);
//  ml_proxy_->PostTask(FROM_HERE, task);
}


void SipccRendererImpl::SetVideoRenderer(scoped_refptr<SIPCCVideoDecoder> video_decoder)
{
	video_renderer_ = video_decoder.get();
       LOG(INFO) << " SipccRendererImpl:: Setting video renderer to " << video_renderer_; 

}

void SipccRendererImpl::PlaceCall(std::string dial_number)
{
//    DCHECK(MessageLoop::current() == render_loop_);
 //   DCHECK(io_loop_); 
      LOG(INFO) << " Place Call ";

//    if(!session_callback_.get())
//		session_callback_.reset(sip_op_callback);

     base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

     io_message_loop_proxy->PostTask(FROM_HERE,
       base::Bind(&SipccRendererImpl::PerformSetupCallTask,
                 base::Unretained(this), dial_number));

} 


void SipccRendererImpl::Register()
{

     base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

     io_message_loop_proxy->PostTask(FROM_HERE,
       base::Bind(&SipccRendererImpl::PerformRegisterTask,
                 base::Unretained(this)));

} 


void SipccRendererImpl::PerformSetupCallTask(std::string dial_number) {
        LOG(INFO) << "SipccRendererImpl:: PerformSetupCallTask Called ";
	filter_->Send(new SipHostMsg_PlaceCall(dial_number));
}


void SipccRendererImpl::PerformRegisterTask() {
        LOG(INFO) << " PerformRegisterTask Called ";
	filter_->Send(new SipHostMsg_Register());
}

// This function gets invoked on the io_loop by default

void SipccRendererImpl::OnVideoBufferCreated(
    base::SharedMemoryHandle handle,
    int length, int buffer_id) {
  LOG(INFO) << " SipccRendererImpl: OnVideoBufferCreated: Posting to decoder loop";
  decoder_loop_->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::DoVideoBufferCreated,
                base::Unretained(this), handle, length, buffer_id));
}

// This function is invoked on io_loop 
void SipccRendererImpl::OnVideoBufferReceived(int buffer_id, unsigned int timestamp) {
 LOG(INFO) << "SipccRendererImpl::OnVideoBufferReceived: Posting to decoder loop";
 decoder_loop_->PostTask(FROM_HERE,
     base::Bind(&SipccRendererImpl::DoVideoBufferReceived,
                base::Unretained(this), buffer_id, timestamp));

 LOG(INFO) << "SipccRendererImpl::OnVideoBufferReceived: Done POST to decoder loop";
 /***************
  base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

   io_message_loop_proxy->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::PerformBufferReadyTask,
                base::Unretained(this), buffer_id));
  **************/

}


// Should be invoked on the decoder loop only

void SipccRendererImpl::DoVideoBufferCreated(
    base::SharedMemoryHandle handle,
    int length, int buffer_id) {
  LOG(INFO) << "SipccRendererImpl::DoVideoBufferCreated:: Creating Shared Memory Cache";
  media::VideoCapture::VideoFrameBuffer* buffer;
  DCHECK(cached_dibs_.find(buffer_id) == cached_dibs_.end());

  base::SharedMemory* dib = new base::SharedMemory(handle, false);
  dib->Map(length);
  buffer = new media::VideoCapture::VideoFrameBuffer();
  buffer->memory_pointer = static_cast<uint8*>(dib->memory());
  buffer->buffer_size = length;
  buffer->width = 640;
  buffer->height = 480;

  DIBBuffer* dib_buffer = new DIBBuffer(dib, buffer);
  cached_dibs_[buffer_id] = dib_buffer;
  LOG(INFO) << "SipccRendererImpl::DoVideoBufferCreated:: Memory Created";
}

void SipccRendererImpl::DoVideoBufferReceived(int buffer_id, unsigned timestamp) {
  LOG(INFO) << "SipccRendererImpl::DoVideoBufferReceived:: Video Frame Recieved , Buffer Id" << buffer_id;

  media::VideoCapture::VideoFrameBuffer* buffer;
  DCHECK(cached_dibs_.find(buffer_id) != cached_dibs_.end());
  buffer = cached_dibs_[buffer_id]->mapped_memory;
  //let our renderer know so that pipeline can paint it
  cached_dibs_[buffer_id]->references = 1;
  video_renderer_->DeliverFrame(buffer);
  LOG(INFO) << "SipccRendererImpl::DoVideoBufferReceived: DeliverFrame to the decoder";

  //let's give the buffer back
  CachedDIB::iterator it;
  for (it = cached_dibs_.begin(); it != cached_dibs_.end(); it++) {
    if (buffer == it->second->mapped_memory)
      break;
  }

  
  DCHECK(it != cached_dibs_.end());
  DCHECK_GT(it->second->references, 0);
  it->second->references--;


  base::MessageLoopProxy* io_message_loop_proxy =
      ChildProcess::current()->io_message_loop_proxy();

   io_message_loop_proxy->PostTask(FROM_HERE,
      base::Bind(&SipccRendererImpl::PerformBufferReadyTask,
   	            base::Unretained(this), buffer_id));

 LOG(INFO) << " SipccRenderImpl::DoVideoBufferRecieved: Handled Video Buffer ";

}

void SipccRendererImpl::PerformBufferReadyTask(int buffer_id)
{

    filter_->Send(new SipHostMsg_BufferReady(buffer_id));

}

/*webmediaplayer callbacks delegation
void SipccRendererImpl::NotifyRegistrationStateChange() {

	if(registration_callback_.get())
			registration_callback_->Run();
	
}

void SipccRendererImpl::NotifySessionStateChange() {

	if(session_callback_.get())
			session_callback_->Run();
	
}

*/





