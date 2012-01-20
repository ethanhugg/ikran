#ifndef _DEBUG_SERVER
#define _DEBUG_SERVER

#include <stdio.h>
#include <winsock2.h>
#include <conio.h> 
#include<list>
#include <iostream>

#include <string>
#include "cc_constants.h"
#include "SharedPtr.h"
#include "base/threading/simple_thread.h"
#include "common.h"
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
	   ServerRecThread(CDebugServer *debugServer,SOCKET sRecSocket): m_debugServer(debugServer),m_sRecSocket(sRecSocket),base::SimpleThread("ServerRecThread") {};
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