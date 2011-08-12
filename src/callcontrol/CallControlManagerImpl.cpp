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

#include "CC_SIPCCDevice.h"
#include "CC_SIPCCDeviceInfo.h"
#include "CC_SIPCCFeatureInfo.h"
#include "CC_SIPCCLine.h"
#include "CC_SIPCCLineInfo.h"
#include "CC_SIPCCCallInfo.h"
#include "CallControlManagerImpl.h"

#include "CCMCIPClient.h"
#include "ConfigRetriever.h"


#include "CSFLog.h"
#include "csf_common.h"

static const char* logTag = "CallControlManager";

static std::string logDestination = "CallControl.log";

using namespace std;
using namespace CSFUnified;

static string toString(const vector<string>& strings)
{
    string result;
    for(vector<string>::const_iterator it = strings.begin(); it != strings.end(); it++)
    {
        if(!result.empty())
            result != ",";
        result += *it;
    }
    return result;
}

namespace CSF
{

CallControlManagerImpl::CallControlManagerImpl()
: certificateLevel(AuthenticationCertificateLevelType::eAllCerts),
  multiClusterMode(false),
  sipccLoggingMask(0),
  authenticationStatus(AuthenticationStatusEnum::eNotAuthenticated),
  connectionState(ConnectionStatusEnum::eIdle)
{
    CSFLogInfoS(logTag, "CallControlManagerImpl()");
}

CallControlManagerImpl::~CallControlManagerImpl()
{
    CSFLogInfoS(logTag, "~CallControlManagerImpl()");
    destroy();
}

bool CallControlManagerImpl::destroy()
{
    CSFLogInfoS(logTag, "destroy()");
    bool retval = disconnect();
    if(retval == false)
	{
		return retval;
	}
	return retval;
}

// Observers
void CallControlManagerImpl::addCCObserver ( CC_Observer * observer )
{
	base::AutoLock lock(m_lock);
    if (observer == NULL)
    {
        CSFLogErrorS(logTag, "NULL value for \"observer\" passed to addCCObserver().");
        return;
    }

    ccObservers.insert(observer);
}

void CallControlManagerImpl::removeCCObserver ( CC_Observer * observer )
{
	base::AutoLock lock(m_lock);
    ccObservers.erase(observer);
}

void CallControlManagerImpl::addECCObserver ( ECC_Observer * observer )
{
	base::AutoLock lock(m_lock);
    if (observer == NULL)
    {
        CSFLogErrorS(logTag, "NULL value for \"observer\" passed to addECCObserver().");
        return;
    }

    eccObservers.insert(observer);
}

void CallControlManagerImpl::removeECCObserver ( ECC_Observer * observer )
{
	base::AutoLock lock(m_lock);
    eccObservers.erase(observer);
}


// Config and global setup
void CallControlManagerImpl::setAuthenticationCredentials(const std::string &username, const std::string& password)
{
    CSFLogInfoS(logTag, "setAuthenticationCredentials()");
    this->username = username;
    this->password = password;
}

void CallControlManagerImpl::setAuthenticationPolicy(const AuthenticationCertificateLevelType::AuthenticationCertificateLevel& level)
{
    CSFLogInfoS(logTag, "setAuthenticationPolicy(" << level << ")");
    certificateLevel = level;
}

void CallControlManagerImpl::setCCMCIPServers(const std::vector<std::string> &servers)
{
    CSFLogInfoS(logTag, "setCCMCIPServers(" << toString(servers) << ")");
    ccmcipServers.assign(servers.begin(), servers.end());
}

void CallControlManagerImpl::setCCMCIPServers(const std::string &server)
{
    CSFLogInfoS(logTag, "setCCMCIPServers(" << server << ")");
    ccmcipServers.clear();
    ccmcipServers.push_back(server);
}

void CallControlManagerImpl::setTFTPServers(const std::vector<std::string> &servers)
{
    CSFLogInfoS(logTag, "setTFTPServers(" << toString(servers) << ")");
    tftpServers.assign(servers.begin(), servers.end());
}

void CallControlManagerImpl::setTFTPServers(const std::string &server)
{
    CSFLogInfoS(logTag, "setTFTPServers(" << server << ")");
    tftpServers.clear();
    tftpServers.push_back(server);
}

void CallControlManagerImpl::setMultiClusterMode(bool allowMultipleClusters)
{
    CSFLogInfoS(logTag, "setMultiClusterMode(" << allowMultipleClusters << ")");
    multiClusterMode = allowMultipleClusters;
}

void CallControlManagerImpl::setSIPCCLoggingMask(const cc_int32_t mask)
{
    CSFLogInfoS(logTag, "setSIPCCLoggingMask(" << mask << ")");
    sipccLoggingMask = mask;
}

void CallControlManagerImpl::setAuthenticationString(const std::string &authString)
{
    CSFLogInfoS(logTag, "setAuthenticationString()");
    this->authString = authString;
}

void CallControlManagerImpl::setSecureCachePath(const std::string &secureCachePath)
{
    CSFLogInfoS(logTag, "setSecureCachePath(" << secureCachePath << ")");
    this->secureCachePath = secureCachePath;
}

// Local IP Address
void CallControlManagerImpl::setLocalIpAddressAndGateway(const std::string& localIpAddress, const std::string& defaultGW)
{
    CSFLogInfoS(logTag, "setLocalIpAddressAndGateway(" << localIpAddress << ", " << defaultGW << ")");
    this->localIpAddress = localIpAddress;
    this->defaultGW = defaultGW;

    if(softPhone != NULL)
    {
        softPhone->setLocalAddressAndGateway(this->localIpAddress, this->defaultGW);
    }
}

// CCMCIP
AuthenticationFailureCodeType::AuthenticationFailureCode CallControlManagerImpl::authenticate()
{
    CSFLogInfoS(logTag, "authenticate()");
    setConnectionState(ConnectionStatusEnum::eRegistering);

    if(ccmcipServers.empty())
    {
        CSFLogErrorS(logTag, "authenticate() failed - no servers configured!");
        setConnectionState(ConnectionStatusEnum::eFailed);

        return AuthenticationFailureCodeType::eNoServersConfigured;
    }

    if(username.empty())
    {
        CSFLogErrorS(logTag, "authenticate() failed - no credentials configured!");
        setConnectionState(ConnectionStatusEnum::eFailed);
        return AuthenticationFailureCodeType::eNoCredentialsConfigured;
    }

    authenticationStatus = AuthenticationStatusEnum::eInProgress;
    notifyAuthenticationStatusChange(authenticationStatus);

    CCMCIPClient ccmcipClientInitializer;//Construct this here to keep XML and libCURL init'd across retries.

    bool authenticated = false;
    map<string, CSFUnified::DeviceInfo> localDeviceInfoMap;
    AuthenticationFailureCodeType::AuthenticationFailureCode result = AuthenticationFailureCodeType::eNoServersConfigured;
    for(vector<string>::iterator it = ccmcipServers.begin(); it != ccmcipServers.end(); it++)
    {
        string server = *it;

        CSFUnified::DeviceRetrieveCertLevel internalCertLevel = (CSFUnified::DeviceRetrieveCertLevel) certificateLevel;

        localDeviceInfoMap.clear();
        int errorCode = 0;
        authenticated = CCMCIPClient::fetchDevices(server, username, password, internalCertLevel, localDeviceInfoMap, &errorCode);

        if(authenticated)
        {
            result = AuthenticationFailureCodeType::eNoError;
        }
        else
        {
            switch(errorCode)
            {
            case DEVICE_RETRIEVE_TIMEOUT:
                result = AuthenticationFailureCodeType::eCouldNotConnect;
                break;
            case DEVICE_RETRIEVE_AUTH_FAILED:
                result = AuthenticationFailureCodeType::eCredentialsRejected;
                break;
            case DEVICE_RETRIEVE_EMPTY_STRING:
                result = AuthenticationFailureCodeType::eResponseEmpty;
                break;
            case DEVICE_RETRIEVE_XML_PARSE_DOC_FAILED:
                result = AuthenticationFailureCodeType::eResponseInvalid;
                break;
            default:
                result = AuthenticationFailureCodeType::eCouldNotConnect;
            }
        }

        // Exit loop on first success.
        if(authenticated)
        {
            CSFLogDebugS(logTag, "authenticate() succeeded with " << server);
            lastCCMCIPServer = server;
            break;
        }

        // If not in multi-cluster mode, also exit loop on authentication failure (but not on failure to connect etc)
        if(result == AuthenticationFailureCodeType::eCredentialsRejected)
        {
        	setConnectionState(ConnectionStatusEnum::eFailed);

            if(!multiClusterMode)
            {
                CSFLogErrorS(logTag, "authenticate() credentials rejected by " << server << ", abandoning");
                break;
            }
            else
            {
                CSFLogDebugS(logTag, "authenticate() credentials rejected by " << server << ", try next server anyway");
            }
        }
    }

    if(authenticated)
    {
        for(map<string, DeviceInfo>::const_iterator it = localDeviceInfoMap.begin(); it != localDeviceInfoMap.end(); it++)
        {
            const DeviceInfo & deviceInfo = it->second;

            PhoneDetailsImplPtr details;
            bool added = false;
            PhoneDetailsMap::iterator it2 = phoneDetailsMap.find(deviceInfo.getName());
            if(it2 != phoneDetailsMap.end())
            {
                CSFLogDebugS(logTag, "authenticate() updated PhoneDetails " << deviceInfo.getName());
                details = it2->second;
            }
            else
            {
                CSFLogDebugS(logTag, "authenticate() added PhoneDetails " << deviceInfo.getName());
                details = PhoneDetailsImplPtr(new PhoneDetailsImpl());
                details->setName(deviceInfo.getName());
                phoneDetailsMap[deviceInfo.getName()] = details;
                added = true;
            }
            details->setModelDescription(deviceInfo.getModel());
            details->setDescription(deviceInfo.getDescription());

            if(added)
                notifyAvailablePhoneEvent(AvailablePhoneEventType::eFound, details);
            else
                notifyAvailablePhoneEvent(AvailablePhoneEventType::eUpdated, details);
        }
        authenticationStatus = AuthenticationStatusEnum::eAuthenticated;
        notifyAuthenticationStatusChange(authenticationStatus);
    }
    else
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);

