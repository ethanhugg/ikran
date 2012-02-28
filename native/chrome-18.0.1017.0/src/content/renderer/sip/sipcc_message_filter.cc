// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/sip/sipcc_message_filter.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "content/common/child_process.h"
#include "content/common/sip/sip_messages.h"
#include "content/common/view_messages.h"
#include "base/logging.h"
#include "ipc/ipc_logging.h"

SipccMessageFilter::SipccMessageFilter()
    : channel_(NULL),
      message_loop_proxy_(NULL) {
    LOG(INFO) << " Sipcc Render Message Filter Created ";
}

SipccMessageFilter::~SipccMessageFilter() {
    LOG(INFO) << " Sipcc Render Message Filter DESTROYED ";
}

// Called on the IPC thread.
bool SipccMessageFilter::Send(IPC::Message* message) {
  LOG(INFO) << " Filter: Send  CHANNEL is " << channel_;
  if (!channel_) {
  LOG(INFO) << " Filter: Send : No Channel ";
    delete message;
    return false;
  }

  return channel_->Send(message);
}

bool SipccMessageFilter::OnMessageReceived(const IPC::Message& message) {

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(SipccMessageFilter, message)
    IPC_MESSAGE_HANDLER(SipMsg_CaptureBufferReady, OnCaptureBufferReceived)
    IPC_MESSAGE_HANDLER(SipMsg_ReceiveBufferReady, OnReceiveBufferReceived)
    IPC_MESSAGE_HANDLER(SipMsg_NewCaptureBuffer, OnCaptureBufferCreated)
    IPC_MESSAGE_HANDLER(SipMsg_NewReceiveBuffer, OnReceiveBufferCreated)
    IPC_MESSAGE_HANDLER(SipMsg_NotifyRegistrationStateChanged, OnRegistrationStateChanged)
    IPC_MESSAGE_HANDLER(SipMsg_NotifySessionStateChanged, OnSessionStateChanged)
    IPC_MESSAGE_HANDLER(SipMsg_IncomingCall, OnIncomingCall)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void SipccMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  // Captures the message loop for IPC.
  channel_ = channel;
  LOG(ERROR) << "SipccMessageFilter::OnFilterAdded: Channel " << channel_;
}

void SipccMessageFilter::OnFilterRemoved() {
  LOG(ERROR) << "SipccMessageFilter::OnFilterREMOVED" ;
  channel_ = NULL;
}

void SipccMessageFilter::OnChannelClosing() {
  LOG(ERROR) << "SipccMessageFilter:: CHANNEL CLOSED" ;
  channel_ = NULL;
}


void SipccMessageFilter::OnCaptureBufferCreated(
    base::SharedMemoryHandle handle,
    int length,
    int buffer_id) {

	if (!delegate_) {
    LOG(ERROR) << "SipccMessageFiler: OnCaptureBufferCreated: Got video frame buffer for a "
        "non-existent or removed video capture.";

    // Send the buffer back to Host in case it's waiting for all buffers
    // to be returned.
    base::SharedMemory::CloseHandle(handle);
    Send(new SipHostMsg_CaptureBufferReady(buffer_id));
    return;
  }
  LOG(INFO) << " SipccMessageFilter: New Capture Buffer Created: Notifying Delegate";
  delegate_->OnCaptureVideoBufferCreated(handle, length, buffer_id);
}


void SipccMessageFilter::OnReceiveBufferCreated(
    base::SharedMemoryHandle handle,
    int length,
    int buffer_id) {

	if (!delegate_) {
    LOG(ERROR) << "SipccMessageFiler: OnReceiveBufferCreated: Got video frame buffer for a "
        "non-existent or removed video capture.";

    // Send the buffer back to Host in case it's waiting for all buffers
    // to be returned.
    base::SharedMemory::CloseHandle(handle);
    Send(new SipHostMsg_ReceiveBufferReady(buffer_id));
    return;
  }
  LOG(INFO) << " SipccMessageFilter: New Receive Buffer Created: Notifying Delegate";
  delegate_->OnReceiveVideoBufferCreated(handle, length, buffer_id);
}



void SipccMessageFilter::OnCaptureBufferReceived(
    int buffer_id,
    unsigned int timestamp) {
  if (!delegate_) {
    LOG(ERROR) << "OnCaptureBufferReceived: Got video frame buffer for a "
        "non-existent or removed video capture.";

    // Send the buffer back to Host in case it's waiting for all buffers
    // to be returned.
    Send(new SipHostMsg_CaptureBufferReady(buffer_id));
    return;
  }
  LOG(INFO)<<" SipccMessageFilter: Capture Video Frame Received : Notifying Delegate";
  delegate_->OnCaptureVideoBufferReceived(buffer_id, timestamp);
}

void SipccMessageFilter::OnReceiveBufferReceived(
    int buffer_id,
    unsigned int timestamp) {
  if (!delegate_) {
    LOG(ERROR) << "OnReceiveBufferReceived: Got video frame buffer for a "
        "non-existent or removed video capture.";

    // Send the buffer back to Host in case it's waiting for all buffers
    // to be returned.
    Send(new SipHostMsg_ReceiveBufferReady(buffer_id));
    return;
  }
  LOG(INFO)<<" SipccMessageFilter: Receive Video Frame Received : Notifying Delegate";
  delegate_->OnReceiveVideoBufferReceived(buffer_id, timestamp);
}


void SipccMessageFilter::OnRegistrationStateChanged(std::string reg_state)
{

  	LOG(INFO)<<" SipccMessageFilter: SIP Registration State Change Notificatin ";
	if(delegate_)
	{
  		delegate_->OnSipRegStateChanged(reg_state);

	}

}


void SipccMessageFilter::OnSessionStateChanged(std::string session_state)
{

  	LOG(INFO)<<" SipccMessageFilter: SIP Session State Change Notificatin ";
	if(delegate_)
	{
  		delegate_->OnSipSessionStateChanged(session_state);
	}

}

void SipccMessageFilter::OnIncomingCall(std::string callingPartyName,
											std::string callingPartyNum,
											std::string state)
{
  	LOG(INFO)<<" SipccMessageFilter: SIP Incoming Call Notificatin ";
	if(delegate_)
	{
  		delegate_->OnSipIncomingCall(callingPartyName, callingPartyNum, state);
	}

}

void SipccMessageFilter::AddDelegate(Delegate* delegate) {
   LOG(INFO) <<" SipccMessageFilter: AddDelegate Invoked " << delegate;
   delegate_ = delegate;
}

void SipccMessageFilter::RemoveDelegate(Delegate* delegate) {
  delegate_ = 0;
}
