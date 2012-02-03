// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef CHROME_RENDERER_MEDIA_SIPCC_RENDERER_IMPL_H_
#define CHROME_RENDERER_MEDIA_SIPCC_RENDERER_IMPL_H_
#pragma once

#include "base/message_loop.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "media/base/filters.h"
#include "content/common/media/video_capture.h"
#include "content/renderer/sip/sipcc_message_filter.h"
#include "media/video/capture/video_capture.h"
#include "sipcc_video_decoder.h"


namespace base {
class MessageLoopProxy;
}



class SipccMessageFilter;

//callbacks for the sipstack operations- registration , place call, hangup
//typedef Callback0::Type SipccOperationCallback;

//This object runs on renderer thread as well as io_thred
// Webkit operations happen on the render thread and Sip State  and
// related operations happen through io_thread for IPC to the browser 
// process. This is also a delegate for the session state changes from
// the sip stack running in the browser process

class SipccRendererImpl : public base::RefCountedThreadSafe<SipccRendererImpl>,
			  public SipccMessageFilter::Delegate {
			  
 public:

  // Methods called on Render thread  - as part of media player initialization
  explicit SipccRendererImpl();
			     
  virtual ~SipccRendererImpl();

  void Initialize(MessageLoop* loop); 

  void SetDecoderMessageLoop(MessageLoop* loop) {
  	decoder_loop_ = loop;
  }
  // SipccMessageFilter::Delegate methods, called by MessageFilter.
  virtual void OnVideoBufferCreated(base::SharedMemoryHandle handle,
                               int length, int buffer_id) OVERRIDE;
  virtual void OnVideoBufferReceived(int buffer_id, unsigned timestamp) OVERRIDE;
 
  //These methods are called on the render loop from the client (webmediaplayer)
  // calls are delegated to the io_thread to perform the IPC requests and responses
  //void PlaceCall(std::string dial_number, SipccOperationCallback* sip_op_callback);
  void PlaceCall(std::string dial_number);
  void Register();
  void SetVideoRenderer(scoped_refptr<SIPCCVideoDecoder> video_decoder );
 // callback registrations
 //void AddRegistrationCallback(SipccOperationCallback* callback); 
 //void AddSessionCallback(SipccOperationCallback* callback); 

  protected:
 	friend class base::RefCountedThreadSafe<SipccRendererImpl>;

 private:

   struct DIBBuffer;

  // Methods called on IO thread ----------------------------------------------
  // These methods are executed on the IO Thread. These use Sipcc Message filter
  // and carry out needed IPC for Browser Sip stack 
  void PerformSetupCallTask(std::string dial_number);
  void PerformRegisterTask();
  void PerformBufferReadyTask(int buffer_id);
  void AddDelegateOnIOThread();
  void RemoveDelegateOnIOThread(base::Closure task);

   // works on the ml_proxy_
   void DoVideoBufferCreated(base::SharedMemoryHandle handle,
                       int length, int buffer_id);
  void DoVideoBufferReceived(int buffer_id, unsigned timestamp);



  // Delegate transfer methods to the callbacks
  void NotifyRegistrationStateChange();
  void NotifySessionStateChange(); 


  // Pool of DIBs.
  typedef std::map<int /* buffer_id */, DIBBuffer*> CachedDIB;
  CachedDIB cached_dibs_;

  // since this reference will be accessed across threads, let's be safe here
  scoped_refptr<SipccMessageFilter> filter_;
  SIPCCVideoDecoder* video_renderer_;
  scoped_refptr<base::MessageLoopProxy> ml_proxy_;
  MessageLoop* render_loop_;

  // sip video decoder: It need not decode anything
  MessageLoop* decoder_loop_;

   //callbacks for registration events. These get reported to webmediaplayer 
  // scoped_ptr<SipccOperationCallback> registration_callback_;
  // scoped_ptr<SipccOperationCallback> session_callback_;
   
  bool initDone;

  DISALLOW_COPY_AND_ASSIGN(SipccRendererImpl);
};

#endif  // CHROME_RENDERER_MEDIA_SIPCC_RENDERER_IMPL_H_
