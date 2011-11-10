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
 *   Anant Narayanan <anant@kix.com>
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

#ifndef CallControl_h_ 
#define CallConrol_h_ 

#include "ICallControl.h"


#include <prmem.h>
#include <plbase64.h>
#include <prthread.h>

#include <nsIPipe.h>
#include <nsIFileStreams.h>
#include <nsIAsyncInputStream.h>
#include <nsIAsyncOutputStream.h>
#include <nsIDOMCanvasRenderingContext2D.h>
#include <nsEmbedString.h>

#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsStringAPI.h>
#include <nsComponentManagerUtils.h>
#include <nsAutoPtr.h>
#include <nsThreadUtils.h>
#include "video_renderer.h"


//#include "VideoRenderer.h" //<-- External Webrtc Renderer
//#include "VideoSourceCanvas.h"
#include "sipcc_controller.h"

#define CALL_CONTROL_CONTRACTID "@cto.cisco.com/call/control;1"
#define CALL_CONTROL_CID { 0xc467b1f4, 0x551c, 0x4e2f, \
                           { 0xa6, 0xba, 0xcb, 0x7d, 0x79, 0x2d, 0x52, 0x44 }}

class CallControl : public ICallControl,
					public SipccControllerObserver 
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_ICALLCONTROL

    nsresult Init();
    static CallControl* GetSingleton();
    virtual ~CallControl();
    CallControl(){}

	virtual void OnIncomingCall(std::string callingPartyName, std::string callingPartyNumber);
 	virtual void OnRegisterStateChange(std::string registrationState);
 	virtual void OnCallTerminated(); 
	virtual void OnCallConnected();
	virtual void OnCallHeld();
	virtual void OnCallResume();
	
	void ParseProperties(nsIPropertyBag2* prop);

protected:

    PRThread *thread;
    PRBool m_session;
	PRBool m_registered;
	PRBool m_callHeld;
    PRLogModuleInfo *log;
    
	nsCOMPtr<nsISessionStateObserver> sessionObserver; 
	nsCOMPtr<nsIMediaStateObserver> mediaObserver; 

	static CallControl *gCallControlService;

	//for webrtc video rendering support
	//TBD: can be moved into more design centric code later
	nsIDOMCanvasRenderingContext2D *vCanvas;
	VideoRenderer *vSource;

private:
	char *m_user_device;
	char *m_user;
	char *m_credentials;
	char *m_proxy_address;
	char *m_dial_number;
	char *m_local_ip_address;
};

#endif
