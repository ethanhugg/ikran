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
 
#ifndef _DEBUG_CLIENT
#define _DEBUG_CLIENT

#include "sockporting.h"

#include <stdio.h>
//#include <winsock2.h>
//#include <conio.h> 
#include "base/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/lock.h"
#include <iostream>
#include "Common.h"


using namespace std;


class CDebugClient : public base::SimpleThread
{
public:
	CDebugClient();
	~CDebugClient();
	void Init(string sIpAddress, int iPort);
	int SendMessagePort(string sMessage);
	int RecMessagePort();
	bool IsConnected(){return m_bIsConnected;}

	class  ClientRecThread : public base::SimpleThread
	{
	public:
	   //ClientRecThread(CDebugClient *debugClient): m_debugClient(debugClient),base::SimpleThread("ClientRecThread") {};
	   ClientRecThread(CDebugClient *debugClient): base::SimpleThread("ClientRecThread"),m_debugClient(debugClient) {};
	   ~ClientRecThread(){};
		virtual void Run();
	private:
		CDebugClient *m_debugClient;
		
	};

	virtual void Run();

	UserOperationCallback* callback;
	void createUserOperationAndSignal (eUserOperationRequest request, void * pData = NULL);
	void createUserOperation(eUserOperationRequest request, void * pData = NULL);
	void createSignal (eUserOperationRequest request, void * pData = NULL);
	void ExecuteCmd(const char* cmd);

	void setCallback(UserOperationCallback* callback);

	void StopRecv();

private:
	void StartRecv();
	
	bool m_bIsConnected; // true - connected false - not connected
	string m_sServerIPAddress;
	int m_iServerPort;
	SOCKET conn; // socket connected to server
	ClientRecThread *clientRecThread;
};
#endif
