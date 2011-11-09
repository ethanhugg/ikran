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

#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "CSFLogStream.h"
#include "jsonparser.h"
#include "incomingroapthread.h"

static const char* logTag = "RoapProxy";
static const unsigned short incoming_roap_port = 7627;

void IncomingRoapThread::initialize()
{
  _socket = socket(AF_INET, SOCK_STREAM, 0);
  
  if (_socket == -1)
  {
    CSFLogDebugS(logTag, "Unable to create socket");
  }
  else
  {
    struct sockaddr_in addr;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(incoming_roap_port);
        
    if (bind(_socket, (sockaddr *)&addr, sizeof(addr)) != 0)
    {
      CSFLogDebugS(logTag, "Unable to bind socket");
    }
    else if (listen(_socket, 5) != 0)
    {
      CSFLogDebugS(logTag, "Unable to listen on socket");
    }
    else
    {
      CSFLogDebugS(logTag, "Successful Bind and Listen");
    }
  }
}

string IncomingRoapThread::nextMessage()
{
  string next;
  socklen_t addrlen= sizeof(struct sockaddr_in);
  struct sockaddr_in address;

  int newSocket = accept(_socket, (struct sockaddr *)&address, &addrlen);
  
  if (newSocket < 0)
  {
    CSFLogDebugS(logTag, "Accept failed");
  }
  else
  {
    CSFLogDebugS(logTag, "Accept Success");
    int countRecv;
    const unsigned buffer_length = 1024;
    char buffer[buffer_length];
    
    while ((countRecv= recv(newSocket, buffer, buffer_length, 0)) > 0)
    {
      next.append(buffer, countRecv);  
    }
    
    close(newSocket);
    CSFLogDebugS(logTag, "Received: " << next);
  }
  
  
  return next;
}

