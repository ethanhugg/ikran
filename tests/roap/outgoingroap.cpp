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

#include "outgoingroap.h"



void OutgoingRoap::push(string roapMessage)
{
  base::AutoLock lock(queueMutex);
  
  outboundQueue.push(roapMessage);
}

string OutgoingRoap::pop()
{
  base::AutoLock lock(queueMutex);
  string result;
  
  if (!outboundQueue.empty())
  {
    result = outboundQueue.front();
    outboundQueue.pop();
  }
  
  return result;
}

void OutgoingRoap::Init()
{
	SipccController::GetInstance()->AddSipccControllerObserver(this);
}

void OutgoingRoap::Shutdown()
{
	SipccController::GetInstance()->RemoveSipccControllerObserver();
}

void OutgoingRoap::Offer(string callerSessionId, string seq, string sdp)
{
  string roapMessage;
  
  roapMessage += "{ \"messageType\":\"OFFER\", \"callerSessionId\":\"";
  roapMessage += callerSessionId;
  roapMessage += "\", \"seq\":\"";
  roapMessage += seq;
  roapMessage += "\", \"sdp\":\"";
  roapMessage += sdp;
  roapMessage += "\" }";
  
  push(roapMessage);
}

void OutgoingRoap::Answer(string callerSessionId, string calleeSessionId, string seq, string sdp)
{
  string roapMessage;
  
  roapMessage += "{ \"messageType\":\"ANSWER\", \"callerSessionId\":\"";
  roapMessage += callerSessionId;
  roapMessage += "\", \"calleeSessionId:\"";
  roapMessage += calleeSessionId;
  roapMessage += "\", \"seq\":\"";
  roapMessage += seq;
  roapMessage += "\", \"sdp\":\"";
  roapMessage += sdp;
  roapMessage += "\" }";
  
  push(roapMessage);
}


void OutgoingRoap::OK(string callerSessionId, string calleeSessionId, string seq)
{
  string roapMessage;
  
  roapMessage += "{ \"messageType\":\"OK\", \"callerSessionId\":\"";
  roapMessage += callerSessionId;
  roapMessage += "\", \"seq\":\"";
  roapMessage += seq;
  roapMessage += "\" }";
  
  push(roapMessage);
}

void OutgoingRoap::TentativeAnswer(string callerSessionId, string calleeSessionId, string seq, string sdp)
{
  string roapMessage;
  
  roapMessage += "{ \"messageType\":\"TENTATIVE_ANSWER\", \"callerSessionId\":\"";
  roapMessage += callerSessionId;
  roapMessage += "\", \"calleeSessionId:\"";
  roapMessage += calleeSessionId;
  roapMessage += "\", \"seq\":\"";
  roapMessage += seq;
  roapMessage += "\", \"sdp\":\"";
  roapMessage += sdp;
  roapMessage += "\" }";
  
  push(roapMessage);
}

void OutgoingRoap::OnIncomingCall(std::string callingPartyName, std::string callingPartyNumber)
{}

void OutgoingRoap::OnRegisterStateChange(std::string registrationState)
{}

void OutgoingRoap::OnCallTerminated()
{}

void OutgoingRoap::OnCallConnected(char* sdp)
{
	if (strcmp(sdp,"") != 0)
		Answer("callerSessionId", "calleeSessionId", "seq", sdp);
}

void OutgoingRoap::OnCallHeld()
{}

void OutgoingRoap::OnCallResume()
{}
