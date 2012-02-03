// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/shared_memory.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"

#define IPC_MESSAGE_START SipMsgStart


// Init the stuff 
IPC_MESSAGE_CONTROL0(SipHostMsg_Init)

//  Tell SIP stack to Register 
IPC_MESSAGE_CONTROL0(SipHostMsg_Register)

//  Tell SIP stack to place the call
IPC_MESSAGE_CONTROL1(SipHostMsg_PlaceCall,
                      std::string )

// Tell the renderer process that a new buffer is allocated for video .
IPC_MESSAGE_CONTROL3(SipMsg_NewBuffer,
                     base::SharedMemoryHandle /* handle */,
                     int /* length */,
                     int /* buffer_id */)

// Tell the renderer process that a buffer is available from video .
IPC_MESSAGE_CONTROL2(SipMsg_BufferReady,
                     int /* buffer_id */,
                     unsigned int /* timestamp */)


IPC_MESSAGE_CONTROL1(SipHostMsg_BufferReady,
                     int /* buffer_id */)

