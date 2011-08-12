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

#include "csf_common.h"
#include "CSFLog.h"

#include "CCMCIPClient.h"
#include "base/base64.h"

extern "C" {
#include "curl/curl.h"
}

#include <iostream>
#include <sstream>

using namespace std;
using namespace CSFUnified;

#define CCMCIP_CONNECTION_TIMEOUT 9 //seconds

static const char* logTag = "CCMCIPClient";

// Write any errors in here
static char curlErrorBuffer[CURL_ERROR_SIZE];

// Write all expected data in here
static string curlBuffer;

static int curlCallback(char* data, size_t size, size_t nmemb, std::string* buffer);

// This is the debug call back function used by curl
static int curlDebugCallback (CURL* curl, curl_infotype intoType, char* msg, size_t msgSize);

//Did make these static data members of the class CCMCIPClient
static CURL * curl = NULL;
static unsigned int curlRefCount = 0;

CCMCIPClient::CCMCIPClient(void)
{
    if ((curlRefCount > 0) || ((curlRefCount == 0) && ((curl = curl_easy_init()) != NULL)))
    {
        ++curlRefCount;
    }
}

CCMCIPClient::~CCMCIPClient(void)
{
    if (curlRefCount == 0)
    {
    	CSFLogWarnS(logTag, "Didn't expect curlRefCount to have a value of 0 on entry to CCMCIPClient::~CCMCIPClient(). Perhaps curl_easy_init() failed.");
    }
    else
    {
        --curlRefCount;

        if (curlRefCount == 0)
        {
            assert(curl != NULL);
            // always cleanup
            curl_easy_cleanup(curl);
            curl = NULL;
        }
    }
}

/* static */
bool CCMCIPClient::fetchDevices(const string& host, const string& userName, const string& password, const DeviceRetrieveCertLevel& certLevel,
                                DeviceMap & deviceMap, int * pErrorCode)
{
    // first clear the cached list of devices
    deviceMap.clear();
    // Erase the curlbuffer before use - Fix for DE1364
    curlBuffer.erase();
    // build the CCMCIP URL from the call manager host
    string url = "https://" + host + ":8443/ccmcip/Personalization";
    
    CSFLogInfo(logTag, "Retrieving devices via CCMCIP at %s", url.c_str());

    // Use the CURL easy interface

    CCMCIPClient client; //If not instantiated outside of this class then we need to make sure
                         //that curl_easy_init() is called here (before we perform any operations using CURL API).

    CURLcode result = CURLE_FAILED_INIT;

    if (curl)
    {
        // Now set up all of the curl options
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlErrorBuffer);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlBuffer);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CCMCIP_CONNECTION_TIMEOUT);

        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curlDebugCallback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        if (certLevel == SIGNED_CERT)
        {
            // TO DO
            //curl_easy_setopt(curl, CURLOPT_CAPATH, "");
        }
        else if (certLevel == SELF_SIGNED_CERT)
        {
            // TO DO
        }
        else
        {
            // do not verify certificate
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }

        string encodedPassword;
        base::Base64Encode(password, &encodedPassword);

        string userpwd = userName + ":" + encodedPassword;
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);


        // build the header list
        struct curl_slist *headers=NULL;
        headers = curl_slist_append(headers, "Content-Type: text/xml");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // the POST content to get the list of devices associated to the user
        string post = "<getDevices><user>" + userName + "</user></getDevices>";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());

        // send the request
        result = curl_easy_perform(curl);

        // free the header list
        curl_slist_free_all(headers);

        if (result == CURLE_OK)
        {
        	long httpResult;
        	result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpResult);
        	if(result == CURLE_OK && httpResult == 200)
        	{
        		return createDeviceListFromXML(curlBuffer, deviceMap, pErrorCode);
        	}
        	else if(result == CURLE_OK && httpResult == 401)
        	{
                setErrorCode(pErrorCode, DEVICE_RETRIEVE_AUTH_FAILED);
        	}
        	else
        	{
                setErrorCode(pErrorCode, DEVICE_RETRIEVE_TIMEOUT);
        	}
        }
        else if (result == CURLE_OPERATION_TIMEDOUT)
        {
            setErrorCode(pErrorCode, DEVICE_RETRIEVE_TIMEOUT);
        }

        CSFLogError(logTag, "curl error %d : \"%s\"", result, curl_easy_strerror(result));
        CSFLogError(logTag, "Request failed : \"%s\"", url.c_str());
    }
    else
    {
    	CSFLogErrorS(logTag, "curl_easy_init returned null !");
    }

    return false;
}

static int curlCallback(char* data, size_t size, size_t nmemb, std::string* buffer)
{
    // What we will return
    int result = 0;

    // Is there anything in the buffer?
    if (buffer != NULL)
    {
        // Append the data to the buffer
        buffer->append(data, nmemb );//pass number of characters (not bytes).

        // Need to return number of bytes, not characters.
        result = (int) (size * nmemb);
    }

    return result;
}

//IMPORTANT: The data pointed to by the char * passed to this function (msg) IS NOT NULL-terminated, it's exactly the size indicated by msgSize. 
//See documentation on CURLOPT_DEBUGFUNCTION (http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTDEBUGFUNCTION)
static int curlDebugCallback(CURL* curl, curl_infotype intoType, char* msg, size_t msgSize)
{
    //Important to use the std::string ctor that takes the "size" second paramater, as msg is not NULL-terminated.
    std::string copyStr(msg, msgSize);

    CSFLogDebugS(logTag, copyStr);

    return 0;
}
