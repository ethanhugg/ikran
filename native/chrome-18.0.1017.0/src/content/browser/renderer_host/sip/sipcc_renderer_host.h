// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef CONTENT_BROWSER_RENDERER_HOST_SIPCC_RENDERER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_SIPCC_RENDERER_HOST_H_
#pragma once

#include <map>

#include "build/build_config.h"


#include "base/gtest_prod_util.h"
#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop_helpers.h"
#include "base/process.h"
#include "base/shared_memory.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/browser/renderer_host/sip/sipcc_controller.h"


namespace content {
class ResourceContext;
}  // namespace content

    
// Making this as external renderer is not a great idea
// May be one should refactor it , if needed
class CONTENT_EXPORT SipccRendererHost 
               : public content::BrowserMessageFilter,
		 		 public SipccControllerObserver {
                          
 public:

  // Called from UI thread from the owner of this object.
  SipccRendererHost(const content::ResourceContext* resource_context);


  // BrowserMessageFilter implementation.
  virtual void OnChannelClosing() OVERRIDE;
  virtual void OnDestruct() const OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message,
                                 bool* message_was_ok) OVERRIDE;


  // SipccControllerObserver methods
  virtual void OnIncomingCall(std::string callingPartyName, std::string callingPartyNumber) OVERRIDE;
  virtual void OnRegisterStateChange(std::string registrationState) OVERRIDE;
  virtual void OnCallTerminated() OVERRIDE; 
  virtual void OnCallConnected(char* sdp) OVERRIDE; 
  virtual void OnCallHeld() OVERRIDE;
  virtual void OnCallResume() OVERRIDE; 

  void DoSendCaptureBufferCreated(base::SharedMemoryHandle handle,
						 			int length,
						 			int buffer_id) OVERRIDE;
  void DoSendCaptureBufferFilled(int buffer_id,
					   				unsigned int timestamp) OVERRIDE;

  void DoSendReceiveBufferCreated(base::SharedMemoryHandle handle,
						 			int length,
						 			int buffer_id) OVERRIDE;
  void DoSendReceiveBufferFilled(int buffer_id,
					   				unsigned int timestamp) OVERRIDE;

  virtual ~SipccRendererHost();
 private:
 friend class content::BrowserThread;
  friend class base::DeleteHelper<SipccRendererHost>;


  // IPC Functionality performed on IO thread
  void OnInit();
  void OnSipPlaceCall(std::string dial_number);
  void OnSipAnswerCall();
  void OnSipHangUp();
  void OnSipRegister(std::string aor, std::string creds, std::string proxy, bool isLocal);
  void OnSipDeRegister();
  void OnReceiveEmptyCaptureBuffer(int buffer_id);
  void OnReceiveEmptyReceiveBuffer(int buffer_id);

   const content::ResourceContext* resource_context_;  
  DISALLOW_COPY_AND_ASSIGN(SipccRendererHost);
};

#endif  // CONTENT_BROWSER_RENDERER_HOST_SIPCC_RENDERER_HOST_H_
