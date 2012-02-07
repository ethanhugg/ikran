// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/sip/sipcc_renderer_host.h"

#include "base/bind.h"
#include "base/process.h"
#include "base/shared_memory.h"
#include "base/logging.h"
#include "content/common/sip/sip_messages.h"
#include "ipc/ipc_logging.h"
#include "content/browser/resource_context.h"
#include "content/renderer/sip/sipcc_renderer_impl.h"

using content::BrowserMessageFilter;
using content::BrowserThread;


///////////////////////////////////////////////////////////////////////////////
// SipccRendererHost implementations.
SipccRendererHost::SipccRendererHost(
	 const content::ResourceContext* resource_context)
         : resource_context_(resource_context) {
    LOG(INFO) <<" SipccRendererHost:: SipccRendererHost() : Created ";
}



SipccRendererHost::~SipccRendererHost() {
	LOG(INFO) << "SipRenderererHost:: Destructor";
	//SipccController::GetInstance()->RemoveSipccControllerObserver();
}

void SipccRendererHost::OnChannelClosing() {
  BrowserMessageFilter::OnChannelClosing();

}

void SipccRendererHost::OnDestruct() const {
  BrowserThread::DeleteOnIOThread::Destruct(this);
}


///////////////////////////////////////////////////////////////////////////////
// IPC Messages handler
bool SipccRendererHost::OnMessageReceived(const IPC::Message& message,
                                          bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(SipccRendererHost, message, *message_was_ok)
    IPC_MESSAGE_HANDLER(SipHostMsg_Init, OnInit)
    IPC_MESSAGE_HANDLER(SipHostMsg_Register, OnSipRegister)
    IPC_MESSAGE_HANDLER(SipHostMsg_PlaceCall, OnSipPlaceCall)
    IPC_MESSAGE_HANDLER(SipHostMsg_BufferReady, OnReceiveEmptyBuffer)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()

  return handled;
}

void SipccRendererHost::OnInit()
{

}

void SipccRendererHost::OnSipRegister() {

	LOG(INFO) << " Attempting to REGISTER ";
        SipccController::GetInstance()->SetRenderProcessHandle(peer_handle());
	SipccController::GetInstance()->AddSipccControllerObserver(this);
        SipccController::GetInstance()->Register("snandakusip01","7223","","172.27.190.5");
	LOG(INFO) << " REGISTERED ";

}
void SipccRendererHost::OnSipPlaceCall(std::string dial_number) {
  	DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
	LOG(INFO) << "SipccRendererHost:OnSipPlaceCall: " ;
	LOG(INFO) << " PLACE CALL";
  	SipccController::GetInstance()->PlaceCall("7222", "", 0 , 0);
}


void SipccRendererHost::OnReceiveEmptyBuffer(int buffer_id) { 
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  LOG(INFO) << "SipccRendererHost::OnReceiveEmptyBuffer : " << buffer_id;
  SipccController::GetInstance()->ReturnBuffer(buffer_id);

}


/*
void SipccRendererHost::SendErrorMessage(int32 render_view_id,
                                         int32 stream_id) {
  ViewMsg_AudioStreamState_Params state;
  state.state = ViewMsg_AudioStreamState_Params::kError;
  Send(new ViewMsg_NotifyAudioStreamStateChanged(
      render_view_id, stream_id, state));
}
*/

////////////////////////////////////////////////////////////////////////////////
//// SipccControllerObserver Implementation

 void SipccRendererHost::OnIncomingCall(std::string callingPartyName, 
					 std::string callingPartyNumber) {

} 

void SipccRendererHost::OnRegisterStateChange(std::string registrationState) {

}

void SipccRendererHost::OnCallTerminated() {
 LOG(ERROR) << "SipccRendererHost::OnCallTerminated" ;
}

void SipccRendererHost::OnCallConnected(char* sdp) {
 LOG(ERROR) << "SipccRendererHost::OnCallConnected" ;
}

void SipccRendererHost::OnCallResume() {
 LOG(ERROR) << "SipccRendererHost::OnCallTerminated" ;
}

void SipccRendererHost::OnCallHeld() {
 LOG(ERROR) << "SipccRendererHost::OnCallTerminated" ;
}


void SipccRendererHost::DoSendBufferCreated(base::SharedMemoryHandle handle,
                         		int length,
                         		int buffer_id)

{
	LOG(INFO) << " SipccRendererHost:: DoSendBufferCreated" << " " << length << " " << buffer_id;	
     Send(new SipMsg_NewBuffer(handle,length, buffer_id));
}

void SipccRendererHost::DoSendBufferFilled(int buffer_id,
                       		     unsigned int timestamp)
{
   LOG(INFO) << "SipccRendererHist:: DoSendBufferFilled " << buffer_id << " " << timestamp;
   Send(new SipMsg_BufferReady(buffer_id,timestamp));

}
