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
IPC_MESSAGE_CONTROL4(SipHostMsg_Register,
		     std::string,
		     std::string,
		     std::string,
		     bool)

// Tell SIP stack to UnRegister
IPC_MESSAGE_CONTROL0(SipHostMsg_UnRegister)

//  Tell SIP stack to place the call
IPC_MESSAGE_CONTROL1(SipHostMsg_PlaceCall,
                      std::string )

// Tell SIP stack to Hangupcall 
IPC_MESSAGE_CONTROL0(SipHostMsg_Hangup)

// Tell SIP stack to AnswerCall 
IPC_MESSAGE_CONTROL0(SipHostMsg_AnswerCall)

// Return the video buffer for remote stream to browser
IPC_MESSAGE_CONTROL1(SipHostMsg_ReceiveBufferReady,
                     int /* buffer_id */)

// Return the video buffer for local capture to browser
IPC_MESSAGE_CONTROL1(SipHostMsg_CaptureBufferReady,
                     int /* buffer_id */)

// Tell the renderer process that a new buffer is allocated for video .
IPC_MESSAGE_CONTROL3(SipMsg_NewCaptureBuffer,
                     base::SharedMemoryHandle /* handle */,
                     int /* length */,
                     int /* buffer_id */)

// Tell the renderer process that a new buffer is allocated for video .
IPC_MESSAGE_CONTROL3(SipMsg_NewReceiveBuffer,
                     base::SharedMemoryHandle /* handle */,
                     int /* length */,
                     int /* buffer_id */)

// Tell the sip renderer handler of reg state change
IPC_MESSAGE_CONTROL1(SipMsg_NotifyRegistrationStateChanged,
                     std::string)


// Tell the sip renderer handler of session state change
IPC_MESSAGE_CONTROL1(SipMsg_NotifySessionStateChanged,
                     std::string)

// Notify the sip renderer of an incoming call
IPC_MESSAGE_CONTROL3(SipMsg_IncomingCall,
                     std::string,
		     		 std::string,
		    		 std::string)

// Tell the renderer process that a buffer is available from video  capture.
IPC_MESSAGE_CONTROL2(SipMsg_CaptureBufferReady,
                     int /* buffer_id */,
                     unsigned int /* timestamp */)


// Tell the renderer process that a buffer is available from remote video 
IPC_MESSAGE_CONTROL2(SipMsg_ReceiveBufferReady,
                     int /* buffer_id */,
                     unsigned int /* timestamp */)

