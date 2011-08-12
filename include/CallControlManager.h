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

#pragma once

#include "CC_Common.h"

#include "CC_Observer.h"
#include "ECC_Observer.h"
#include "ECC_Types.h"

#include <string>
#include <vector>

/**
 *  @mainpage Enhanced Call Control
 *
 *  @section intro_sec Introduction
 *  This wraps and aggregates the SIPCC and CTI call control stacks, media stacks, and various additional
 *  components and glue necessary to start, configure and run them, and presents a high-level C++ API
 *  for connection, device selection and status, and call control.
 *
 *  @section main_outline Outline
 *  @li The main entry point is CSF::CallControlManager, which is used to configure and start a
 *  	call control stack.
 *  @li Configuration and events are raised to the CSF::ECC_Observer interface.
 *  @li Call Control is performed via CSF::CC_Device, CSF::CC_Line and CSF::CC_Call.
 *  @li Call Control events are raised to the CSF::CC_Observer interface.
 *  @li Audio/Video device selection and global media configuration is performed via CSF::AudioControl
 *      and CSF::VideoControl.  Per-call media operations (mute, volume, etc) are integrated onto
 *      the CSF::CC_Call and CSF::CC_CallInfo interfaces.
 */

namespace CSF
{
	DECLARE_PTR(CallControlManager);
	/**
	 * CallControlManager
	 *
	 * The class is partitioned into several blocks of functionality:
	 * - Create/Destroy - Initialisation and clean shutdown.
	 * 					  Destroy is optional if the destructor is used properly.
	 * - Observer - Register for events when any state changes.  Optional but strongly advised.
	 * - Config - Set* methods, apply initial setup, preferences and settings.
	 * 			  The username, servers, etc are set here.
	 * 			  Which settings are mandatory will depend on the intended usage.
	 * 			  MultiClusterMode defaults to false, and is only applicable to CWC, making ECC
	 * 			  keep trying the next servers in the list after authentication failures.
	 * 			  AuthenticationPolicy defaults to AuthenticationCertificateLevelType::eSignedCert.
	 * - Authenticate - this relates to using CCMCIP to validate the CUCM username and password,
	 *                  basic connectivity to CUCM, and obtaining the list of available phones.
	 *                  This step is optional.
	 * - DeviceConfig - This is an additional step for softphone mode, to obtain the device config
	 *                  file via TFTP from CUCM.  A client may cache the result or do its own
	 *                  TFTP implementation and provide the file directly.
	 *                  Optional if CCMCIP is performed or if using CTI mode only.
	 * - Connect() - Actually start the SIP or CTI stacks, ultimately providing an active CC_Device for
	 *               call control.
	 *               Disconnect() is the opposite of connect() and will end all calls and tear down the
	 *               SIP or CTI stack, but will not discard Config, Authentication or DeviceConfig.
	 *               Destroy() wipes everything, ready to free CallControlManager or start fresh.
	 *               The AvailablePhones is CallControlManager's best guess at what devices the user could
	 *               control.  It is allowable for clients to cache or to "know better" and provide device
	 *               names and line DNs from other sources.
	 *               connect() in softphone mode will do its best to automatically call fetchDeviceConfig()
	 *               implicitly as required, so long as sufficient information has already been provided
	 *               via config and/or in AvailablePhones for it to decide which device to use.
	 *
	 *
	 * @code
	 * CallControlManagerPtr manager = CallControlManager::create();
	 * manager->setLocalIpAddress("1.2.3.4");
	 * if(!useFixedDeviceName) {
	 *   manager->setCCMCIPServers("5.6.7.8");
	 * }
	 * manager->setTFTPServers("5.6.7.8");
	 * if(!useFixedDeviceName) {
	 *   manager->setAuthenticationCredentials("myuser", "mypass");
	 *   manager->authenticate();
	 *   manager->connect("", "");
	 * } else {
	 *   manager->connect(myDeviceName, "");
	 * }
	 * // At this point, getActiveDevice() will allow call control.
	 * manager->disconnect();
	 * manager->destroy();
	 * @endcode
	 *
	 * Methods are generally synchronous (at present).
	 */
    class ECC_API CallControlManager
    {
    public:
		/**
		 *  Use create() to create a CallControlManager instance.
		 *
		 *  CallControlManager cleans up its resources in its destructor, implicitly disconnect()in if required.
		 *  Use the destroy() method if you need to force a cleanup earlier.  It is a bad idea to continue using
		 *  CallControlManager or any of its related objects after destroy().
		 */
        static CallControlManagerPtr create();
        virtual bool destroy() = 0;

        virtual ~CallControlManager();

        /**
           CC_Observer is for core call control events (on CC_Device, CC_Line and CC_Call).
           ECC_Observer is for "value add" features of CallControlManager.

           Client can add multiple observers if they have different Observer objects that handle
           different event scenarios, but generally it's probably sufficient to only register one observer.

           @param[in] observer - This is a pointer to a CC_Observer-derived class that the client
                                 must instantiate to receive notifications on this client object.
         */
        virtual void addCCObserver ( CC_Observer * observer ) = 0;
        virtual void removeCCObserver ( CC_Observer * observer ) = 0;

        virtual void addECCObserver ( ECC_Observer * observer ) = 0;
        virtual void removeECCObserver ( ECC_Observer * observer ) = 0;

