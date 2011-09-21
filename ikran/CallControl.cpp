/****** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Video for Jetpack.
 *
 * The Initial Developer of the Original Code is Mozilla Labs.
 * Portions created by the Initial Developer are Copyright (C) 2009-10
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ethan Hugg      <ehugg@cisco.com> 
 *   Enda Mannion    <emannion@cisco.com>
 *   Suhas Nandakumar<snandaku@cisco.com>
 *   Anant Narayanan <anant@kix.in>
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

#include "CallControl.h"
#include "assert.h"
#include "Logger.h"


NS_IMPL_ISUPPORTS1(CallControl, ICallControl)

CallControl* CallControl::gCallControlService = nsnull;
CallControl*
CallControl::GetSingleton()
{
    if (gCallControlService) {
        gCallControlService->AddRef();
        return gCallControlService;
    }
    gCallControlService = new CallControl();
    if (gCallControlService) {
        gCallControlService->AddRef();
        if (NS_FAILED(gCallControlService->Init()))
            gCallControlService->Release();
    }
    return gCallControlService;
}

/*
 * This class reports various events related to SIP
 * registration and other signalling
 */
class SessionCallback : public nsRunnable 
{
public:
    SessionCallback(nsISessionStateObserver *obs, const char *msg, const char *arg) {
        m_S_Obs = obs;
        strcpy(m_Msg, msg);
        strcpy(m_Arg, arg);
    }
    
    NS_IMETHOD Run() {
        return m_S_Obs->OnSessionStateChange((char *)m_Msg, (char *)m_Arg);
    }
    
private:
    nsCOMPtr<nsISessionStateObserver> m_S_Obs;
    char m_Msg[128]; char m_Arg[256];
};

/*
 * This class reports various events related Media 
 * state of currently active session
 */
class MediaCallback : public nsRunnable {
public:
    MediaCallback(nsIMediaStateObserver *obs, const char *msg, const char *arg) {
        m_M_Obs = obs;
        strcpy(m_Msg, msg);
        strcpy(m_Arg, arg);
    }
   
    NS_IMETHOD Run() {
        return m_M_Obs->OnMediaStateChange((char *)m_Msg, (char *)m_Arg);
    }
   
private:
    nsCOMPtr<nsIMediaStateObserver> m_M_Obs;
    char m_Msg[128]; char m_Arg[256];
};



/*
 * === Class methods begin now! ===
 */
nsresult
CallControl::Init()
{
    m_session = PR_FALSE;
	m_registered = PR_FALSE;
	vSource = 0;
    return NS_OK;
}

CallControl::~CallControl()
{

    gCallControlService = nsnull;
}

/*
 * Callback method from Sip Call Controller
 * on Incoming call event
 */
void CallControl::OnIncomingCall(std::string callingPartyName,
									std::string callingPartyNumber)
{


 	NS_DispatchToMainThread(new SessionCallback(
        sessionObserver, "incoming-call", ""
    ));

} 

/*
 * Callback from Sip Call Controller on
 * change in SIP Registration State
 */
void CallControl::OnRegisterStateChange(std::string registrationState)
{

	NS_DispatchToMainThread(new SessionCallback(
        sessionObserver, registrationState.c_str(), ""
    ));
} 

/*
 * Callback from Sip Call Controller on
 * termination of call (local/remote)
 */
void CallControl::OnCallTerminated()
{
	gCallControlService->m_session = PR_FALSE;
	NS_DispatchToMainThread(new MediaCallback(
        mediaObserver, "call-terminated", ""
    ));
} 

/*
 * Callback from Sip Call Controller on
 * media connected between the peers 
 */
void CallControl::OnCallConnected()
{
	 NS_DispatchToMainThread(new MediaCallback(
        mediaObserver, "call-connected", ""
    ));
} 

/*
 * Start ikran session
 * Performs SIP registration 
 */
NS_IMETHODIMP
CallControl::RegisterUser(
    const char* user_device,
    const char* user,
    const char* credentials,
    const char* proxy_address,
    nsISessionStateObserver *obs
)
{
	//copy the parameters	
    sessionObserver = obs;
    m_user_device = const_cast<char*>(user_device);
    m_user = const_cast<char*>(user);
    m_credentials  = const_cast<char*>(credentials);
    m_proxy_address = const_cast<char*>(proxy_address);

    if (m_registered) {
        NS_DispatchToMainThread(new SessionCallback(
            sessionObserver, "error", "User Already Registered"
        ));
        return NS_ERROR_FAILURE;
    }
	

	//lets give it a shot
	SipccController::GetInstance()->AddSipccControllerObserver(this);
	int res = SipccController::GetInstance()->Register(m_user_device, m_user, m_credentials, m_proxy_address);
	if(res == 0) {
		m_registered = PR_TRUE;	
    	return NS_OK;
	}
	else
		return NS_ERROR_FAILURE;
}


/*
 * Start ikran session
 * Performs SIP registration
 */
