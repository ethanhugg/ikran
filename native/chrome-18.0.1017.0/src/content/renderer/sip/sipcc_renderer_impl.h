// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef CHROME_RENDERER_MEDIA_SIPCC_RENDERER_IMPL_H_
#define CHROME_RENDERER_MEDIA_SIPCC_RENDERER_IMPL_H_
#pragma once

#include <vector>

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
typedef base::Callback<void(void)> SipccOperationCallback; 

//This object runs on renderer thread as well as io_thred
// Webkit operations happen on the render thread and Sip State  and
// related operations happen through io_thread for IPC to the browser 
// process. This is also a delegate for the session state changes from
// the sip stack running in the browser process

class SipccRendererImpl : public base::RefCountedThreadSafe<SipccRendererImpl>,
			  			  public SipccMessageFilter::Delegate {
			  
 public:

  	enum SipRegState { SIP_NOREGISTRAR, SIP_REGISTERING, SIP_REGISTERED, SIP_REGISTRATIONFAILED,SIP_REGINVALID };
 	enum SipSessionState { SIP_NOSESSION, SIP_OPENINGSESSION, SIP_ACCEPTINGSESSION, SIP_INSESSION, SIP_CLOSINGSESSION, SIP_SESSIONINVALID };

	//Singleton Object
    static SipccRendererImpl* GetInstance();

 	int sipRegState()  
	{
		return reg_state_;
	}
	int sipSessionState()
	{
		return session_state_;
	} 

  	// Methods called on Render thread  - as part of media player initialization
  	void Initialize(MessageLoop* loop); 

  	void SetCaptureDecoderMessageLoop(MessageLoop* loop) {
  		capture_decoder_loop_ = loop;
  	}

  	void SetReceiveDecoderMessageLoop(MessageLoop* loop) {
  		receive_decoder_loop_ = loop;
  	}

  // SipccMessageFilter::Delegate methods, called by MessageFilter.
  	virtual void OnCaptureVideoBufferCreated(base::SharedMemoryHandle handle,
                         				      int length, int buffer_id) OVERRIDE;
  	virtual void OnReceiveVideoBufferCreated(base::SharedMemoryHandle handle,
                         				      int length, int buffer_id) OVERRIDE;
  	virtual void OnCaptureVideoBufferReceived(int buffer_id, unsigned timestamp) OVERRIDE;
  	virtual void OnReceiveVideoBufferReceived(int buffer_id, unsigned timestamp) OVERRIDE;
	virtual void OnSipRegStateChanged(const std::string reg_state) OVERRIDE;
    virtual void OnSipSessionStateChanged(const std::string session_state) OVERRIDE;
    virtual void OnSipIncomingCall(std::string callingPartyName, std::string callingPartyNum,std::string state) OVERRIDE;


  //These methods are called on the render loop from the client (webmediaplayer)
  // calls are delegated to the io_thread to perform the IPC requests and responses
  	void PlaceCall(std::string dial_number);
    void HangUp();
  	void Register(std::string aor, std::string creds, std::string proxy, bool isLocal);
    void UnRegister();
    void AnswerCall();
  	void SetCaptureVideoRenderer(scoped_refptr<SIPCCVideoDecoder> video_decoder );
  	void SetReceiveVideoRenderer(scoped_refptr<SIPCCVideoDecoder> video_decoder );

  //callback registrations
  	void AddRegistrationCallback(SipccOperationCallback* callback); 
  	void AddSessionCallback(SipccOperationCallback* callback); 

	//few more access functions
	std::string callerName() 
	{
		return caller_;
	} 
	std::string callerNumber() {
		return caller_number_;
	} 
	int sessionId() const;


  	protected:
 		friend class base::RefCountedThreadSafe<SipccRendererImpl>;

 	private:
   	struct DIBBuffer;

  	SipccRendererImpl();
  	virtual ~SipccRendererImpl();
  // Methods called on IO thread ----------------------------------------------
  // These methods are executed on the IO Thread. These use Sipcc Message filter
  // and carry out needed IPC for Browser Sip stack 
  	void PerformSetupCallTask(std::string dial_number);
  	void PerformRegisterTask(std::string aor, std::string creds, std::string proxy, bool isLocal);
	void PerformUnRegisterTask();
	void PerformHangupCallTask();
	void PerformAnswerCallTask();
  	void PerformCaptureBufferReadyTask(int buffer_id);
  	void PerformReceiveBufferReadyTask(int buffer_id);
  	void AddDelegateOnIOThread();
  	void RemoveDelegateOnIOThread(base::Closure task);

   // works on the ml_proxy_
   	void DoCaptureVideoBufferCreated(base::SharedMemoryHandle handle,
   	             				       int length, int buffer_id);							  
	void DoReceiveVideoBufferCreated(base::SharedMemoryHandle handle,
										int length, int buffer_id);
							  
    void DoCaptureVideoBufferReceived(int buffer_id, unsigned timestamp);
	void DoReceiveVideoBufferReceived(int buffer_id, unsigned timestamp);
	


  	// Delegate transfer methods to the callbacks
  	void NotifyRegistrationStateChange();
  	void NotifySessionStateChange(); 


  	// Pool of DIBs.
  	typedef std::map<int /* buffer_id */, DIBBuffer*> CachedDIB;
  	CachedDIB capture_cached_dibs_;
  	CachedDIB receive_cached_dibs_;

  	// since this reference will be accessed across threads, let's be safe here
  	scoped_refptr<SipccMessageFilter> filter_;
  	SIPCCVideoDecoder* capture_video_renderer_;
  	SIPCCVideoDecoder* receive_video_renderer_;
  	scoped_refptr<base::MessageLoopProxy> ml_proxy_;
  	MessageLoop* render_loop_;

  	// sip video decoder: It need not decode anything
  	MessageLoop* capture_decoder_loop_;
  	MessageLoop* receive_decoder_loop_;

   	//callbacks for registration events. These get reported to webmediaplayer 
  	scoped_ptr <SipccOperationCallback> registration_callback_;
  	scoped_ptr <SipccOperationCallback> session_callback_;
   
	int32 call_id_;
	std::string caller_;
	std::string caller_number_;
	SipRegState reg_state_;
	SipSessionState session_state_;
	int session_id_;	

	//Session Related information
	std::string address_of_record_;
	std::string sip_proxy_server_;
	std::string session_credential_;
	std::string sip_url_;
	std::string dn_;	
  	bool initDone;
	bool hasLocalCache;
    bool hasRemoteCache;

	 // singleton instance
    static SipccRendererImpl* _instance;

  	DISALLOW_COPY_AND_ASSIGN(SipccRendererImpl);
};

#endif  // CHROME_RENDERER_MEDIA_SIPCC_RENDERER_IMPL_H_