        /**
         * Main config functions, to be called before connect().
         * This is a separate step because it configures the servers for both CTI and SIPCC modes, allowing
         *   subsequent mode switches to be handled more easily.
         *
	     * @param[in] certificateLevel - This specifies the acceptable certificate level that is to be accepted from
							  the server. For example, if you specify SIGNED_CERT (highest level of strictness)
							  then if the sever gives the client a self-signed cert during HTTPS auth stage then
							  this is deemed not an acceptable level of security based on the fact that the client
							  will only accept SIGNED_CERT. If SELF_SIGNED_CERT had been specified then the cert
							  would be accepted in this case. ALL_CERTS is the lowest level of severity and
							  will accept any class of cert.
         *
         * @param[in] mask - This controls what areas of the underlying Softphone implementation logs messages.
                             pSIPCC has a number of defined areas of functionality for which the logging can
                             be controlled independently, that is, logging can be turned on/off separately for
                             each of these sub-components. The mask specified here has bits that are defined in
                             the #defines at the top of ECC_Types.h.
         */
        virtual void setAuthenticationCredentials(const std::string &username, const std::string& password) = 0;
        virtual void setAuthenticationPolicy(const AuthenticationCertificateLevelType::AuthenticationCertificateLevel& level) = 0;
        virtual void setCCMCIPServers(const std::vector<std::string> &servers) = 0;
        virtual void setCCMCIPServers(const std::string &server) = 0;
        virtual void setTFTPServers(const std::vector<std::string> &servers) = 0;
        virtual void setTFTPServers(const std::string &server) = 0;
        virtual void setMultiClusterMode(bool allowMultipleClusters) = 0;
        virtual void setSIPCCLoggingMask(const cc_int32_t mask) = 0;
        virtual void setAuthenticationString(const std::string &authString) = 0;
        virtual void setSecureCachePath(const std::string &secureCachePath) = 0;

        /**
         * For now, recovery is not implemented.
         * setLocalIpAddressAndGateway must be called before connect()ing in softphone mode.
         */
        virtual void setLocalIpAddressAndGateway(const std::string& localIpAddress, const std::string& defaultGW) = 0;

        /**
         * Performs CCMCIP authentication, which:
         * - validates the server IP, username and password
         * - obtains an initial list of available devices which may be connected to
         *
         * Note that changing the authentication credentials, etc can only be done while
         * disconnect()ed and will wipe any existing available device list and reset
         * hasAuthenticated() to false.
         */
        virtual AuthenticationFailureCodeType::AuthenticationFailureCode authenticate() = 0;
        virtual AuthenticationStatusEnum::AuthenticationStatus getAuthenticationStatus() = 0;
        virtual std::string getLastCCMCIPServerUsed() = 0;

        /**
         * Softphone operation requires an additional config step - the TFTP device config file
         * must be retrieved.  The client is allowed to pass the config verbatim (if it was cached
         * from a previous connection) or to request ECC to fetch a fresh copy.
         */
        virtual DeviceRetrievalFailureCodeType::DeviceRetrievalFailureCode fetchDeviceConfig(const std::string& preferredDeviceName) = 0;
        virtual bool setDeviceConfig(const std::string& preferredDeviceName, const std::string& deviceConfigFileContents) = 0;
        virtual std::string getLastTFTPServerUsed() = 0;

        /**
         * Main connect functions, to start/stop the SIPCC or CTI stacks.
         *   Passing a blank device or line name instructs ECC to pick any available one from those available.
         *   Passing a specific device and/or line name instructs ECC to only use that specific device and/or line,
         *   	and to fail if it cannot successfully register it.
         *
         * Calling connect() before calling authenticate() is fine, and just skips this step.
         *
         * Calling connect() requires a device config for the preferred device to be available.
         * If one has been passed verbatim, it will be used.  If not, this will implicitly call fetchDeviceConfig().
         *
         * All outstanding device, line and call objects are invalidated when disconnect() is called.
         */
        virtual bool connect(const std::string& preferredDeviceName, const std::string& preferredLineDN) = 0;

        virtual bool registerUser( const std::string& deviceName, const std::string& user, const std::string& domain, const std::string& sipContact ) = 0;

        virtual bool disconnect() = 0;
        virtual std::string getPreferredDeviceName() = 0;
        virtual std::string getPreferredLineDN() = 0;
        virtual ConnectionStatusEnum::ConnectionStatus getConnectionStatus() = 0;
        virtual std::string getCurrentServer() = 0;

        /**
         * Obtain the device object, from which call control can be done.
         * getAvailablePhoneDetails lists all known devices which the user is likely to be able to control.
         */
        virtual CC_DevicePtr getActiveDevice() = 0;
        virtual PhoneDetailsVtrPtr getAvailablePhoneDetails() = 0;
        virtual PhoneDetailsPtr getAvailablePhoneDetails(const std::string& deviceName) = 0;

        /**
         * Obtain the audio/video object, from which video setup can be done.
         * This relates to global tuning, device selection, preview window positioning, etc, not to
         * per-call settings or control.
         *
         * These objects are unavailable except while in softphone mode.
         */
        virtual VideoControlPtr getVideoControl() = 0;
        virtual AudioControlPtr getAudioControl() = 0;

    protected:
        CallControlManager() {}
    private:
        CallControlManager(const CallControlManager&);
        CallControlManager& operator=(const CallControlManager&);
    };


};//end namespace CSF
