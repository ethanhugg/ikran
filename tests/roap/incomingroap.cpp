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

#include <stdarg.h>

#include "roapapp.h"
#include "incomingroap.h"
#include "CSFLogStream.h"
#include "sipcc_controller.h"
#include <stdlib.h>

static const char* logTag = "RoapProxy";

void IncomingRoap::Init(string device, string user, string password, string domain)
{
  if (roapProxyCallState != CALLSTATE_NOT_REGISTERED)
  {
    CSFLogErrorS(logTag, "Init called in invalid state: " << roapProxyCallState);
  }
  else
  {
    SipccController::GetInstance()->SetProperty("transport", "tcp");
    
    int regResult = SipccController::GetInstance()->StartROAPProxy(device, user, password, domain);

    //zero means happy here
    //TODO - maybe finer grained results?
    if (regResult != 0) 
    {
      CSFLogDebugS(logTag, "ERROR registering with " << domain);
    }
    else 
    {
      CSFLogDebugS(logTag, "Successfully registered with " << domain);
      roapProxyCallState = CALLSTATE_REGISTERED;
    }
  }
}

void IncomingRoap::Offer(string callerSessionId, string seq, string sdp)
{
  if (roapProxyCallState != CALLSTATE_REGISTERED)
  {
    CSFLogErrorS(logTag, "Offer called in invalid state: " << roapProxyCallState);
  }
  else
  {
	  unsigned int ipIdx, aIdx, vIdx ;
	  std::string ipAddress, audioTxPort, videoTxPort;
	  ipIdx=aIdx=vIdx = -1;
	  //get IP Addrss index
	  ipIdx = sdp.find("IP4");
	  if(ipIdx == std::string::npos) {
		  CSFLogDebugS(logTag, "Unable to find ipaddress in the string" );
	  } else {
		  CSFLogDebugS(logTag, "Found  ipaddress in the string @ " << ipIdx );
	  }
	  //grab the ip address
	  ipAddress = sdp.substr(ipIdx+4,15);
	  CSFLogDebugS(logTag, " IP Address extracted is " << ipAddress.c_str());

	  //get the audioport Index
	  aIdx = sdp.find("m=audio");
	  if(aIdx == std::string::npos) {
		  CSFLogDebugS(logTag, "Unable to find audio line in the string" );
	  } else {
		  CSFLogDebugS(logTag, "Found  audiolinein the string @ " );
	  }
	  audioTxPort = sdp.substr(aIdx+8,13);
	  CSFLogDebugS(logTag, " Audio Port extracted is " << audioTxPort.c_str());

	  //get the vodeoport Index
	  vIdx = sdp.find("m=video");
	  if(vIdx == std::string::npos) {
		  CSFLogDebugS(logTag, "Unable to find video line in the string" );
	  } else {
	      CSFLogDebugS(logTag, "Found  vode line the string @ " );
	  }
	  videoTxPort=sdp.substr(vIdx+8,13);

	  CSFLogDebugS(logTag, " Video Port extracted is " << videoTxPort.c_str());

	  audioTxPort = "16384";
	  videoTxPort = "1024";

	  SipccController::GetInstance()->PlaceCall(callerSessionId, ipAddress.c_str(), atoi(audioTxPort.c_str()), atoi(videoTxPort.c_str()) );
	  //roapProxyCallState = CALLSTATE_IN_CALL;
  }
}

void IncomingRoap::Answer(string callerSessionId, string calleeSessionId, string seq, string sdp)
{
  
}

void IncomingRoap::OK(string callerSessionId, string calleeSessionId, string seq)
{
  
}

void IncomingRoap::TentativeAnswer(string callerSessionId, string calleeSessionId, string seq, string sdp)
{
  
}



