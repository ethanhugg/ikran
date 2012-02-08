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
 *  Kai Chen <kaichen2@cisco.com>
 *  Yi Wang <yiw2@cisco.com>
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
 
#ifndef _DEBUG_SERVER
#define _DEBUG_SERVER

#include "sockporting.h"

#include <stdio.h>
//#include <winsock2.h>
//#include <conio.h> 
#include<list>
#include <iostream>

#include <string>
#include "cc_constants.h"
#include "SharedPtr.h"
#include "base/threading/simple_thread.h"
#include "Common.h"
using namespace std;



class CDebugServer : public base::SimpleThread
{
public:
	CDebugServer();
	~CDebugServer();
	virtual void Run();

	class  ServerRecThread : public base::SimpleThread
	{
	public:
	   //ServerRecThread(CDebugServer *debugServer,SOCKET sRecSocket): m_debugServer(debugServer),m_sRecSocket(sRecSocket),base::SimpleThread("ServerRecThread") {};
	   ServerRecThread(CDebugServer *debugServer,SOCKET sRecSocket): base::SimpleThread("ServerRecThread"),m_debugServer(debugServer),m_sRecSocket(sRecSocket) {};
	   ~ServerRecThread(){};
		virtual void Run();
	private:
		CDebugServer *m_debugServer;
		SOCKET m_sRecSocket;
	
	};

	bool IsConnected(){return m_bIsConnected;} // returns connection status
	void StartListenClient(); // Listen to client
	int SendMessagePort(string sMessage); // Send message to sll clients.
	int RecClient(SOCKET sRecSocket); // receive message for a particulat socket
	int GetClientList(){return m_vClientList.size();};

	
	void setCallback(UserOperationCallback* callback);

private:
	bool m_bIsConnected; // true - connected false - not connected
	int m_iServerPort;
	list<SOCKET> m_vClientList; // All socket connected to client
	SOCKET m_SClient;
	SOCKET m_SListenClient; // socket listening for client calls

	UserOperationCallback* callback;
	void createUserOperationAndSignal (eUserOperationRequest request, void * pData = NULL);
	void ExecuteCmd(const char* cmd);
	
};

#endif