        CSFLogErrorS(logTag, "authenticate() failed [" << result << "]");
        authenticationStatus = AuthenticationStatusEnum::eFailed;
        notifyAuthenticationStatusChange(authenticationStatus);
    }

    return result;
}

AuthenticationStatusEnum::AuthenticationStatus CallControlManagerImpl::getAuthenticationStatus()
{
    return authenticationStatus;
}

std::string CallControlManagerImpl::getLastCCMCIPServerUsed()
{
    return lastCCMCIPServer;
}

// TFTP
DeviceRetrievalFailureCodeType::DeviceRetrievalFailureCode CallControlManagerImpl::fetchDeviceConfig(const std::string& preferredDeviceName)
{
    CSFLogInfoS(logTag, "fetchDeviceConfig(" << preferredDeviceName << ")");
    if(tftpServers.empty())
    {
        CSFLogErrorS(logTag, "fetchDeviceConfig() failed - no servers configured!");
        return DeviceRetrievalFailureCodeType::eNoServersConfigured;
    }

    if(preferredDeviceName == "")
    {
        CSFLogErrorS(logTag, "fetchDeviceConfig() failed - no device name given!");
        return DeviceRetrievalFailureCodeType::eNoDeviceNameConfigured;
    }

    ConfigRetriever retriever;
    retriever.setTFTPServers(tftpServers);
    retriever.setAuthenticationString(authString);
    retriever.setSecureCachePath(secureCachePath);
    retriever.setMultiClusterMode(multiClusterMode);

	string config;
	DeviceRetrievalFailureCodeType::DeviceRetrievalFailureCode result = retriever.retrieveConfig(preferredDeviceName, config);
	if (result == DeviceRetrievalFailureCodeType::eNoError)
	{
		CSFLogDebugS(logTag, "fetchDeviceConfig() retrieved config for " << preferredDeviceName);
		lastTFTPServer = retriever.getLastTFTPServerUsed();

		CSFLogDebug(logTag, "%s", config.c_str());

		PhoneDetailsMap::iterator it = phoneDetailsMap.find(preferredDeviceName);
		if(it != phoneDetailsMap.end())
		{
			CSFLogDebugS(logTag, "fetchDeviceConfig() added PhoneDetails " << preferredDeviceName);
			PhoneDetailsImplPtr details = it->second;
			details->setConfigStatus(DeviceConfigStatusEnum::eFetchedConfig);
			details->setConfig(config);
			notifyAvailablePhoneEvent(AvailablePhoneEventType::eUpdated, details);
		}
		else
		{
			CSFLogDebugS(logTag, "fetchDeviceConfig() added PhoneDetails " << preferredDeviceName);
			PhoneDetailsImplPtr details(new PhoneDetailsImpl());
			details->setName(preferredDeviceName);
			details->setConfigStatus(DeviceConfigStatusEnum::eFetchedConfig);
			details->setConfig(config);
			phoneDetailsMap[preferredDeviceName] = details;
			notifyAvailablePhoneEvent(AvailablePhoneEventType::eFound, details);
		}

		return DeviceRetrievalFailureCodeType::eNoError;
	}
	else
	{
		CSFLogErrorS(logTag, "fetchDeviceConfig() could not obtain config for " << preferredDeviceName);
		return result;
    }
}

