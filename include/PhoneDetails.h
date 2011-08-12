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

#include <string>

#include "CC_Common.h"
#include "ECC_Types.h"

namespace CSF
{
	DECLARE_PTR_VECTOR(PhoneDetails);
	class ECC_API PhoneDetails
	{
	public:
		virtual ~PhoneDetails() {}
		/**
		 * Get the device name (the CUCM device name) and the free text description.
		 */
		virtual std::string getName() const = 0;
		virtual std::string getDescription() const = 0;

		/**
		 * Get the model number (the internal CUCM number, not the number printed on the phone)
		 * and the corresponding description (which normally does include the number printed on the phone).
		 * Returns -1, "" if unknown (e.g. CCMCIP does not see the model number, but CTI fills it in).
		 */
		virtual int getModel() const = 0;
		virtual std::string getModelDescription() const = 0;

		virtual bool isSoftPhone() = 0;

		/**
		 * List the known directory numbers of lines associated with the device.
		 * Empty list if unknown.
		 */
		virtual std::vector<std::string> getLineDNs() const = 0;

		/**
		 * Current status of the device, if known.
		 */
		virtual ServiceStateType::ServiceState getServiceState() const = 0;

		/**
		 * TFTP config of device, and freshness of the config.
		 */
		virtual std::string getConfig() const = 0;
		virtual DeviceConfigStatusEnum::DeviceConfigStatus getConfigStatus() const = 0;

	protected:
		PhoneDetails() {}

	private:
		PhoneDetails(const PhoneDetails&);
		PhoneDetails& operator=(const PhoneDetails&);
	};
};