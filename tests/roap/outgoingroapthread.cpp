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
#include "outgoingroapthread.h"

static const char* logTag = "RoapProxy";
static const unsigned short outgoing_roap_port = 7628;

void OutgoingRoapThread::initialize()
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
    addr.sin_port = htons(outgoing_roap_port);
    
    if (bind(_socket, (sockaddr *)&addr, sizeof(addr)) != 0)
    {
      CSFLogDebugS(logTag, "Unable to bind socket");
      close(_socket);
      _socket = -1;
    }
    else if (listen(_socket, 5) != 0)
    {
      CSFLogDebugS(logTag, "Unable to listen on socket");
      close(_socket);
      _socket = -1;
    }
    else
    {
      CSFLogDebugS(logTag, "Successful Bind and Listen");
    }
  }
}

bool OutgoingRoapThread::nextConnection()
{
  bool success = false;
  socklen_t addrlen= sizeof(struct sockaddr_in);
  struct sockaddr_in address;
  
  if (_socket == -1)
  {
    CSFLogDebugS(logTag, "nextConnection - no socket");
  }
  else
  {
    int newSocket = accept(_socket, (struct sockaddr *)&address, &addrlen);
    
    if (newSocket < 0)
    {
      CSFLogDebugS(logTag, "Accept failed");
    }
    else
    {
      int i;
      
      CSFLogDebugS(logTag, "Accept Success");
      
      for (i= 0; i < 60; i++)
      {
        string next = _outgoing.pop();
        
        if (next.empty())
        {
          sleep(5);
        }
        else
        {
          CSFLogDebugS(logTag, "nextConnection sending " << next);
          send(newSocket, next.c_str(), next.length(), 0);
          break;  
        }
      }
      
      CSFLogDebugS(logTag, "nextConnection complete");
      close(newSocket);
      
      success = true;
    }
  }
  
  return success;
}


void OutgoingRoapThread::Run()
{
  CSFLogDebugS(logTag, "OutgoingRoapThread Start");
  
  initialize();
  
  _outgoing.Init();

  // TEST CODE
//  string callerSessionId = "callerId";
//  string calleeSessionId = "calleeId";
//  string seq = "seq123";
//  string sdp = "sdp456";
//  
//  _outgoing.Offer(callerSessionId, seq, sdp);
//  _outgoing.Answer(callerSessionId, calleeSessionId, seq, sdp);
//  _outgoing.OK(callerSessionId, calleeSessionId, seq);
//  _outgoing.TentativeAnswer(callerSessionId, calleeSessionId, seq, sdp);
  // END TEST
  
  while (!_shutdown)
  {
    if (!nextConnection())
    {
      break;
    }
  }
  
  CSFLogDebugS(logTag, "OutgoingRoapThread End");
}

void OutgoingRoapThread::shutdown()
{
  CSFLogDebugS(logTag, "OutgoingRoapThread shutdown called");
  
  _outgoing.Shutdown();

  _shutdown = true;
  if (_socket != -1)
  {
    close(_socket);
  }
}



