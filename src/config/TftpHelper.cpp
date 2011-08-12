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
#include "TftpHelper.h"
#include "curl/curl.h"

#include <fstream>

using namespace std;
using namespace CSFUnified;

// static initialization
#include "CSFLog.h"
static const char* logTag = "TftpHelper";

static std::string curlBuffer;
static char curlErrorBuffer[CURL_ERROR_SIZE];

#define TFTP_CONNECTION_TIMEOUT 10 //seconds

static int curlDebugCallback(CURL* curl, curl_infotype intoType, char* msg, size_t msgSize);


TftpHelper::TftpHelper() {
}

TftpHelper::~TftpHelper() {
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

static int curlDebugCallback(CURL* curl, curl_infotype intoType, char* msg, size_t msgSize)
{
    //Important to use the string ctor that takes the "size" second paramater, as msg is not NULL-terminated.
    string copyStr(msg, msgSize);

    CSFLogDebugS(logTag, copyStr.c_str());

    return 0;
}

bool TftpHelper::retrieveFile(const string & url, string & contents)
{
	CSFLogDebug(logTag, "Retrieving file at %s", url.c_str());
	bool downloadOk = false;

	// Use the CURL easy interface
	CURL *curl;
	CURLcode result = CURLE_FAILED_INIT;

	// Create our curl handle
	curl = curl_easy_init();

	// New xml data will be appended to this string, so ensure it is empty before we start
	// ( it might contain data left over from the last use of this function ).
	curlBuffer.erase();

	if (curl)
	{
		// Now set up all of the curl options
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlErrorBuffer);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TFTP_CONNECTION_TIMEOUT);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlBuffer);

		curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 1);

		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curlDebugCallback);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

		// send the request
		result = curl_easy_perform(curl);

		// always cleanup
		curl_easy_cleanup(curl);

		if (result == CURLE_OK)
		{
			contents = curlBuffer;
			downloadOk = true;
		}
		else
		{
			CSFLogError(logTag, "cannot get config file %d %s", result, curl_easy_strerror(result));
		}
	}
	else
	{
		CSFLogErrorS(logTag, "curl_easy_init returned null !");
	}

	return downloadOk;
}

bool TftpHelper::retrieveFile(const string & url, const string & filePath)
{
	string fileContents;

	if(retrieveFile(url, fileContents))
	{
		return writeLocalFile(filePath, fileContents);
	}

	return false;
}

bool TftpHelper::writeLocalFile(const std::string& filePath, const std::string& contents)
{
	fstream file(filePath.c_str(), ios::out | ios::binary);
	file << contents;

	if (ios::failbit || ios::badbit)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool TftpHelper::readLocalFile(const std::string& filePath, std::string& contents)
{
	fstream file(filePath.c_str(), ios::in | ios::binary);
	file >> contents;

	if (ios::failbit || ios::badbit)
	{
		return false;
	}
	else
	{
		return true;
	}
}
