/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _SIPCC_CONTROLLER_HOST_H_
#define _SIPCC_CONTROLLER_HOST_H_
#pragma once

#include <map>
#include <set>
#include <list>

#ifndef WIN32
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#endif

#include "CallControlManager.h"
#include "CSFLog.h"
#include "CC_Call.h"
#include "CC_CallInfo.h"
#include "CC_Device.h"
#include "CC_DeviceInfo.h"
#include "CC_DeviceInfo.h"

#ifndef WIN32
typedef int SOCKET;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
#endif

using namespace CSF;


// Observer for events to be propogated to WebKit
class SipccControllerObserver {
public:	
	virtual void OnIncomingCall(std::string callingPartyName, std::string callingPartyNumber) = 0;
	virtual void OnRegisterStateChange(std::string registrationState) = 0;
 	virtual void OnCallTerminated() = 0;   // do we specify if terminated is local or remote
	virtual void OnCallConnected() = 0;
	
};


class SipccController : CC_Observer, ECC_Observer {
                          
public:

	//Singleton Object
	static SipccController* GetInstance();

   	//Registration and Session Operations 
	int Register(std::string device, std::string sipUser, std::string sipCredentials, std::string sipDomain);
	void UnRegister();
    void PlaceCall( std::string dial_number);
	void EndCall();
	void AnswerCall();
	
	void SetProperty(std::string key, std::string value);
	std::string GetProperty(std::string key);

	int StartP2PMode(std::string sipUser);
	void PlaceP2PCall(std::string dial_number,  std::string sipDomain);

	void SendDigits(std::string digits);

	void SetExternalRenderer(void* renderer) {
		ext_renderer = renderer;
	}

	void SetVideoWindow(void* videoWin) {
		video_window =  videoWin;
	}

	// Sip Stack Event Callback-Observer functions
	void onDeviceEvent(ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_DeviceInfoPtr info);
	void onFeatureEvent(ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_FeatureInfoPtr feature_info);
	void onLineEvent(ccapi_line_event_e lineEvent,     CC_LinePtr line,     CC_LineInfoPtr info);
	void onCallEvent(ccapi_call_event_e callEvent,     CC_CallPtr call,     CC_CallInfoPtr info);	
	
	void onAvailablePhoneEvent(AvailablePhoneEventType::AvailablePhoneEvent event,const PhoneDetailsPtr availablePhoneDetails);
	void onAuthenticationStatusChange(AuthenticationStatusEnum::AuthenticationStatus);
	void onConnectionStatusChange(ConnectionStatusEnum::ConnectionStatus status);	

	void AddSipccControllerObserver(SipccControllerObserver* observer) {
		if(observer == NULL)
			return;
		observer_ = observer;
	}

	void RemoveSipccControllerObserver() {
		observer_ = NULL;
	}

private:
	
	SipccController();
	~SipccController();

	CC_CallPtr GetFirstCallWithCapability(CallControlManagerPtr ccmPtr, CC_CallCapabilityEnum::CC_CallCapability cap);
	void InitInternal();
	bool RegisterInternal();
#ifndef WIN32
	std::string NetAddressToString(const struct sockaddr*, socklen_t); 
#endif
	bool GetLocalActiveInterfaceAddress();

    //Session Information, passed in from the webkit
	
    std::string sip_domain_;
    std::string user_name_; 
    std::string device_;
    std::string sip_user_;
    std::string sip_credentials_;
    std::string destination_url_; // destination sip-address or phone number to call
    std::string dial_number_;
    std::string sipDomain_;
	//Controllers local database
    std::string local_ip_v4_address_;

    //Device and Line info, needed by ECC
    std::string device_name_;
    std::string  preferred_line_;
    CC_DevicePtr device_ptr_;
    CC_CallPtr	outgoing_call_;	
    std::string localVoipPort;
    std::string remoteVoipPort;
    std::string transport;
    
	//State Variables for Reg and Session
    bool initDone;    

	//video window handle
	void* video_window;
	
	//external webrtc renderer
	void* ext_renderer;

    //CallControlManager
	CallControlManagerPtr ccm_ptr_; 
	
	// singleton instance
	static SipccController* _instance;

	//sole observer
	SipccControllerObserver* observer_;

};

#endif  // CONTENT_BROWSER_RENDERER_HOST_SIPCC_CONTROLLER_HOST_H_
