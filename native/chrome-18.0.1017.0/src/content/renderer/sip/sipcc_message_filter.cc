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
    IPC_MESSAGE_HANDLER(SipMsg_BufferReady, OnBufferReceived)
    IPC_MESSAGE_HANDLER(SipMsg_NewBuffer, OnBufferCreated)
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


void SipccMessageFilter::OnBufferCreated(
    base::SharedMemoryHandle handle,
    int length,
    int buffer_id) {

	if (!delegate_) {
    LOG(ERROR) << "SipccMessageFiler: OnBufferCreated: Got video frame buffer for a "
        "non-existent or removed video capture.";

    // Send the buffer back to Host in case it's waiting for all buffers
    // to be returned.
    base::SharedMemory::CloseHandle(handle);
    Send(new SipHostMsg_BufferReady(buffer_id));
    return;
  }
  LOG(INFO) << " SipccMessageFilter: New Buffer Created: Notifying Delegate";
  delegate_->OnVideoBufferCreated(handle, length, buffer_id);
}



void SipccMessageFilter::OnBufferReceived(
    int buffer_id,
    unsigned int timestamp) {
  if (!delegate_) {
    LOG(ERROR) << "OnBufferReceived: Got video frame buffer for a "
        "non-existent or removed video capture.";

    // Send the buffer back to Host in case it's waiting for all buffers
    // to be returned.
    Send(new SipHostMsg_BufferReady(buffer_id));
    return;
  }
  LOG(INFO)<<" SipccMessageFilter: Video Frame Received : Notifying Delegate";
  delegate_->OnVideoBufferReceived(buffer_id, timestamp);
}


void SipccMessageFilter::AddDelegate(Delegate* delegate) {
   delegate_ = delegate;
}

void SipccMessageFilter::RemoveDelegate(Delegate* delegate) {
  delegate_ = 0;
}
