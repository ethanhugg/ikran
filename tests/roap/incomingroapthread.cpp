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
    addr.sin_port = htons(7627);
        
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
        string callerSessionId = (*itCallerSessionId).second;
        string seq = (*itSeq).second;
        string sdp = (*itSdp).second;

        CSFLogDebugS(logTag, "Calling OFFER");
        _incoming.Offer(callerSessionId, seq, sdp);
      }
    }
    else if (messageType.compare("ANSWER") == 0)
    {
    
    }
    else if (messageType.compare("OK") == 0)
    {
      
    }
    else if (messageType.compare("TENTATIVE_ANSWER") == 0)
    {
      
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
    }
  }
  
  CSFLogDebugS(logTag, "IncomingRoapThread End");
}

void IncomingRoapThread::shutdown()
{
  _shutdown = true;
  if (_socket != -1)
  {
    close(_socket);
  }
}