bool CallControlManagerImpl::setDeviceConfig(const std::string& preferredDeviceName, const std::string& deviceConfigFileContents)
{
    CSFLogInfoS(logTag, "setDeviceConfig(" << preferredDeviceName << ", " <<
            (deviceConfigFileContents.empty() ? "(null)" : "(...)") << ")");
    bool isEmpty = (deviceConfigFileContents == "");
    PhoneDetailsMap::iterator it = phoneDetailsMap.find(preferredDeviceName);
    if(it != phoneDetailsMap.end())
    {
        CSFLogDebugS(logTag, "setDeviceConfig() updated PhoneDetails " << preferredDeviceName);
        PhoneDetailsImplPtr details = it->second;
        if(!isEmpty)
        {
            details->setConfigStatus(DeviceConfigStatusEnum::eCachedConfig);
            details->setConfig(deviceConfigFileContents);
        }
        else
        {
            details->setConfigStatus(DeviceConfigStatusEnum::eNoConfig);
            details->setConfig("");
        }
        notifyAvailablePhoneEvent(AvailablePhoneEventType::eUpdated, details);
    }
    else
    {
        CSFLogDebugS(logTag, "setDeviceConfig() added PhoneDetails " << preferredDeviceName);
        PhoneDetailsImplPtr details(new PhoneDetailsImpl());
        details->setName(preferredDeviceName);
        if(!isEmpty)
        {
            details->setConfigStatus(DeviceConfigStatusEnum::eCachedConfig);
            details->setConfig(deviceConfigFileContents);
        }
        else
        {
            details->setConfigStatus(DeviceConfigStatusEnum::eNoConfig);
            details->setConfig("");
        }
        phoneDetailsMap[preferredDeviceName] = details;
        notifyAvailablePhoneEvent(AvailablePhoneEventType::eFound, details);
    }
    return true;
}

