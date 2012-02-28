// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MessageFilter that handles audio messages and delegates them to audio
// renderers. Created on render thread, AudioMessageFilter is operated on
// IO thread (main thread of render process), it intercepts audio messages
// and process them on IO thread since these messages are time critical.

#ifndef CHROME_RENDERER_SIPCC_MESSAGE_FILTER_H_
#define CHROME_RENDERER_SIPCC_MESSAGE_FILTER_H_
#pragma once

#include <map>

#include "base/message_loop_proxy.h"
#include "base/shared_memory.h"
#include "content/common/content_export.h"
#include "base/id_map.h"
#include "base/sync_socket.h"
#include "ipc/ipc_channel_proxy.h"



class SipccMessageFilter 
      : public IPC::ChannelProxy::MessageFilter {
 public:
  class Delegate {
   public:
    virtual void OnCaptureVideoBufferCreated(base::SharedMemoryHandle handle,
				      							int length, int buffer_id) = 0;

    virtual void OnReceiveVideoBufferCreated(base::SharedMemoryHandle handle,
				      							int length, int buffer_id) = 0;

    // Called when a video frame buffer is received from the browser process.
    virtual void OnCaptureVideoBufferReceived(int buffer_id, unsigned timestamp) = 0;
    virtual void OnReceiveVideoBufferReceived(int buffer_id, unsigned timestamp) = 0;

	virtual void OnSipRegStateChanged(const std::string reg_state) = 0;	
	virtual void OnSipSessionStateChanged(const std::string reg_state) = 0;	
    virtual void OnSipIncomingCall(std::string callingPartyName, std::string callingPartyNum,std::string state) = 0;

   protected:
    virtual ~Delegate() {}
  };

  SipccMessageFilter();
  virtual ~SipccMessageFilter();

  // Add a delegate to the map and return id of the entry.
  void AddDelegate(Delegate* delegate);

  // Remove a delegate referenced by |id| from the map.
  void RemoveDelegate(Delegate* delegate);

  // Sends an IPC message using |channel_|.
  bool Send(IPC::Message* message);


 private:



  // IPC::ChannelProxy::MessageFilter override. Called on IO thread.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void OnFilterAdded(IPC::Channel* channel) OVERRIDE;
  virtual void OnFilterRemoved() OVERRIDE;
  virtual void OnChannelClosing() OVERRIDE;

  // Receive a newly created buffer from browser process.
  void OnCaptureBufferCreated(base::SharedMemoryHandle handle,
		       					int length, int buffer_id);

  void OnReceiveBufferCreated(base::SharedMemoryHandle handle,
		       					int length, int buffer_id);

  // Receive a buffer from browser process.
  void OnCaptureBufferReceived(int buffer_id, unsigned int timestamp);
  void OnReceiveBufferReceived(int buffer_id, unsigned int timestamp);


  // Sip stack notified on registration state change
  void OnRegistrationStateChanged(std::string reg_state);
  
  //Sip stack notified on session state change
  void OnSessionStateChanged(std::string session_state);
  
  //Sip stack notified on incoming call
  void OnIncomingCall(std::string callingPartyName, std::string callingPartyNum,
						 std::string state);

  // A map of stream ids to delegates.
  Delegate* delegate_;
  IPC::Channel* channel_;

  scoped_refptr<base::MessageLoopProxy> message_loop_proxy_;


  DISALLOW_COPY_AND_ASSIGN(SipccMessageFilter);
};

#endif  // CHROME_RENDERER_SIPCC_MESSAGE_FILTER_H_
