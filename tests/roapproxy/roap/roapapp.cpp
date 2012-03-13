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
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include "getopt.h"
#include <string.h>
#include <sys/time.h>
#include "common.h"
#include "outgoingroap.h"
#include "incomingroap.h"
#include "jsonparser.h"
#include "libwebsockets.h"

#include <stdarg.h>
#include "csf_common.h"
#include "CSFLogStream.h"
#include "debug-psipcc-types.h"
#include "base/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/lock.h"
#include "roapapp.h"
#include "gettimeofday.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>


static int close_testing;
static const char* logTag = "RoapProxy";

int roapProxyCallState = CALLSTATE_NOT_REGISTERED;

IncomingRoap _incoming;
OutgoingRoap _outgoing;



void HandleMessage(map<string, string> message)
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
				//CSFLogDebugS(logTag, "INIT is missing device");
			}
			else if (itUser == message.end())
			{
				//CSFLogDebugS(logTag, "INIT is missing user");
			}
			else if (itPassword == message.end())
			{
				//CSFLogDebugS(logTag, "INIT is missing password");
			}
			else if (itDomain == message.end())
			{
				//CSFLogDebugS(logTag, "INIT is missing domain");
			}
			else
			{
				//CSFLogDebugS(logTag, "Calling INIT");
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
				//CSFLogDebugS(logTag, "OFFER is missing callerSessionId");  
			}
			else if (itSeq == message.end())
			{
				//CSFLogDebugS(logTag, "OFFER is missing seq");  
			}
			else if (itSdp == message.end())
			{
				//CSFLogDebugS(logTag, "OFFER is missing sdp");  
			}
			else
			{
				//CSFLogDebugS(logTag, "Calling OFFER");
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
				//CSFLogDebugS(logTag, "ANSWER is missing callerSessionId");  
			}
			else if (itCalleeSessionId == message.end())
			{
				//CSFLogDebugS(logTag, "ANSWER is missing calleeSessionId");  
			}
			else if (itSeq == message.end())
			{
				//CSFLogDebugS(logTag, "ANSWER is missing seq");  
			}
			else if (itSdp == message.end())
			{
				//CSFLogDebugS(logTag, "ANSWER is missing sdp");  
			}
			else
			{
				//CSFLogDebugS(logTag, "Calling ANSWER");
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
				//CSFLogDebugS(logTag, "OK is missing callerSessionId");  
			}
			else if (itCalleeSessionId == message.end())
			{
				//CSFLogDebugS(logTag, "OK is missing calleeSessionId");  
			}
			else if (itSeq == message.end())
			{
				//CSFLogDebugS(logTag, "OK is missing seq");  
			}
			else
			{
				//CSFLogDebugS(logTag, "Calling OK");
				_incoming.OK((*itCallerSessionId).second, (*itCalleeSessionId).second, (*itSeq).second);
			}
		}
		else if (messageType.compare("SHUTDOWN") == 0)
		{
			map<string,string>::iterator itCallerSessionId = message.find("callerSessionId");
			map<string,string>::iterator itCalleeSessionId = message.find("calleeSessionId");
			map<string,string>::iterator itSeq = message.find("seq");

			if (itCallerSessionId == message.end())
			{
				//CSFLogDebugS(logTag, "SHUTDOWN is missing callerSessionId");  
			}
			else if (itCalleeSessionId == message.end())
			{
				//CSFLogDebugS(logTag, "SHUTDOWN is missing calleeSessionId");  
			}
			else if (itSeq == message.end())
			{
				//CSFLogDebugS(logTag, "SHUTDOWN is missing seq");  
			}
			else
			{
				//CSFLogDebugS(logTag, "Calling SHUTDOWN");
				_incoming.Shutdown((*itCallerSessionId).second, (*itCalleeSessionId).second, (*itSeq).second);
			}
		}
		else
		{
			//CSFLogDebugS(logTag, "Unknown messageType: " << messageType);
		}
	}
}


enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_DUMB_INCREMENT,
	PROTOCOL_LWS_MIRROR,

	/* always last */
	DEMO_PROTOCOL_COUNT
};


