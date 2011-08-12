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

#include "ConfigRetriever.h"

#include "TftpHelper.h"
#include "PhoneConfig.h"
#include "base/threading/platform_thread.h"

#ifdef ECC_USE_HANDYIRON
extern "C" {
#include "secure_api.h"
}
#endif

#include "CSFLog.h"
static const char* logTag = "ConfigRetriever";

using namespace std;
using namespace CSFUnified;

namespace CSF
{

ConfigRetriever::ConfigRetriever():
	multiClusterMode(false)
{

}

ConfigRetriever::~ConfigRetriever()
{

}


void ConfigRetriever::setTFTPServers(const std::vector<std::string> &servers)
{
    tftpServers.assign(servers.begin(), servers.end());
}

void ConfigRetriever::setMultiClusterMode(bool allowMultipleClusters)
{
    multiClusterMode = allowMultipleClusters;
}

void ConfigRetriever::setAuthenticationString(const std::string authString)
{
	this->authString = authString;
}

void ConfigRetriever::setSecureCachePath(const std::string& secureCachePath)
{
	this->secureCachePath = secureCachePath;
}

std::string ConfigRetriever::getLastTFTPServerUsed()
{
	return lastTftpServer;
}

DeviceRetrievalFailureCodeType::DeviceRetrievalFailureCode ConfigRetriever::retrieveConfig(
		const std::string& preferredDeviceName,
		/* out */ std::string& deviceConfigFileContents)
{
    CSFLogInfoS(logTag, "retrieveConfig(" << preferredDeviceName << ")");
    if(tftpServers.empty())
    {
        CSFLogErrorS(logTag, "retrieveConfig() failed - no servers configured!");
        return DeviceRetrievalFailureCodeType::eNoServersConfigured;
    }

    if(preferredDeviceName == "")
    {
        CSFLogErrorS(logTag, "retrieveConfig() failed - no device name given!");
        return DeviceRetrievalFailureCodeType::eNoDeviceNameConfigured;
    }

#ifdef ECC_USE_HANDYIRON
	return secureRetrieveConfig(preferredDeviceName, deviceConfigFileContents);
#else
	return insecureRetrieveConfig(preferredDeviceName, deviceConfigFileContents);
#endif
}

DeviceRetrievalFailureCodeType::DeviceRetrievalFailureCode ConfigRetriever::insecureRetrieveConfig(
		const std::string& preferredDeviceName,
		/* out */ std::string& deviceConfigFileContents)
{
    TftpHelper tftpClient;
    for(vector<string>::iterator it = tftpServers.begin(); it != tftpServers.end(); it++)
    {
        string server = *it;
        string tftpURL = "tftp://" + server + "/" + preferredDeviceName + ".cnf.xml";

        string config;
        if (tftpClient.retrieveFile(tftpURL, config))
        {
            CSFLogDebugS(logTag, "insecureRetrieveConfig() retrieved file " << tftpURL);
            lastTftpServer = server;

            CSFLogDebug(logTag, "%s", config.c_str());

            deviceConfigFileContents = config;
            return DeviceRetrievalFailureCodeType::eNoError;
        }
        else
        {
            if(!multiClusterMode)
            {
                CSFLogWarnS(logTag, "insecureRetrieveConfig() failed to retrieve file " << tftpURL << ", try next server");
            }
            else
            {
                CSFLogDebugS(logTag, "insecureRetrieveConfig() failed to retrieve file " << tftpURL << ", try next server");
            }
        }
    }

    CSFLogErrorS(logTag, "insecureRetrieveConfig() could not obtain config for " << preferredDeviceName);
    return DeviceRetrievalFailureCodeType::eCouldNotConnect;
}

DeviceRetrievalFailureCodeType::DeviceRetrievalFailureCode ConfigRetriever::secureRetrieveConfig(
		const std::string& preferredDeviceName,
		/* out */ std::string& deviceConfigFileContents)
{
#ifdef ECC_USE_HANDYIRON
    CSFLogInfoS(logTag, "secureRetrieveConfig(" << preferredDeviceName << ")");

    if(secureCachePath == "")
    {
        CSFLogErrorS(logTag, "retrieveConfig() failed - no secure cache path given!");
        return DeviceRetrievalFailureCodeType::eCouldNotConnect;  // No secure cache path
    }

    // Reset this early (a little awkward, but it's useful for now).
    lastTftpServerUsed = "";

    if(sec_start_security_session() != 0)
    {
    	CSFLogError(logTag, "secureRetrieveConfig() could not start security session.");
    	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // Security Lib Failure
    }

    // Update CTL.
    SecTrustListInfo trustList;
    if(sec_get_trust_list_info(&trustList) != TL_SUCCESS)
    {
    	CSFLogError(logTag, "secureRetrieveConfig() could not get trust list.");
    	sec_stop_security_session();
    	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // Could not get local trust list.
    }

    vector<string> secureTftpServers;
    vector<string> insecureTftpServers;
    for(int i=0; i<trustList.CTLEntriesCount; i++)
    {
    	SecTrustListData item;
    	if(sec_get_trust_list_item(SEC_CTL_FILE, i, &item) == TL_SUCCESS)
    	{
    		// Is this a secure TFTP server?  Assume secure for now.
    		if(item.role == SEC_ROLE_TFTP || item.role == SEC_ROLE_CUCM_TFTP)
    		{
    			secureTftpServers.push_back(item.subjectName);
    		}
    	}
    }

    vector<string> serversToUse;
    if(secureTftpServers.empty())
    {
    	if(insecureServers.empty())
    	{
			CSFLogWarn(logTag, "secureRetrieveConfig() No TFTP servers in local CTL, trying user-supplied servers.");
			serversToUse.assign(tftpServers.begin(), tftpServers.end());
    	}
    	else
    	{
			CSFLogWarn(logTag, "secureRetrieveConfig() No secure TFTP servers known, trying insecure servers.");
			serversToUse.assign(insecureTftpServers.begin(), insecureTftpServers.end());
    	}
    }
    else
    {
		CSFLogInfo(logTag, "secureRetrieveConfig() using secure TFTP servers.");
    	serversToUse.assign(secureTftpServers.begin(), secureTftpServers.end());
    }

    TftpClient tftpClient;

    // Fetch the latest CTL.
    for(vector<string>::iterator it = serversToUse.begin(); it != serversToUse.end(); it++)
    {
    	string server = *it;
        string tftpURL = "tftp://" + server + "/CTL" + preferredDeviceName + ".tlv";
        string ctlPath = secureCachePath + "CTL.tlv";
        string ctlContents;
        if (tftpClient.retrieveFile(tftpURL, ctlContents))
        {
        	if(!tftpClient.writeLocalFile(ctlPath, ctlContents)
        			|| sec_add_trust_file(ctlPath, SEC_CTL_FILE) != TL_SUCCESS)
        	{
    			CSFLogError(logTag, "secureRetrieveConfig() Could not add CTL file to trust store!");
    	    	sec_stop_security_session();
    	    	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // Could add CTL to trust store.
        	}
        	lastTftpServerUsed = server;
        	break;
        }
    }

    // Now fetch the initial device config (in secure mode, this will usually be a short config).
    // Use the same TFTP server as we had for the TLV.
    if(lastTftpServerUsed == "")
    {
		CSFLogWarn(logTag, "secureRetrieveConfig() Could not update CTL, assuming we don't have one and doing insecure retrieval.");
    	sec_stop_security_session();
    	return insecureRetrieveConfig(preferredDeviceName, deviceConfigFileContents);
    }

	string shortConfig;
	PhoneConfig parsedShortConfig;
    {
        string tftpURL = "tftp://" + lastTftpServerUsed + "/" + preferredDeviceName + ".cnf.xml";
        if (!tftpClient.retrieveFile(tftpURL, shortConfig))
        {
			CSFLogError(logTag, "secureRetrieveConfig() Could not obtain short config!");
	    	sec_stop_security_session();
	    	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // Could not fetch TFTP config.
        }
    }
    if(!parsedShortConfig.parse(shortConfig))
    {
		CSFLogError(logTag, "secureRetrieveConfig() Could not parse short config!");
    	sec_stop_security_session();
    	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // Could not parse TFTP config.
    }

    int capfMode = CAPF_CLNT_AUTH_IGNORE;
    int capfReason = CAPF_CLNT_NORM_AUTO_SESSION;
    if(parsedShortConfig.getCAPFMode() == 0 && parsedShortConfig.isFullConfig() && !parsedShortConfig.hasEncryptedConfig())
    {
		CSFLogInfo(logTag, "secureRetrieveConfig() No security required for config - use it!");
    	sec_stop_security_session();
    	deviceConfigFileContents = shortConfig;
    	return DeviceRetrievalFailureCodeType::eNoError;
    }
    else if(parsedShortConfig.getCAPFMode() == 0)
    {
    	// Get local certificate hash.
    	string localCertificateHash = getLocalCertificateHash();
    	if(localCertificateHash != parsedShortConfig.getCertificateHash())
    	{
    		capfReason = CAPF_CLNT_PUBKEY_MISMATCH;
    		capfMode = authReason.empty() ? CAPF_CLNT_NULL_STR_AUTH : CAPF_CLNT_STR_AUTH;
    	}

    	if(parsedShortConfig.hasEncryptedConfig())
    	{
    		string encryptedFullConfig;
            string tftpURL = "tftp://" + lastTftpServerUsed + "/" + preferredDeviceName + ".cnf.xml.enc.sgn";
    		if(!tftpHelper.retrieveFile(tftpURL, encryptedFullConfig))
    		{
    			CSFLogError(logTag, "secureRetrieveConfig() Could not fetch full, encrypted config!");
    	    	sec_stop_security_session();
    	    	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // Could not fetch full TFTP config.
    		}
    		else
    		{
    			char* decryptedFullConfig = new char[encryptedFullConfig.length()+1];
    			strcpy(decryptedFullConfig, encryptedFullConfig.c_str());
    			string fileName = preferredDeviceName + ".cnf.xml.enc.sgn";
    			int length = encryptedFullConfig.length()+1;
    			int result = sec_validate_config_buffer(decryptedFullConfig, &length, fileName.c_str());
    			if(result != 0)
    			{
    				capfReason = CAPF_CLNT_DECRYPT_FILE_FAILED;
    	    		capfMode = authReason.empty() ? CAPF_CLNT_NULL_STR_AUTH : CAPF_CLNT_STR_AUTH;
    			}
    			else
    			{
    				// A bit premature, but here goes.
    				deviceConfigFileContents = string(decryptedFullConfig, length);
    			}
    			delete[] decryptedFullConfig;
    		}
    	}
    }
    else if(parsedShortConfig.getCAPFMode() == 1 && authString.empty())
    {
		CSFLogError(logTag, "secureRetrieveConfig() Authentication string required but not supplied!");
    	sec_stop_security_session();
    	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // No Auth String
    }
    else if(parsedShortConfig.getCAPFMode() == 1 && !authString.empty())
    {
    	capfReason = CAPF_CLNT_MANUAL_SESSION;
		capfMode = CAPF_CLNT_STR_AUTH;
    }
    else if(parsedShortConfig.getCAPFMode() == 2)
    {
    	capfReason = CAPF_CLNT_MANUAL_SESSION;
		capfMode = CAPF_CLNT_NULL_STR_AUTH;
    }

    // Now, the actual CAPF part.
    vector<string> capfServers;
    for(int i=0; i<trustList.CTLEntriesCount; i++)
    {
    	SecTrustListData item;
    	if(sec_get_trust_list_item(SEC_CTL_FILE, i, &item) == TL_SUCCESS)
    	{
    		// Is this a secure server?  Assume secure for now.
    		if(item.role == SEC_ROLE_CAPF)
    		{
    			capfServers.push_back(item.subjectName);
    		}
    	}
    }

    for(vector<string>::iterator it = capfServers.begin(); it != capfServers.end(); it++)
    {
    	string server = *it;
    	if(sec_initiate_capf_process(server, 3804, capfMode, capfReason, authString.c_str(), preferredDeviceName) == 0)
    	{
    		CSFLogError(logTag, "secureRetrieveConfig() Could not initiate CAPF process!");
        	sec_stop_security_session();
        	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // Couldn't start CAPF process
    	}

    	// Now poll for a result.
    	CapfStatus status;
    	while(sec_get_capf_status(&status) == 1)
    	{
    		base::PlatformThread::Sleep(500);
    	}

    	if(status == CAPF_SUCCESS)
    	{
    		CSFLogInfo(logTag, "secureRetrieveConfig() CAPF success!");
        	sec_stop_security_session();
        	return DeviceRetrievalFailureCodeType::eNoError;
    	}
    	else if(status == CAPF_LSC_UPDATED)
    	{
    		CSFLogInfo(logTag, "secureRetrieveConfig() CAPF certificate updated - retry");
        	sec_stop_security_session();
        	return secureRetrieveConfig(preferredDeviceName, deviceConfigFileContents);
    	}
    	else if(status == CAPF_CONNECT_FAILED || status == CAPF_CONN_LOST || status == CAPF_TIMEOUT)
    	{
    		CSFLogWarn(logTag, "secureRetrieveConfig() could not connect to CAPF server, try next");
    		continue;
    	}
    	else
    	{
    		CSFLogInfo(logTag, "secureRetrieveConfig() CAPF failure");
        	sec_stop_security_session();
        	return DeviceRetrievalFailureCodeType::eCouldNotConnect; // CAPF failure
    	}
    }
#endif
    return DeviceRetrievalFailureCodeType::eCouldNotConnect;
}

std::string ConfigRetriever::getLocalCertificateHash()
{
	std::string result;
#ifdef ECC_USE_HANDYIRON
    SecFile file;
    SecCertificate cert;
	SecCertInfo certInfo;

    secLoadFile((char *) "myCertFile", &file);
    if (file.data) {
       cert.data = (uint8_t*)file.data;
       cert.len  = file.len;
       secGetCertificateInfo(&cert, &certInfo);

       secFreeFile(&file);
    }

    if(certInfo.hashLen > 20)
	{
		CSFLogError(logTag, "getCertHash(): hash is > 160 bits!");
		return "";
	}

	// Convert the binary representation to hex.
	for(int i=0; i<certInfo.hashLen; i++)
	{
		int nibble = (certInfo.hash[i] & 0xF0) >> 4;
		char digit = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
		result += digit;

		nibble = (certInfo.hash[i] & 0x0F);
		digit = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
		result += digit;
	}
	CSFLogInfoS(logTag, "getCertHash(): success: " << result);
#endif
	return result;
}

}