std::string CallControlManagerImpl::getLastTFTPServerUsed()
{
    return lastTFTPServer;
}

// SIP and CTI stacks
bool CallControlManagerImpl::connect(const std::string& preferredDeviceName, const std::string& preferredLineDN)
{
	setConnectionState(ConnectionStatusEnum::eRegistering);

    CSFLogInfoS(logTag, "connect(" << preferredDeviceName << ", " << preferredLineDN << ")");
    if(phone != NULL)
    {
    	setConnectionState(ConnectionStatusEnum::eReady);

        CSFLogErrorS(logTag, "connect() failed - already connected!");
        return false;
    }

    // Check preconditions.
    if(localIpAddress.empty() || localIpAddress == "127.0.0.1")
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);
        CSFLogErrorS(logTag, "connect() failed - No local IP address set!");
    	return false;
    }

    // First determine the device to use.  Autoselect if none is provided.
    string selectedDeviceName;
    if(preferredDeviceName == "")
    {
        // If no device is indicated and we have not get tried to authenticate, and CCMCIP is configured,
        // give CMCCIP a whirl, with luck we'll discover a suitable device.
        if(authenticationStatus == AuthenticationStatusEnum::eNotAuthenticated && !ccmcipServers.empty()
                    && !username.empty())
        {
            if(AuthenticationFailureCodeType::eNoError != authenticate())
            {
                setConnectionState(ConnectionStatusEnum::eFailed);
                CSFLogErrorS(logTag, "connect() failed - Could not authenticate!");
                return false;
            }
        }

        // Pick the first likely looking device from our PhoneDetails map.
        for(PhoneDetailsMap::iterator it = phoneDetailsMap.begin(); it != phoneDetailsMap.end(); it++)
        {
            PhoneDetailsPtr details = it->second;

            if(details->isSoftPhone() || authenticationStatus != AuthenticationStatusEnum::eAuthenticated)
            {
                selectedDeviceName = details->getName();
                break;
            }
        }
    }
    else
    {
        selectedDeviceName = preferredDeviceName;
    }

    // We must select a device.
    if(selectedDeviceName == "")
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);
        CSFLogErrorS(logTag, "connect() failed - Could not select a device!");
        return false;
    }
    CSFLogInfoS(logTag, "selected device '" << selectedDeviceName << "'");

    // Not valid to give a preference for this.
    if(preferredLineDN != "")
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);
        CSFLogErrorS(logTag, "connect() failed - PreferredLineDN must be blank!");
        return false;
    }

    PhoneDetailsPtr details;
    {
        PhoneDetailsMap::iterator it = phoneDetailsMap.find(selectedDeviceName);
        if(it != phoneDetailsMap.end())
        {
            details = it->second;
        }

        // If we don't have a config handy, try to fetch one.
        if(details == NULL || details->getConfigStatus() == DeviceConfigStatusEnum::eNoConfig)
        {
            if(DeviceRetrievalFailureCodeType::eNoError != fetchDeviceConfig(selectedDeviceName))
            {
             	setConnectionState(ConnectionStatusEnum::eFailed);

                CSFLogErrorS(logTag, "connect() failed - failed to fetch device config for " << selectedDeviceName << "!");
                return false;
            }
            it = phoneDetailsMap.find(selectedDeviceName);
            if(it != phoneDetailsMap.end())
            {
                details = it->second;
            }
        }
    }

    // We do have to obtain a PhoneDetails record containing a config.
    if(details == NULL || details->getConfigStatus() == DeviceConfigStatusEnum::eNoConfig)
    {
     	setConnectionState(ConnectionStatusEnum::eFailed);
        CSFLogErrorS(logTag, "connect() failed - could not obtain a device config!");
        return false;
    }

    this->preferredDevice = selectedDeviceName;
    this->preferredLineDN = preferredLineDN;
    softPhone = CC_SIPCCServicePtr(new CC_SIPCCService());
    phone = softPhone;
    phone->init("","","","");
    softPhone->setConfig(details->getConfig());
    softPhone->setDeviceName(details->getName());
    softPhone->setLoggingMask(sipccLoggingMask);
    softPhone->setLocalAddressAndGateway(localIpAddress, defaultGW);
    phone->addCCObserver(this);

    bool bStarted = phone->start();
    if (!bStarted) {
        setConnectionState(ConnectionStatusEnum::eFailed);
    } else {
       setConnectionState(ConnectionStatusEnum::eReady);
    }

    return bStarted;
}