void IncomingRoapThread::HandleMessage(map<string, string> message)
{
  map<string,string>::iterator it;
  
  it = message.find("messageType");
  if (it == message.end())
  {
    CSFLogDebugS(logTag, "messageType not found");
  }
  else
  {
    string messageType = (*it).second;
    CSFLogDebugS(logTag, "messageType is " << messageType);
    
    if (messageType.compare("INIT") == 0)
    {
      map<string,string>::iterator itDevice = message.find("device");
      map<string,string>::iterator itUser = message.find("user");
      map<string,string>::iterator itPassword = message.find("password");
      map<string,string>::iterator itDomain = message.find("domain");
      
      if (itDevice == message.end())
      {
        CSFLogDebugS(logTag, "INIT is missing device");
      }
      else if (itUser == message.end())
      {
        CSFLogDebugS(logTag, "INIT is missing user");
      }
      else if (itPassword == message.end())
      {
        CSFLogDebugS(logTag, "INIT is missing password");
      }
      else if (itDomain == message.end())
      {
        CSFLogDebugS(logTag, "INIT is missing domain");
      }
      else
      {
        CSFLogDebugS(logTag, "Calling INIT");
        _incoming.Init((*itDevice).second, (*itUser).second, (*itPassword).second, (*itDomain).second);
      }
    }
    else if (messageType.compare("OFFER") == 0)
    {
      map<string,string>::iterator itCallerSessionId = message.find("callerSessionId");
      map<string,string>::iterator itSeq = message.find("seq");
      map<string,string>::iterator itSdp = message.find("sdp");
      
      if (itCallerSessionId == message.end())
      {
        CSFLogDebugS(logTag, "OFFER is missing callerSessionId");  
      }
      else if (itSeq == message.end())
      {
        CSFLogDebugS(logTag, "OFFER is missing seq");  
      }
      else if (itSdp == message.end())
      {
        CSFLogDebugS(logTag, "OFFER is missing sdp");  
      }
      else
      {
        CSFLogDebugS(logTag, "Calling OFFER");
        _incoming.Offer((*itCallerSessionId).second, (*itSeq).second, (*itSdp).second);
      }
    }
    else if (messageType.compare("ANSWER") == 0)
    {
      map<string,string>::iterator itCallerSessionId = message.find("callerSessionId");
      map<string,string>::iterator itCalleeSessionId = message.find("calleeSessionId");
      map<string,string>::iterator itSeq = message.find("seq");
      map<string,string>::iterator itSdp = message.find("sdp");
      
      if (itCallerSessionId == message.end())
      {
        CSFLogDebugS(logTag, "ANSWER is missing callerSessionId");  
      }
      else if (itCalleeSessionId == message.end())
      {
        CSFLogDebugS(logTag, "ANSWER is missing calleeSessionId");  
      }
      else if (itSeq == message.end())
      {
        CSFLogDebugS(logTag, "ANSWER is missing seq");  
      }
      else if (itSdp == message.end())
      {
        CSFLogDebugS(logTag, "ANSWER is missing sdp");  
      }
      else
      {
        CSFLogDebugS(logTag, "Calling ANSWER");
        _incoming.Answer((*itCallerSessionId).second, (*itCalleeSessionId).second, (*itSeq).second, (*itSdp).second);
      }
    }
    else if (messageType.compare("OK") == 0)
    {
      map<string,string>::iterator itCallerSessionId = message.find("callerSessionId");
      map<string,string>::iterator itCalleeSessionId = message.find("calleeSessionId");
      map<string,string>::iterator itSeq = message.find("seq");
      
      if (itCallerSessionId == message.end())
      {
        CSFLogDebugS(logTag, "OK is missing callerSessionId");  
      }
      else if (itCalleeSessionId == message.end())
      {
        CSFLogDebugS(logTag, "OK is missing calleeSessionId");  
      }
      else if (itSeq == message.end())
      {
        CSFLogDebugS(logTag, "OK is missing seq");  
      }
      else
      {
        CSFLogDebugS(logTag, "Calling OK");
        _incoming.OK((*itCallerSessionId).second, (*itCalleeSessionId).second, (*itSeq).second);
      }
    }
    else if (messageType.compare("TENTATIVE_ANSWER") == 0)
    {
      map<string,string>::iterator itCallerSessionId = message.find("callerSessionId");
      map<string,string>::iterator itCalleeSessionId = message.find("calleeSessionId");
      map<string,string>::iterator itSeq = message.find("seq");
      map<string,string>::iterator itSdp = message.find("sdp");
      
      if (itCallerSessionId == message.end())
      {
        CSFLogDebugS(logTag, "TENTATIVE_ANSWER is missing callerSessionId");  
      }
      else if (itCalleeSessionId == message.end())
      {
        CSFLogDebugS(logTag, "TENTATIVE_ANSWER is missing calleeSessionId");  
      }
      else if (itSeq == message.end())
      {
        CSFLogDebugS(logTag, "TENTATIVE_ANSWER is missing seq");  
      }
      else if (itSdp == message.end())
      {
        CSFLogDebugS(logTag, "TENTATIVE_ANSWER is missing sdp");  
      }
      else
      {
        CSFLogDebugS(logTag, "Calling TENTATIVE_ANSWER");
        _incoming.TentativeAnswer((*itCallerSessionId).second, (*itCalleeSessionId).second, (*itSeq).second, (*itSdp).second);
      }
    }
    else
    {
      CSFLogDebugS(logTag, "Unknown messageType: " << messageType);
    }
  }
}

void IncomingRoapThread::Run()
{
  string next;
  
  CSFLogDebugS(logTag, "IncomingRoapThread Start");

  // TEMP TEST
//  string test;
//  test += "{ \"messageType\":\"OFFER\", \"callerSessionId\":\"1234567890\", \"seq\":123, \"sdp\":\"sdpstuff\" }";
//  map<string,string> message = JsonParser::Parse(test);
//  map<string,string>::iterator it;
//  for (it=message.begin(); it != message.end(); it++)
//  {
//    CSFLogDebugS(logTag, "name/value map: " << (*it).first << " and " << (*it).second << " -- ");
//  }
//  HandleMessage(message);
//  shutdown();
  // END TEMP TEST
  
  initialize();
  
  while (!_shutdown)
  {
    next = nextMessage();
    
    if (next.length() > 0)
    {
      map<string,string> messageMap = JsonParser::Parse(next);
      
      if (messageMap.empty())
      {
        CSFLogDebugS(logTag, "Parse returned empty map");
      }
      else
      {
        HandleMessage(messageMap);
      }
    }
  }
  
  CSFLogDebugS(logTag, "IncomingRoapThread End");
}

void IncomingRoapThread::shutdown()
{
  CSFLogDebugS("IncomingRoapThread shutdown called");

  _shutdown = true;
  if (_socket != -1)
  {
    close(_socket);
  }
}