NS_IMETHODIMP
CallControl::StartP2PMode(
    const char* user,
    nsISessionStateObserver *obs
)
{
	//copy the parameters
    sessionObserver = obs;
    m_user = const_cast<char*>(user);

    if (m_registered) {
        NS_DispatchToMainThread(new SessionCallback(
            sessionObserver, "error", "User Already Registered"
        ));
        return NS_ERROR_FAILURE;
    }


	//lets give it a shot
	SipccController::GetInstance()->AddSipccControllerObserver(this);
	int res = SipccController::GetInstance()->StartP2PMode(m_user);
	if(res == 0) {
		m_registered = PR_TRUE;
    	return NS_OK;
	}
	else
		return NS_ERROR_FAILURE;
}


/*
 * UnRegister User 
 */
NS_IMETHODIMP
CallControl::UnregisterUser()
{
    if (!m_registered) {
        NS_DispatchToMainThread(new SessionCallback(
            sessionObserver, "error", "no session in progress"
        ));
        return NS_ERROR_FAILURE;
    }
    
	SipccController::GetInstance()->RemoveSipccControllerObserver();
	SipccController::GetInstance()->UnRegister();
	m_registered = PR_FALSE;	
    return NS_OK;
}

/*
 * Initiate VOIP call to dn
 * for registered user
 */
NS_IMETHODIMP
CallControl::PlaceCall(const char* dn,
						nsIDOMCanvasRenderingContext2D *ctx,
						nsIMediaStateObserver* obs)
{
	mediaObserver = obs;
	vCanvas = ctx;
	if(m_session) {
		Logger::Instance()->logIt("Place Call: call is in progress");
        NS_DispatchToMainThread(new MediaCallback(
            mediaObserver, "error", "call is in progress"
        ));
        return NS_ERROR_FAILURE;
	}
	m_dial_number = const_cast<char*>(dn);
	if(vSource == 0)
	{
		//hardcoding the width and height for now	
		vSource = new VideoRenderer(640,480, vCanvas);
	 	Logger::Instance()->logIt("Vsource is null in PlaceCall");
	}
	SipccController::GetInstance()->SetExternalRenderer(vSource);
	SipccController::GetInstance()->PlaceCall(m_dial_number);
	m_session = PR_TRUE;
	return NS_OK;
}

/*
 * Initiate VOIP P2P call to dn and IP Address
 * for local peer user
 */
NS_IMETHODIMP
CallControl::PlaceP2PCall(const char* dn,
						const char* ip_address,
						nsIDOMCanvasRenderingContext2D *ctx,
						nsIMediaStateObserver* obs)
{
	mediaObserver = obs;
	vCanvas = ctx;
	if(m_session) {
		Logger::Instance()->logIt("Place P2P Call: call is in progress");
        NS_DispatchToMainThread(new MediaCallback(
            mediaObserver, "error", "call is in progress"
        ));
        return NS_ERROR_FAILURE;
	}
	m_dial_number = const_cast<char*>(dn);
	m_local_ip_address = const_cast<char*>(ip_address);
	if(vSource == 0)
	{
		//hardcoding the width and height for now
		vSource = new VideoRenderer(640, 480, vCanvas);
	 	Logger::Instance()->logIt("Vsource is null in PlaceCall");
	}
	SipccController::GetInstance()->SetExternalRenderer(vSource);
	SipccController::GetInstance()->PlaceP2PCall(m_dial_number, m_local_ip_address);
	m_session = PR_TRUE;
	return NS_OK;
}


/*
 * End Call 
 */
NS_IMETHODIMP
CallControl::HangupCall()
{
	if(!m_session) {
		Logger::Instance()->logIt("HangupCall: no call is in progress");
        NS_DispatchToMainThread(new MediaCallback(
            mediaObserver, "error", " no call is in progress"
        ));
        return NS_ERROR_FAILURE;
	}
	SipccController::GetInstance()->EndCall();
	m_session = PR_FALSE;	
    return NS_OK;
}

/*
 * Answer Incoming Call for
 * registered user
 */

NS_IMETHODIMP
CallControl::AnswerCall(nsIDOMCanvasRenderingContext2D *ctx,
				nsIMediaStateObserver* obs)
{
	mediaObserver = obs;
	vCanvas = ctx;
    if(m_session) {
		Logger::Instance()->logIt("AnswerCall: no call is in progress");
        NS_DispatchToMainThread(new MediaCallback(
            mediaObserver, "error", " Call is in progress"
        ));
        return NS_ERROR_FAILURE;
     }

	if(vSource == 0)
	{
	   Logger::Instance()->logIt("Vsource is null in AnswerCall");
	   vSource = new VideoRenderer(640,480, vCanvas);
	}
	if(vCanvas == 0 || vCanvas == nsnull)
		Logger::Instance()->logIt(" CANVAS IS NULL MAN");
	SipccController::GetInstance()->SetExternalRenderer(vSource);
        SipccController::GetInstance()->AnswerCall();
        m_session = PR_TRUE;
    return NS_OK;
}