bool CallControlManagerImpl::registerUser( const std::string& deviceName, const std::string& user, const std::string& domain, const std::string& sipContact )
{
	setConnectionState(ConnectionStatusEnum::eRegistering);

    CSFLogInfoS(logTag, "registerUser(" << user << ", " << domain << ", " << sipContact << " )");
    if(phone != NULL)
    {
    	setConnectionState(ConnectionStatusEnum::eReady);

        CSFLogErrorS(logTag, "registerUser() failed - already connected!");
        return false;
    }

    // Check preconditions.
    if(localIpAddress.empty() || localIpAddress == "127.0.0.1")
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);
    	CSFLogErrorS(logTag, "registerUser() failed - No local IP address set!");
    	return false;
    }

    softPhone = CC_SIPCCServicePtr(new CC_SIPCCService());
    phone = softPhone;
    phone->init(user, domain, deviceName, sipContact);
    softPhone->setLoggingMask(sipccLoggingMask);
    softPhone->setLocalAddressAndGateway(localIpAddress, defaultGW);
    phone->addCCObserver(this);

    bool bStarted = phone->startService();
    if (!bStarted) {
        setConnectionState(ConnectionStatusEnum::eFailed);
    } else {
        setConnectionState(ConnectionStatusEnum::eReady);
    }

    return bStarted;
}