#define LOCAL_RESOURCE_PATH DATADIR"/libwebsockets-roap-server"

/* this protocol server (always the first one) just knows how to do HTTP */

static int callback_http(struct libwebsocket_context * context,
struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason, void *user,
	void *in, size_t len)
{
	char client_name[128];
	char client_ip[128];

	switch (reason) {
	case LWS_CALLBACK_HTTP:
		fprintf(stderr, "serving HTTP URI %s\n", (char *)in);

		/* send the script... when it runs it'll start websockets */

		if (in && strcmp((char*)in, "/jquery-1.6.2.js") == 0) {
			if (libwebsockets_serve_http_file(wsi,
				LOCAL_RESOURCE_PATH"/jquery-1.6.2.js", "text/javascript"))
				fprintf(stderr, "Failed to send HTTP file\n");
			break;
		}

		if (in && strcmp((char*)in, "/jquery-ui-1.8.16.custom.min.js") == 0) {
			if (libwebsockets_serve_http_file(wsi,
				LOCAL_RESOURCE_PATH"/jquery-ui-1.8.16.custom.min.js", "text/javascript"))
				fprintf(stderr, "Failed to send HTTP file\n");
			break;
		}


		if (in && strcmp((char*)in, "/json2.js") == 0) {
			if (libwebsockets_serve_http_file(wsi,
				LOCAL_RESOURCE_PATH"/json2.js", "text/javascript"))
				fprintf(stderr, "Failed to send HTTP file\n");
			break;
		}



		if (libwebsockets_serve_http_file(wsi,
			LOCAL_RESOURCE_PATH"/roap.html", "text/html"))
			fprintf(stderr, "Failed to send HTTP file\n");
		break;



		/*
		* callback for confirming to continue with client IP appear in
		* protocol 0 callback since no websocket protocol has been agreed
		* yet.  You can just ignore this if you won't filter on client IP
		* since the default uhandled callback return is 0 meaning let the
		* connection continue.
		*/

	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:

		libwebsockets_get_peer_addresses((int)(long)user, client_name,
			sizeof(client_name), client_ip, sizeof(client_ip));

		fprintf(stderr, "Received network connect from %s (%s)\n",
			client_name, client_ip);

		/* if we returned non-zero from here, we kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/*
* this is just an example of parsing handshake headers, you don't need this
* in your code unless you will filter allowing connections by the header
* content
*/

static void
	dump_handshake_info(struct lws_tokens *lwst)
{
	int n;
	static const char *token_names[WSI_TOKEN_COUNT] = {
		/*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
		/*[WSI_TOKEN_HOST]		=*/ "Host",
		/*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
		/*[WSI_TOKEN_KEY1]		=*/ "key 1",
		/*[WSI_TOKEN_KEY2]		=*/ "key 2",
		/*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
		/*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
		/*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
		/*[WSI_TOKEN_DRAFT]		=*/ "Draft",
		/*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",

		/* new for 04 */
		/*[WSI_TOKEN_KEY]		=*/ "Key",
		/*[WSI_TOKEN_VERSION]		=*/ "Version",
		/*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",

		/* new for 05 */
		/*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",

		/* client receives these */
		/*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
		/*[WSI_TOKEN_NONCE]		=*/ "Nonce",
		/*[WSI_TOKEN_HTTP]		=*/ "Http",
		/*[WSI_TOKEN_MUXURL]	=*/ "MuxURL",
	};

	for (n = 0; n < WSI_TOKEN_COUNT; n++) {
		if (lwst[n].token == NULL)
			continue;

		fprintf(stderr, "    %s = %s\n", token_names[n], lwst[n].token);
	}
}

/* dumb_increment protocol */

/*
* one of these is auto-created for each connection and a pointer to the
* appropriate instance is passed to the callback in the user parameter
*
* for this example protocol we use it to individualize the count for each
* connection.
*/

struct per_session_data__dumb_increment {
	int number;
};

static int
	callback_roap_proxy(struct libwebsocket_context * context,
struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len)
{
	int n;
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 +
		LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	struct per_session_data__dumb_increment *pss = (per_session_data__dumb_increment *)user;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		fprintf(stderr, "callback_roap_proxy: LWS_CALLBACK_ESTABLISHED\n");
		pss->number = 0;
		_outgoing.Init();
		break;

		

		/*
		* in this protocol, we just use the broadcast action as the chance to
		* send our own connection-specific data and ignore the broadcast info
		* that is available in the 'in' parameter
		*/
	case LWS_CALLBACK_CLOSED:
		_outgoing.Stop();

	case LWS_CALLBACK_BROADCAST:
		
		break;

	case LWS_CALLBACK_RECEIVE:{

		map<string,string> messageMap = JsonParser::Parse((char*)in);

		HandleMessage(messageMap);
							  }
		break;
		/*
		* this just demonstrates how to use the protocol filter. If you won't
		* study and reject connections based on header content, you don't need
		* to handle this callback
		*/

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info((struct lws_tokens *)(long)user);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/* lws-mirror_protocol */

#define MAX_MESSAGE_QUEUE 64

struct per_session_data__lws_mirror {
	struct libwebsocket *wsi;
	int ringbuffer_tail;
};

struct a_message {
	void *payload;
	size_t len;
};

static struct a_message ringbuffer[MAX_MESSAGE_QUEUE];
static int ringbuffer_head;


/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
			callback_http,		/* callback */
			0			/* per_session_data_size */
	},
	{
		"dumb-increment-protocol",
			callback_roap_proxy,
			sizeof(struct per_session_data__dumb_increment),
		},
	
		{
			NULL, NULL, 0		/* End of list */
			}
};

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "killmask",	no_argument,		NULL, 'k' },
	{ "interface",  required_argument, 	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },
	{ NULL, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	int n = 0;
	const char *cert_path =
		LOCAL_RESOURCE_PATH"/libwebsockets-test-server.pem";
	const char *key_path =
		LOCAL_RESOURCE_PATH"/libwebsockets-test-server.key.pem";
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 1024 +
		LWS_SEND_BUFFER_POST_PADDING];
	int port = 7681;
	int use_ssl = 0;
	struct libwebsocket_context *context;
	int opts = 0;
	char interface_name[128] = "";
	const char * interfaces = NULL;
