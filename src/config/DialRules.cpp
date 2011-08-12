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
#include "DialRules.h"
#include "ecc-utils.h"

#include <sstream>
#include <functional>
#include <algorithm>

using namespace std;
using namespace CSFUnified;

// static initialization
static const char* logTag = "DialRules";

/*

DialRule

*/

DialRule::DialRule (string aBeginsWith, int aNumChars, int aNumToRemove, string aPrefixWith) :
  beginsWith(aBeginsWith),
  numCharsInNumber(aNumChars),
  numCharsToRemoveFromStart(aNumToRemove),
  prefixWith(aPrefixWith)
{
}

/**
   Returns true if phoneNumber matches this DialRule.
   
   @param phoneNumber
 */
bool DialRule::checkIfRuleMatchesNumber (const string & phoneNumber) const
{
	if (phoneNumber.length() > 0)
    {
        if ((numCharsInNumber > 0) && ((int) phoneNumber.length() != numCharsInNumber))
		{
            return false;
		}

        return (phoneNumber.compare(0, beginsWith.length(), beginsWith) == 0);
    }
    return false;
}

/**
   Converts phoneNumber according to this DialRule.

   This function does not check if the rule applied to this number. It merely applies it.
   The function checkIfRuleMatchesNumber() is used to determine if the rule applies. Invocation
   of these functions is controlled by the fact that they are declared private. The class
   DialRules is the only class that can use these functions, are it uses them in a controlled
   way.
   
   @param phoneNumber

   @return applies rule to phone number.
 */
string DialRule::transformNumber (const string & phoneNumber) const
{
	string transformedNumber = phoneNumber;
	return prefixWith + transformedNumber.erase(0, numCharsToRemoveFromStart);
}

string DialRule::toString() const
{
	stringstream sstream;
    sstream << "BeginsWith=" << beginsWith;
    sstream << ", NumChars=" << numCharsInNumber;
    sstream << ", DigitsToRemove=" << numCharsToRemoveFromStart;
    sstream << ", PrefixWith=" << prefixWith;
    return sstream.str();
}

/*

DialRules

*/

DialRules::DialRules(const std::string & dialRulesXMLString) :
  errorCode(0)
{
    rulesAreValid = generateDialRulesFromXML(dialRulesXMLString, dialRules, &errorCode);
}

bool DialRules::areRulesValid()
{
    return rulesAreValid;
}

int DialRules::getErrorCode()
{
    return errorCode;
}

string DialRules::applyRulesTo (const string & phoneNumber) const
{
	string transformedNumber = phoneNumber;

	for (vector<DialRule>::const_iterator it = dialRules.begin(); it != dialRules.end(); it++ )
	{
		const DialRule & rule = *it;

		if (rule.checkIfRuleMatchesNumber(phoneNumber))
		{
			return rule.transformNumber(phoneNumber);
		}
	}

	return phoneNumber;
}

/* static */
void DialRules::setErrorCode(int * pErrorCode, int value)
{
    if (pErrorCode != NULL)
    {
        *pErrorCode = value;
    }
}

