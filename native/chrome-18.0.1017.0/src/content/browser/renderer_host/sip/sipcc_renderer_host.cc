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
	SipccController::GetInstance()->RemoveSipccControllerObserver();
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
    IPC_MESSAGE_HANDLER(SipHostMsg_UnRegister, OnSipDeRegister)
    IPC_MESSAGE_HANDLER(SipHostMsg_PlaceCall, OnSipPlaceCall)
    IPC_MESSAGE_HANDLER(SipHostMsg_Hangup, OnSipHangUp)
    IPC_MESSAGE_HANDLER(SipHostMsg_AnswerCall, OnSipAnswerCall)
    IPC_MESSAGE_HANDLER(SipHostMsg_CaptureBufferReady, OnReceiveEmptyCaptureBuffer)
    IPC_MESSAGE_HANDLER(SipHostMsg_ReceiveBufferReady, OnReceiveEmptyReceiveBuffer)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()

  return handled;
}

void SipccRendererHost::OnInit()
{

}

void SipccRendererHost::OnSipRegister(std::string aor,
									  std::string creds,
									  std::string proxy,
									  bool isLocal) 
{

	LOG(INFO) << "SUHAS: BrowserProcess:Attempting to REGISTER " << aor << "  " << proxy << " " << isLocal;
    SipccController::GetInstance()->SetRenderProcessHandle(peer_handle());
	SipccController::GetInstance()->AddSipccControllerObserver(this);
    //SipccController::GetInstance()->Register("snandakusip01","7223","","172.27.190.5", isLocal);
    SipccController::GetInstance()->Register(aor,creds,"",proxy, isLocal);
	LOG(INFO) << " REGISTERED ";
}

void SipccRendererHost::OnSipDeRegister()
{

	LOG(INFO) << "SipccRendererHost:OnSipDeRegister " ;
	SipccController::GetInstance()->RemoveSipccControllerObserver();
	SipccController::GetInstance()->UnRegister();

}

void SipccRendererHost::OnSipPlaceCall(std::string dial_number) {
  	DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
	LOG(INFO) << "SipccRendererHost:OnSipPlaceCall: dial number " << dial_number ;
	LOG(INFO) << " PLACE CALL";
  	//SipccController::GetInstance()->PlaceCall("7222", "", 0 , 0);
  	SipccController::GetInstance()->PlaceCall(dial_number, "", 0 , 0);
}

void SipccRendererHost::OnSipHangUp()
{

	LOG(INFO) << "SipccRendererHost:OnSipHangUp" ;
	SipccController::GetInstance()->EndCall();
	Send(new SipMsg_NotifySessionStateChanged("SIP_NOSESSION"));

}

void SipccRendererHost::OnSipAnswerCall()
{

  LOG(INFO) << "SipccRendererHost::OnSipAnswerCall " ;
  SipccController::GetInstance()->AnswerCall();
  Send(new SipMsg_NotifySessionStateChanged("SIP_INSESSION"));
}


void SipccRendererHost::OnReceiveEmptyCaptureBuffer(int buffer_id) 
{ 
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  LOG(INFO) << "SipccRendererHost::OnReceiveEmptyCaptureBuffer : " << buffer_id;
  SipccController::GetInstance()->ReturnCaptureBuffer(buffer_id);
}


void SipccRendererHost::OnReceiveEmptyReceiveBuffer(int buffer_id) 
{ 
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  LOG(INFO) << "SipccRendererHost::OnReceiveEmptyRecieveBuffer : " << buffer_id;
  SipccController::GetInstance()->ReturnReceiveBuffer(buffer_id);
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
					 std::string callingPartyNumber) 
{
	LOG(INFO)<<"SipccRendererHost::OnIncomigCall: caller number " << callingPartyNumber;
	Send(new SipMsg_IncomingCall(callingPartyName,callingPartyNumber,"SIP_ACCEPTINGSESSION"));
    
} 

void SipccRendererHost::OnRegisterStateChange(std::string registrationState) 
{
	LOG(INFO)<<"SipccRendererHost::OnRegisterStateChange: " << registrationState;
    std::string state_ = "SIP_REGINVALID";

    if(registrationState == "noRegistrar") {
		state_="SIP_NOREGISTRAR";
    }
    else if (registrationState == "registering")
		state_="SIP_REGISTERING";
    else if (registrationState == "registered"){
		state_="SIP_REGISTERED";
    }
    else if (registrationState == "registrationFailed")
		state_="SIP_REGISTRATIONFAILED";
    else {
        return;
    }

	Send(new SipMsg_NotifyRegistrationStateChanged(state_));	
}

void SipccRendererHost::OnCallTerminated() 
{

 	LOG(ERROR) << "SipccRendererHost::OnCallTerminated" ;
 	Send(new SipMsg_NotifySessionStateChanged("SIP_NOSESSION"));

}

void SipccRendererHost::OnCallConnected(char* sdp) {
 LOG(ERROR) << "SipccRendererHost::OnCallConnected" ;
 Send(new SipMsg_NotifySessionStateChanged("SIP_INSESSION"));
}

void SipccRendererHost::OnCallResume() {
 LOG(ERROR) << "SipccRendererHost::OnCallResume" ;
}

void SipccRendererHost::OnCallHeld() {
 LOG(ERROR) << "SipccRendererHost::OnCallHeld" ;
}


void SipccRendererHost::DoSendCaptureBufferCreated(base::SharedMemoryHandle handle,
                         							int length,
                         							int buffer_id)
{
	LOG(INFO) << " SipccRendererHost:: DoSendCaptureBufferCreated" << " " << length << " " << buffer_id;	
     Send(new SipMsg_NewCaptureBuffer(handle,length, buffer_id));
}

void SipccRendererHost::DoSendReceiveBufferCreated(base::SharedMemoryHandle handle,
                         							int length,
                         							int buffer_id)
{
	LOG(INFO) << " SipccRendererHost:: DoSendReceiveBufferCreated" << " " << length << " " << buffer_id;	
     Send(new SipMsg_NewReceiveBuffer(handle,length, buffer_id));
}

void SipccRendererHost::DoSendReceiveBufferFilled(int buffer_id,
                       		     					unsigned int timestamp)
{
   LOG(INFO) << "SipccRendererHost:: DoSendReceiveBufferFilled " << buffer_id << " " << timestamp;
   Send(new SipMsg_ReceiveBufferReady(buffer_id,timestamp));
}

void SipccRendererHost::DoSendCaptureBufferFilled(int buffer_id,
												  unsigned int timestamp)
{
	LOG(INFO) << "SipccRendererHost:: DoSendCaptureBufferFilled " << buffer_id << " " << timestamp;
	Send(new SipMsg_CaptureBufferReady(buffer_id,timestamp));
}