#ifdef LWS_NO_FORK
	unsigned int oldus = 0;
#endif

	fprintf(stderr, "Roap Proxy test server\n");

	while (n >= 0) {
		n = getopt_long(argc, argv, "ci:khsp:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
		case 's':
			use_ssl = 1;
			break;
		case 'k':
			opts = LWS_SERVER_OPTION_DEFEAT_CLIENT_MASK;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			interfaces = interface_name;
			break;
		case 'c':
			close_testing = 1;
			fprintf(stderr, " Close testing mode -- closes on "
				"client after 50 dumb increments"
				"and suppresses lws_mirror spam\n");
			break;
		case 'h':
			fprintf(stderr, "Usage: test-server "
				"[--port=<p>] [--ssl]\n");
			exit(1);
		}
	}

	if (!use_ssl)
		cert_path = key_path = NULL;

	context = libwebsocket_create_context(port, interfaces, protocols,
		libwebsocket_internal_extensions,
		cert_path, key_path, -1, -1, opts);
	if (context == NULL) {
		fprintf(stderr, "libwebsocket init failed\n");
		return -1;
	}

	buf[LWS_SEND_BUFFER_PRE_PADDING] = 'x';

#ifdef LWS_NO_FORK

	/*
	* This example shows how to work with no forked service loop
	*/

	fprintf(stderr, " Using no-fork service loop\n");

	while (1) {
		struct timeval tv;

		gettimeofday(&tv, NULL);

		
		libwebsocket_service(context, 50);
	}

#else

	/*
	* This example shows how to work with the forked websocket service loop
	*/

	fprintf(stderr, " Using forked service loop\n");

	/*
	* This forks the websocket service action into a subprocess so we
	* don't have to take care about it.
	*/

	n = libwebsockets_fork_service_loop(context);
	if (n < 0) {
		fprintf(stderr, "Unable to fork service loop %d\n", n);
		return 1;
	}

	while (1) {

		usleep(50000);

	}

#endif

	libwebsocket_context_destroy(context);

	return 0;
}

