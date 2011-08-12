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

#include "PhoneConfig.h"
#include "TftpHelper.h"
#include "libxml/xmlreader.h"
#include "xmlReadHelper.h"
#include "CSFLog.h"
#include "csf_common.h"

using namespace std;

static const char* logTag = "PhoneConfig";

namespace CSF
{
    PhoneConfig::PhoneConfig()
    {
        init();
    }

    PhoneConfig::~PhoneConfig() 
    {
    }

    bool PhoneConfig::parse(const std::string& configFileContents)
    {
        // Reset all internal values
        init();

        if (configFileContents.length() == 0)
        {
            CSFLogErrorS(logTag, "PhoneConfig::parse() empty deviceXml string passed to PhoneConfig::parse()");
            return false;
        }

        CSFLogDebug(logTag, "PhoneConfig XML: \"%s\"", configFileContents.c_str());

        xmlDocPtr doc = xmlParseDoc((const xmlChar *) configFileContents.c_str());

        if (doc == NULL)
        {
		    CSFLogError(logTag, "PhoneConfig::parse() failed for string : \"%s\"", configFileContents.c_str() );
            return false;
        }

        xmlNodePtr nodePtr = xmlDocGetRootElement(doc);

        if (nodePtr == NULL)
        {
            CSFLogErrorS(logTag, "PhoneConfig::parse() xmlDocGetRootElement returned empty document");
            return false;
        }

        XmlNode rootNode(doc, nodePtr);
        bool bHadProblemParsing = false;

        if (rootNode.isName("device"))
        {
		    XmlNode node = rootNode.getChildByName("fullConfig");
		    if(node)
		    {
                fullConfig = node.getString() == "true";
            }
            else
            {
                bHadProblemParsing = true;
            }

	        node = rootNode.getChildByName("capfAuthMode");
		    if(node)
		    {
                CAPFMode = atoi(node.getString().c_str());
            }
            else
            {
                bHadProblemParsing = true;
            }

	        node = rootNode.getChildByName("certHash");
		    if(node)
		    {
                certificateHash = node.getString().c_str();
            }
            else
            {
                bHadProblemParsing = true;
            }

		    node = rootNode.getChildByName("encrConfig");
		    if(node)
		    {
                encryptedConfig = node.getString() == "true";
            }
            else
            {
                bHadProblemParsing = true;
            }

            // Only do further parsing if we have an unencrypted config
            if(fullConfig)
            {            		       
                // Media ports
                XmlNode startMediaPortNode = rootNode.getChildByHierarchy("sipProfile/startMediaPort");               
                if(startMediaPortNode)
                {
                    mediaStartPort = atoi(startMediaPortNode.getString().c_str());
                }
                else
                {
                    bHadProblemParsing = true;
                }

                XmlNode stopMediaPortNode = rootNode.getChildByHierarchy("sipProfile/stopMediaPort");               
                if(stopMediaPortNode)
                {
                    mediaEndPort = atoi(stopMediaPortNode.getString().c_str());
                }
                else
                {
                    bHadProblemParsing = true;
                }

                XmlNode audioDSCPNode = rootNode.getChildByHierarchy("sipProfile/dscpForAudio");
                if(audioDSCPNode)
                {
                	DSCPAudio = atoi(audioDSCPNode.getString().c_str());
                }
                else
                {
                    bHadProblemParsing = true;
                }

                XmlNode videoDSCPNode = rootNode.getChildByHierarchy("sipProfile/dscpVideo");
                if(videoDSCPNode)
                {
                    DSCPVideo = atoi(videoDSCPNode.getString().c_str());
                }
                else
                {
                    bHadProblemParsing = true;
                }

                XmlNode vadEnabledNode = rootNode.getChildByHierarchy("sipProfile/enableVad");
                if(vadEnabledNode)
                {
                    VADEnabled = videoDSCPNode.getString().compare("true")==0;
                }
                else
                {
                    bHadProblemParsing = true;
                }

                // Call Managers
                XmlNode callManagers = rootNode.getChildByHierarchy("devicePool/callManagerGroup/members");               
                if(callManagers)
                {
                    XmlNode child = callManagers.firstChild();
                    while(child)
                    {
                        XmlNode processNodeName = child.getChildByHierarchy("callManager/processNodeName"); 
                        if(processNodeName)
                        {
                            // Don't populate list if field is empty.
                            if(!processNodeName.getString().empty())
                            {
                                callManagerAddresses.push_back(processNodeName.getString());
                            }
                        }
                        else
                        {
                            bHadProblemParsing = true;
                        }

                        child = child.nextSibling();
                    }                    
                }
                else
                {
                    bHadProblemParsing = true;
                }

                // Line DNs
                XmlNode lines = rootNode.getChildByHierarchy("sipProfile/sipLines");               
                if(lines)
                {
                    XmlNode child = lines.firstChild();
                    while(child)
                    {
                        XmlNode dn = child.getChildByName("name");
                        if(dn)
                        {
                            lineNumbers.push_back(dn.getString());
                        }
                        else
                        {
                            bHadProblemParsing = true;
                        }

                        child = child.nextSibling();
                    }                    
                }
                else
                {
                    bHadProblemParsing = true;
                }
            }
        }
        else
        {
            CSFLogErrorS(logTag, "PhoneConfig::parse() missing <device> element in XML.");
        }

        xmlFreeNode(nodePtr);

        validConfig = !bHadProblemParsing;

        return validConfig;
    }

    bool PhoneConfig::isValidConfig()
    {
        return validConfig;
    }

    bool PhoneConfig::isFullConfig()
    {
        return fullConfig;
    }

    int PhoneConfig::getMediaStartPort()
    {
        return mediaStartPort;
    }

    int PhoneConfig::getMediaEndPort()
    {
        return mediaEndPort;
    }

    std::vector< std::string > PhoneConfig::getCallManagerAddresses()
    {
        return callManagerAddresses;
    }

    std::vector< std::string > PhoneConfig::getLineNumbers()
    {
        return lineNumbers;
    }

    int PhoneConfig::getCAPFMode()
    {
        return CAPFMode;;
    }

    bool PhoneConfig::hasEncryptedConfig()
    {
        return encryptedConfig;
    }

    std::string PhoneConfig::getCertificateHash()
    {
        return certificateHash;
    }

    int PhoneConfig::getDSCPAudio()
    {
    	return DSCPAudio;
    }

    int PhoneConfig::getDSCPVideo()
    {
    	return DSCPVideo;
    }

    bool PhoneConfig::isVADEnabled()
    {
    	return VADEnabled;
    }

    void PhoneConfig::init()
    {
        this->validConfig = false;
        this->fullConfig = false;
        this->mediaStartPort = 0;
        this->mediaEndPort = 0;
        this->callManagerAddresses.clear();
        this->lineNumbers.clear();
        this->CAPFMode = 0;
        this->encryptedConfig = false;
        this->certificateHash = "";
        this->DSCPAudio = 0;
        this->DSCPVideo = 0;
        this->VADEnabled = false;

    }
}