static bool parseDialRulesXML (xmlDocPtr doc, xmlNodePtr nodePtr, DialRuleVector & dialRuleVector, int * pErrorCode)
{
    XmlNode node(doc, nodePtr);
    bool bHadProblemParsing = false;

    if (!node.isName("DialRules"))//case sensitive
    {
    	CSFLogErrorS(logTag, "Missing <DialRules> element in XML.");
        DialRules::setErrorCode(pErrorCode, DIAL_RULES_XML_PARSE_DOC_FAILED);
        bHadProblemParsing = true;
    }
    else
    {
        XmlNode dialRuleNode = node.firstChild();

        if (!dialRuleNode)
        {
        	CSFLogWarnS(logTag, "Missing <DialRule> element in XML. There are no dial rules contained in the <DialRules> element.");
            //This is not a parse problem, it's just that the result of parsing is that we have no dial rules.
        }
        else
        {
            while (true)
            {
                if (dialRuleNode.isName("DialRule"))//case sensitive
                {
                    string beginsWithStr;
                    string numDigitsStr;
                    string digitsToRemoveStr;
                    string prefixWithStr;

                    //Rule must contain either a BeginsWith or NumDigits (or both), rest is optional, though
                    //generally rule is redundant unless it performs some operation on the number via

                    if (dialRuleNode.hasAttribute("NumDigits") &&
                        dialRuleNode.hasAttribute("DigitsToRemove") &&
                        dialRuleNode.hasAttribute("PrefixWith"))
                    {
                        numDigitsStr = dialRuleNode.getAttribute("NumDigits"); //Must be there, should be a positive number >0
                        digitsToRemoveStr = dialRuleNode.getAttribute("DigitsToRemove");//Can be an empty string, but must be there.
                                                                                        //Empty string means don't remove any digits, 
                                                                                        //as does a value "0". 
                        prefixWithStr = dialRuleNode.getAttribute("PrefixWith");//Can be an empty string, but must be there.

                        //Optional fields
                        beginsWithStr = dialRuleNode.getAttribute("BeginsWith");

                        int numDigitsNum = getIntValueFromString(numDigitsStr);
                        int numDigitsToRemove = getIntValueFromString(digitsToRemoveStr);
                            
                        //DialRule (string aBeginsWith, int aNumChars, int aNumToRemove, string aPrefixWith);

                        DialRule dialRule(beginsWithStr, numDigitsNum, numDigitsToRemove, prefixWithStr);

                        dialRuleVector.push_back(dialRule);
                    }
                    else
                    {
                        string elementDumpStr = dialRuleNode.dumpElementDetails();
                        CSFLogWarn(logTag, "Ignoring unsupported DialRule \"%s\". A DialRule must contain all the attributes "
                                            	 "\"NumDigits\", \"DigitsToRemove\" and \"PrefixWith\".", elementDumpStr.c_str());
                    }
                }
                else
                {
                    string elementDumpStr = dialRuleNode.dumpElementDetails();
                    CSFLogWarn(logTag, "Ignoring unrecognised element \"%s\".", elementDumpStr.c_str());
                }

                dialRuleNode = dialRuleNode.nextSibling();

                if (!dialRuleNode)
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
bool DialRules::generateDialRulesFromXML (const string & dialRulesXMLString, DialRuleVector & dialRuleVector, int * pErrorCode)
{
    if (dialRulesXMLString.length() == 0)
    {
    	CSFLogErrorS(logTag, "Received empty dialRulesXMLString string. No device list generated.");
        setErrorCode(pErrorCode, DIAL_RULES_EMPTY_STRING);
        return false;
    }

    string trimmedXML = dialRulesXMLString;

    trim_left(trimmedXML);
    trim_right(trimmedXML);

    if (trimmedXML.length() == 0)
    {
    	CSFLogErrorS(logTag, "XML string received from server only contained whitespace. No device list generated.");
        setErrorCode(pErrorCode, DIAL_RULES_EMPTY_STRING);
        return false;
    }

    CSFLogDebug(logTag, "XML: \"%s\"", trimmedXML.c_str());

    xmlDocPtr doc = xmlParseDoc((const xmlChar *) trimmedXML.c_str());

    if (doc == NULL)
    {
    	CSFLogWarn(logTag, "xmlParseDoc() failed for string : \"%s\"", trimmedXML.c_str() );

        return false;
    }

    bool bCreatedDialRules = false;

    xmlNodePtr nodePtr = xmlDocGetRootElement(doc);

    if (nodePtr == NULL)
    {
    	CSFLogErrorS(logTag, "xmlDocGetRootElement() returned empty document.");
        setErrorCode(pErrorCode, DIAL_RULES_EMPTY_STRING);
    }
    else
    {
        bCreatedDialRules = parseDialRulesXML(doc, nodePtr, dialRuleVector, pErrorCode);
    }

    xmlFreeDoc(doc);

    return bCreatedDialRules;
}

string DialRules::toString() const
{
	int i=0;
	vector<DialRule>::const_iterator it;
	stringstream sstream;

	sstream << "{";

	for (i=0, it=dialRules.begin(); it != dialRules.end(); it++, i++ )
	{
		const DialRule & rule = *it;

		if (i>0)
		{
			sstream << ", ";
		}

		sstream << "{";
		string ruleStr = rule.toString();
		sstream << ruleStr << "}";
	}

	sstream << "}";

	return sstream.str();
}