bool CallControlManagerImpl::disconnect()
{
    CSFLogInfoS(logTag, "disconnect()");
    if(phone == NULL)
        return true;

    connectionState = ConnectionStatusEnum::eIdle;
    phone->removeCCObserver(this);
    phone->stop();
    phone->destroy();
    phone.reset();
    softPhone.reset();

    return true;
}

std::string CallControlManagerImpl::getPreferredDeviceName()
{
    return preferredDevice;
}

std::string CallControlManagerImpl::getPreferredLineDN()
{
    return preferredLineDN;
}

ConnectionStatusEnum::ConnectionStatus CallControlManagerImpl::getConnectionStatus()
{
    return connectionState;
}

std::string CallControlManagerImpl::getCurrentServer()
{
    return "";
}

// Currently controlled device
CC_DevicePtr CallControlManagerImpl::getActiveDevice()
{
    if(phone != NULL)
        return phone->getActiveDevice();

    return CC_DevicePtr();
}

// All known devices
PhoneDetailsVtrPtr CallControlManagerImpl::getAvailablePhoneDetails()
{
    PhoneDetailsVtrPtr result = PhoneDetailsVtrPtr(new PhoneDetailsVtr());
    for(PhoneDetailsMap::iterator it = phoneDetailsMap.begin(); it != phoneDetailsMap.end(); it++)
    {
        PhoneDetailsPtr details = it->second;
        result->push_back(details);
    }
    return result;
}
PhoneDetailsPtr CallControlManagerImpl::getAvailablePhoneDetails(const std::string& deviceName)
{
    PhoneDetailsMap::iterator it = phoneDetailsMap.find(deviceName);
    if(it != phoneDetailsMap.end())
    {
        return it->second;
    }
    return PhoneDetailsPtr();
}

// Media setup
VideoControlPtr CallControlManagerImpl::getVideoControl()
{
    if(phone != NULL)
        return phone->getVideoControl();

    return VideoControlPtr();
}

AudioControlPtr CallControlManagerImpl::getAudioControl()
{
    if(phone != NULL)
        return phone->getAudioControl();

    return AudioControlPtr();
}

/*
  There are a number of factors that determine PhoneAvailabilityType::PhoneAvailability. The supported states for this enum are:
  { eUnknown, eAvailable, eUnAvailable, eNotAllowed }. eUnknown is the default value, which is set when there is no information
  available that would otherwise determine the availability value. The factors that can influence PhoneAvailability are:
  phone mode, and for a given device (described by DeviceInfo) the model, and the name of the device. For phone control mode, the
  device registration and whether CUCM says the device is CTI controllable (or not) is a factor.

  For Phone Control mode the state machine is:

     is blacklisted model name? -> Yes -> NOT_ALLOWED
        (see Note1 below)
              ||
              \/
              No
              ||
              \/
       is CTI Controllable?
     (determined from CUCM) -> No -> NOT_ALLOWED
              ||
              \/
              Yes
              ||
              \/
  Can we tell if it's registered? -> No -> ?????? TODO: Seems to depends on other factors (look at suggestedAvailability parameter
              ||                                        in DeviceSubProviderImpl.addOrUpdateDevice() in CSF1G Java code.
              \/
              Yes
              ||
              \/
         is Registered?
     (determined from CUCM) -> No -> NOT_AVAILABLE
              ||
              \/
              Yes
              ||
              \/
           AVAILABLE

  ========

  For Softphone mode the state machine is:

        is device excluded?
   (based on "ExcludedDevices"   -> Yes -> NOT_ALLOWED
         config settings
        (see Note2 below))
              ||
              \/
              No
              ||
              \/
          isSoftphone?



     Note1: model name has to match completely, ie it's not a sub-string match, but we are ignoring case. So, if the blacklist
            contains a string "Cisco Unified Personal Communicator" then the model has to match this completely (but can be a
            different case) to be a match. In CSF1G the blacklist is hard-wired to:
                { "Cisco Unified Personal Communicator",
                  "Cisco Unified Client Services Framework",
                  "Client Services Framework",
                  "Client Services Core" }

     Note2: The "ExcludedDevices" is a comma-separated list of device name prefixes (not model name). Unlike the above, this is
            a sub-string match, but only a "starts with" sub-string match, not anywhere in the string. If the device name
            is a complete match then this is also excluded, ie doesn't have to be a sub-string. For example, if the
            ExcludeDevices list contains { "ECP", "UPC" } then assuming we're in softphone mode, then any device whose
            name starts with the strings ECP or UPC, or whose complete name is either of these will be deemed to be excluded
            and will be marked as NOT_ALLOWED straightaway. In Phone Control mode the "ExcludedDevices" list i not taken into
            account at all in the determination of availability.

     Note3: isSoftphone() function

  The config service provides a list of "blacklisted" device name prefixes, that is, if the name of the device starts with a
  sub-string that matches an entry in the blacklist, then it is straightaway removed from the list? marked as NOT_ALLOWED.
 */

