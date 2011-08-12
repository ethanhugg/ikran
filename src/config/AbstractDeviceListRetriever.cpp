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

#include "AbstractDeviceListRetriever.h"
#include "ecc-utils.h"
#include <libxml/HTMLparser.h>

using namespace std;
using namespace CSFUnified;


static const char* logTag = "DeviceListRetriever";

static bool checkAllDOMForAuthFailed (const XmlNode & next)
{
    if (!next) return false;

    if (next.getType() == XML_ELEMENT_NODE)
    {
        string value(next.getString());

        size_t found = value.find("HTTP Status");

        if (found!=string::npos)
        {
            found = value.find("401");

            if (found!=string::npos)
            {
                CSFLogWarnS(logTag, "Found \"HTTP Status 401\" embedded in [X]HTML.");
                return true;
            }
        }
    }
    //else
    //{
    //    bool bwd = true;
    //}

    bool authFailed = checkAllDOMForAuthFailed(next.nextSibling());

    if (!authFailed)
    {
        authFailed = checkAllDOMForAuthFailed(next.firstChild());
    }

    return authFailed;
}



//This function checks if there's a 401 error embedded XML(or HTML) returned from CM.
static bool checkIfHTTPAuthFailed (xmlDocPtr doc)
{
    xmlNodePtr rootNodePtr = xmlDocGetRootElement(doc);

    if (rootNodePtr == NULL)
    {
    	CSFLogErrorS(logTag, "xmlDocGetRootElement() returned empty document.");
        return false;
    }
    else
    {
        XmlNode node(doc, rootNodePtr);
        return checkAllDOMForAuthFailed(node);
    }

    return false;
}

//This function checks if there's a 401 error embedded in the HTML returned by the server.
static bool checkIfHTTPAuthFailedHTML (const char * str, size_t len)
{
    htmlDocPtr doc = htmlReadDoc((const xmlChar*)str, "", NULL, HTML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);

    if (doc == NULL)
    {
    	CSFLogWarnS(logTag, "htmlReadDoc() failed\n" );
        return false;
    }

    bool authFailed = checkIfHTTPAuthFailed(doc);

    xmlFreeDoc(doc);

    return authFailed;
}

/* static */
void AbstractDeviceListRetriever::setErrorCode(int * pErrorCode, int value)
{
    if (pErrorCode != NULL)
    {
        *pErrorCode = value;
    }
}

static bool parseDevicesXML (xmlDocPtr doc, xmlNodePtr nodePtr, DeviceMap & deviceMap, int * pErrorCode)
{
    XmlNode node(doc, nodePtr);
    bool bHadProblemParsing = false;

    if (!node.isName("devices"))
    {
        //Might have received some error info in [X]HTML text response.
    	CSFLogWarnS(logTag, "Missing <devices> element in XML.");
        AbstractDeviceListRetriever::setErrorCode(pErrorCode, DEVICE_RETRIEVE_XML_PARSE_DOC_FAILED);
        bHadProblemParsing = true;
    }
    else
    {
        XmlNode deviceNode = node.firstChild();

        if (!deviceNode)
        {
        	CSFLogErrorS(logTag, "Missing <device> element in XML.");
            bHadProblemParsing = true;
        }
        else
        {
            while (true)
            {
                string nameValueStr;
                string descriptionValueStr;
                string modelValueStr;

                if ((getValueOfSubElement(deviceNode, "name", nameValueStr)) &&
                    (getValueOfSubElement(deviceNode, "description", descriptionValueStr)) &&
                    (getValueOfSubElement(deviceNode, "model", modelValueStr)))
                {
                    DeviceInfo deviceInfo;
                    deviceInfo.setName(nameValueStr);
                    deviceInfo.setDescription(descriptionValueStr);
                    deviceInfo.setModel(modelValueStr);

                    deviceMap[nameValueStr] = deviceInfo;
                }
                else
                {
                    bHadProblemParsing = true;
                    break;
                }

                deviceNode = deviceNode.nextSibling();

                if (!deviceNode)
                {
                    //no more <device> elements.
                    break;
                }
            }//end while
        }
    }

    return !bHadProblemParsing;
}

/* static */
bool AbstractDeviceListRetriever::createDeviceListFromXML (const string & devicesXml, DeviceMap & deviceMap, int * pErrorCode)
{
    if (devicesXml.length() == 0)
    {
    	CSFLogErrorS(logTag, "Received empty devicesXml string. No device list generated.");
        setErrorCode(pErrorCode, DEVICE_RETRIEVE_EMPTY_STRING);
        return false;
    }

    string trimmedXML = devicesXml;

    trim_left(trimmedXML);
    trim_right(trimmedXML);

    if (trimmedXML.length() == 0)
    {
    	CSFLogErrorS(logTag, "XML string received from server only contained whitespace. No device list generated.");
        setErrorCode(pErrorCode, DEVICE_RETRIEVE_EMPTY_STRING);
        return false;
    }

    CSFLogDebug(logTag, "XML: \"%s\"", trimmedXML.c_str());

    xmlDocPtr doc = xmlParseDoc((const xmlChar *) trimmedXML.c_str());

    if (doc == NULL)
    {
    	CSFLogWarn(logTag, "xmlParseDoc() failed for string : \"%s\"", trimmedXML.c_str() );
        //When I connect to "7.1.2.10000-16", it seem to send back invalid XML when I enter invalid
        //credentials to a valid server. Looking at this closer it's clearly HTML (not XHTML). XMLParser
        //doesn't like the HTML.
        //6.1.2.9961-3

        //However, when I connect to "8.5.1.10000-10" it returns XHTML, so I handle that case
        //separately below.
        if (checkIfHTTPAuthFailedHTML(trimmedXML.c_str(), trimmedXML.length()))
        {
        	CSFLogErrorS(logTag, "Authorization failed connecting to server." );
            setErrorCode(pErrorCode, DEVICE_RETRIEVE_AUTH_FAILED);
        }
        else
        {
            setErrorCode(pErrorCode, DEVICE_RETRIEVE_XML_PARSE_DOC_FAILED);
        }

        return false;
    }

    //Need to free doc before returning. Better to use bool value and single return point from here out.

    bool bCreatedDeviceList = false;

    xmlNodePtr nodePtr = xmlDocGetRootElement(doc);

    if (nodePtr == NULL)
    {
    	CSFLogErrorS(logTag, "xmlDocGetRootElement() returned empty document.");
        setErrorCode(pErrorCode, DEVICE_RETRIEVE_EMPTY_STRING);
    }
    else
    {
        bCreatedDeviceList = parseDevicesXML(doc, nodePtr, deviceMap, pErrorCode);
    }

    if ((!bCreatedDeviceList) && (*pErrorCode == DEVICE_RETRIEVE_XML_PARSE_DOC_FAILED))
    {
        if (checkIfHTTPAuthFailed(doc))
        {
        	CSFLogErrorS(logTag, "Authorization failed connecting to server." );
            setErrorCode(pErrorCode, DEVICE_RETRIEVE_AUTH_FAILED);
        }
    }

    xmlFreeDoc(doc);

    return bCreatedDeviceList;
}
