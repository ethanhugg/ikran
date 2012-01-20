#ifndef _DEBUG_CLIENT
#define _DEBUG_CLIENT


#include <stdio.h>
#include <winsock2.h>
#include <conio.h> 
#include "base/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/lock.h"
#include <iostream>
#include "common.h"


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
	   ClientRecThread(CDebugClient *debugClient): m_debugClient(debugClient),base::SimpleThread("ClientRecThread") {};
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