// CC_Observers
void CallControlManagerImpl::onDeviceEvent(ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_DeviceInfoPtr info)
{
    notifyDeviceEventObservers(deviceEvent, devicePtr, info);
}
void CallControlManagerImpl::onFeatureEvent(ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_FeatureInfoPtr info)
{
    notifyFeatureEventObservers(deviceEvent, devicePtr, info);
}
void CallControlManagerImpl::onLineEvent(ccapi_line_event_e lineEvent,     CC_LinePtr linePtr, CC_LineInfoPtr info)
{
    notifyLineEventObservers(lineEvent, linePtr, info);
}
void CallControlManagerImpl::onCallEvent(ccapi_call_event_e callEvent,     CC_CallPtr callPtr, CC_CallInfoPtr info)
{
    notifyCallEventObservers(callEvent, callPtr, info);
}


void CallControlManagerImpl::notifyDeviceEventObservers (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_DeviceInfoPtr info)
{
	base::AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onDeviceEvent(deviceEvent, devicePtr, info);
    }
}

void CallControlManagerImpl::notifyFeatureEventObservers (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_FeatureInfoPtr info)
{
	base::AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onFeatureEvent(deviceEvent, devicePtr, info);
    }
}

void CallControlManagerImpl::notifyLineEventObservers (ccapi_line_event_e lineEvent, CC_LinePtr linePtr, CC_LineInfoPtr info)
{
	base::AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onLineEvent(lineEvent, linePtr, info);
    }
}

void CallControlManagerImpl::notifyCallEventObservers (ccapi_call_event_e callEvent, CC_CallPtr callPtr, CC_CallInfoPtr info)
{
	base::AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onCallEvent(callEvent, callPtr, info);
    }
}

void CallControlManagerImpl::notifyAvailablePhoneEvent (AvailablePhoneEventType::AvailablePhoneEvent event,
        const PhoneDetailsPtr availablePhoneDetails)
{
	base::AutoLock lock(m_lock);
    set<ECC_Observer*>::const_iterator it = eccObservers.begin();
    for ( ; it != eccObservers.end(); it++ )
    {
        (*it)->onAvailablePhoneEvent(event, availablePhoneDetails);
    }
}

void CallControlManagerImpl::notifyAuthenticationStatusChange (AuthenticationStatusEnum::AuthenticationStatus status)
{
	base::AutoLock lock(m_lock);
    set<ECC_Observer*>::const_iterator it = eccObservers.begin();
    for ( ; it != eccObservers.end(); it++ )
    {
        (*it)->onAuthenticationStatusChange(status);
    }
}

void CallControlManagerImpl::notifyConnectionStatusChange(ConnectionStatusEnum::ConnectionStatus status)
{
	base::AutoLock lock(m_lock);
    set<ECC_Observer*>::const_iterator it = eccObservers.begin();
    for ( ; it != eccObservers.end(); it++ )
    {
        (*it)->onConnectionStatusChange(status);
    }
}

void CallControlManagerImpl::setConnectionState(ConnectionStatusEnum::ConnectionStatus status)
{
	connectionState = status;
	notifyConnectionStatusChange(status);
}

}
