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
 
#include "DebugServer.h"
#include "csf_common.h"
#include "CSFLogStream.h"
#include <iostream>


static const char* logTag = "DebugServer";




CDebugServer::CDebugServer():base::SimpleThread("ServerListenThread") 
{
	//cout << "Starting up Debug server\n";

	m_bIsConnected = false;

	WSADATA wsaData;

	sockaddr_in local;

	int wsaret=WSAStartup(0x101,&wsaData);

	if(wsaret!=0)
	{
		return;
	}

	local.sin_family=AF_INET; 
	local.sin_addr.s_addr=INADDR_ANY; 
	local.sin_port=htons((u_short)8084); 

	m_SListenClient=socket(AF_INET,SOCK_STREAM,0);


	if(m_SListenClient==INVALID_SOCKET)
	{
		return;
	}


	if(bind(m_SListenClient,(sockaddr*)&local,sizeof(local))!=0)
	{
		return;
	}


	if(listen(m_SListenClient,10)!=0)
	{
		return;
	}

	m_bIsConnected = true;
	return;
}



CDebugServer::~CDebugServer()
{
	closesocket(m_SListenClient);

	WSACleanup();
}

void CDebugServer::StartListenClient()
{

	sockaddr_in from;
	socklen_t fromlen=sizeof(from);

	m_SClient=accept(m_SListenClient,
		(struct sockaddr*)&from,&fromlen);

	if(m_SClient != INVALID_SOCKET)
		m_vClientList.push_back(m_SClient);


	ServerRecThread *serverRecThread = new ServerRecThread(this,m_SClient);
	serverRecThread->Start();
	

}



int CDebugServer::SendMessagePort(string sMessage)
{
	int iStat = 0;
	list<SOCKET>::iterator itl;

	if(m_vClientList.size() == 0)
		return 0;

	for(itl = m_vClientList.begin();itl != m_vClientList.end();itl++)
	{
		iStat = send(*itl,sMessage.c_str(),sMessage.size()+1,0);			
		if(iStat == -1)
			m_vClientList.remove(*itl);
	}

	if(iStat == -1)
		return 1;

	return 0;

}

int CDebugServer::RecClient(SOCKET sRecSocket)
{
	char temp[4096];
	int iResult;

	//cout <<inet_ntoa(from.sin_addr) <<":"<<temp<<"\r\n";
	iResult = recv(sRecSocket,temp,4096,0);

	
	if(iResult <= 0 )
	{
		m_vClientList.remove(sRecSocket);
		return 1;
	}
	else
	{
		cout <<":"<<temp<<"\n";
		ExecuteCmd(temp);
		SendMessagePort(temp);
		return 0;
	}
	return 0;

}

void CDebugServer::ServerRecThread::Run()
{
	while(1)
	{
		if(m_sRecSocket != 0 && m_debugServer != NULL)
			if(m_debugServer->RecClient(m_sRecSocket))
				break;
	}
}

void CDebugServer::Run()
{
	while(1)
		StartListenClient();
	
}


void CDebugServer::ExecuteCmd(const char* cmd)
{

	switch (*cmd)
	{
	case 'd':
		//createUserOperationAndSignal(eOriginatePhoneCall, new string(getInput("Enter DN# to call: ")));
		createUserOperationAndSignal(eOriginatePhoneCall, new string("1038"));
		break;
	case 'p':
		//createUserOperationAndSignal(eOriginateP2PPhoneCall, new string(getInput("Enter DN# to call P2P: ")));
		createUserOperationAndSignal(eOriginateP2PPhoneCall, new string("1038"));
		break;
		break;
	case 'a':
		{
			createUserOperationAndSignal(eAnswerCall);
			string temp = "eAnswerCall";
			SendMessagePort(temp);
		}
		break;
	case 'e':
		{
			createUserOperationAndSignal(eEndFirstCallWithEndCallCaps);
			string temp = "sEndCall";
			SendMessagePort(temp);
		}
		break;
	case 'h':
		createUserOperationAndSignal(eHoldFirstCallWithHoldCaps);
		break;
	case 'r':
		createUserOperationAndSignal(eResumeFirstCallWithResumeCaps);
		break;
	case 'm':
		createUserOperationAndSignal(eMuteAudioForConnectedCall);
		break;
	case 'n':
		createUserOperationAndSignal(eUnmuteAudioForConnectedCall);
		break;
	case 'k':
		createUserOperationAndSignal(eMuteVideoForConnectedCall);
		break;
	case 'j':
		createUserOperationAndSignal(eUnmuteVideoForConnectedCall);
		break;
	case 'v':
		createUserOperationAndSignal(eCycleThroughVideoPrefOptions);
		break;
	case 'l':
		createUserOperationAndSignal(ePrintActiveCalls);
		break;
	case '+':
		createUserOperationAndSignal(eVolumeUp);
		break;
	case '-':
		createUserOperationAndSignal(eVolumeDown);
		break;
	case 't':
		createUserOperationAndSignal(eToggleAutoAnswer);
		break;
	case 'z':
		createUserOperationAndSignal(eToggleShowVideoAutomatically);
		break;
	case 'x':
		createUserOperationAndSignal(eAddVideoToConnectedCall);
		break;
	case 'y':
		createUserOperationAndSignal(eRemoveVideoFromConnectedCall);
		break;
	case 'w':
		createUserOperationAndSignal(eDestroyAndCreateWindow);
		break;
	case 'q':
		createUserOperationAndSignal(eQuit);
		break;
	case '?':
		CSFLogDebugS(logTag, "\nl = print list of all calls");
		CSFLogDebugS(logTag, "\nd = dial");
		CSFLogDebugS(logTag, "\np = dialP2P");
		CSFLogDebugS(logTag, "\na = answer (incoming call)");
		CSFLogDebug(logTag, "\nv = cycle to next video pref option (current=%s)", getUserFriendlyNameForVideoPref(getActiveVideoPref()));
		CSFLogDebugS(logTag, "\ne = end call");
		CSFLogDebugS(logTag, "\nh = hold");
		CSFLogDebugS(logTag, "\nr = resume");
		CSFLogDebugS(logTag, "\nm = audio mute");
		CSFLogDebugS(logTag, "\nn = audio unmute");
		CSFLogDebugS(logTag, "\nk = video mute");
		CSFLogDebugS(logTag, "\nj = video unmute");
		CSFLogDebugS(logTag, "\n+ = volume up");
		CSFLogDebugS(logTag, "\n- = volume down");
#ifndef NOVIDEO
		CSFLogDebugS(logTag, "\nx = Add video to connected call (escalate)");
		CSFLogDebugS(logTag, "\ny = Remove video from connected call (de-escalate)");
		CSFLogDebugS(logTag, "\nz = toggle show-video-automatically");
#endif
		CSFLogDebugS(logTag, "\nt = toggle auto-answer");
		CSFLogDebugS(logTag, "\nw = destroy video window and create a new one");
		CSFLogDebugS(logTag, "\n0123456789#*ABCD = send digit");
		CSFLogDebugS(logTag, "\nq = quit");
		CSFLogDebugS(logTag, "\n");
		break;
	}
}


void CDebugServer::createUserOperationAndSignal (eUserOperationRequest request, void * pData)
{
	UserOperationRequestDataPtr pOperation(new UserOperationRequestData(request, pData));
	if(callback != NULL)
	{
		callback->onUserRequest(pOperation);
	}
}

void CDebugServer::setCallback(UserOperationCallback* callback)
{
	this->callback = callback;
}



