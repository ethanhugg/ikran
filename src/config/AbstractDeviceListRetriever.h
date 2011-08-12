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

#ifndef _ABSTRACT_DEVICE_LIST_RETRIEVER_H
#define _ABSTRACT_DEVICE_LIST_RETRIEVER_H

#include <string>
#include <map>

//Error codes that may be returned by createDeviceListFromXML() in its pErrorCode field.
//
#define DEVICE_RETRIEVE_EMPTY_STRING         1
#define DEVICE_RETRIEVE_XML_PARSE_DOC_FAILED 2
#define DEVICE_RETRIEVE_AUTH_FAILED          3// Authorization failed (401 error) from server.
#define DEVICE_RETRIEVE_TIMEOUT              4

namespace CSFUnified
{
    namespace DeviceRetrievalErrorCodeType
    {
        typedef enum
        {
            EMPTY_XML_STRING_RECEIVED_FROM_SERVER
        } DeviceRetrievalErrorCode;
    };

    // A class to represent a device which has 3 attributes:
    // name, description and model.

    class DeviceInfo
    {
    private:
        std::string name;
        std::string description;
        std::string model;

    public:
        const std::string & getName (void) const { return this->name; }
        void setName (const std::string & name) { this->name = name; }

        const std::string & getDescription(void) const { return this->description; }
        void setDescription (const std::string & description) { this->description = description; }

        const std::string & getModel (void) const { return this->model; }
        void setModel (const std::string & model) { this->model = model; }
    };

    //
    typedef std::map<std::string, DeviceInfo> DeviceMap;
    //

    class AbstractDeviceListRetriever
    {
    protected:
        AbstractDeviceListRetriever (void) { }
        // The destructor
        virtual ~AbstractDeviceListRetriever(void) {};

    protected:
        //Deriving class should call this function
        //after it has retrieved the device list by whatever
        //means the derived class implements (see CCMCIPClient::fetchDevices()
        //for an example of this.
        static bool createDeviceListFromXML (const std::string & devicesXml, DeviceMap & deviceMap, int * pErrorCode);
    public:
        static void setErrorCode(int * pErrorCode, int value);
    };
}

#endif
