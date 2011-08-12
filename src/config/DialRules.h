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

#ifndef _DIAL_RULES_H
#define _DIAL_RULES_H

#include <string>
#include <vector>

#define DIAL_RULES_EMPTY_STRING         1
#define DIAL_RULES_XML_PARSE_DOC_FAILED 2
#define DIAL_RULES_TIMEOUT              3

namespace CSFUnified
{
    class DialRules;

    /**

    Application Dial Rules (some information taken from http://www.cisco.com/en/US/docs/voice_ip_comm/cucm/admin/5_0_1/ccmsys/a03adial.html

    Application dial rules should conform to the following:

    - The "BeginsWith" (optional) string supports only digits (0-9) and the 3 additional
      Chars +*#. The length of this string should not exceed 100 Chars.

    - The "NumDigits" (required) string supports only digits (0-9).
      <b>This field cannot be blank for a dial rule, and its numeric value must be >0.</b>

    Assuming a rule "matches" to a given phone number, the following field describe how that
    number is then to be transformed.

    - The "DigitsToRemove" (required) string supports only digits as it is the numeric value indicating
      the number of chacracters to remove from the beginning of the number. The value of this field
      must not be greater than the "NumDigits" field.

    - The "PrefixWith" (required) string supports only digits (0-9) and the Chars +*#.
      Can me an empty string, but must always be specified in the XML.
      The length cannot exceed 100 Chars.

    - The "DigitsToRemove" and "PrefixWith" strings cannot both be blank. 

    - Dial rules should be unique.

    */

class DialRule
    {
    friend class DialRules;

    public:

        DialRule (std::string aBeginsWith, int aNumChars, int aNumCharsToRemove, std::string aPrefixWith);

    private:
        //Rule matching criteria are: beginsWith && numCharsInNumber
        std::string beginsWith; //To match this dial rule the phone number being checked must "begin with" this sequence of Chars.
                                //Generally digits, but may also include '+' character at the start. We don't care what character is specified
                                //here, we just check for starts with match. If the rule is not concerned with what the phone number starts
                                //with, then this field can be blank (empty string).
        int numCharsInNumber;   //To match this dial rule the phone number being checked must have this exact number of characters. If this
                                //rule does not care about the length of the number, then this field will have the value 0.

        //Transformation operations: Assuming the rule matches a given number, the numberOfDigitsToRemove dictate
        //what form of operation is to be carried out on the matched number.
        
        int numCharsToRemoveFromStart; //This defines the number of digits to remove from the beginning of the phone number string.
                                       //This can have a value >=0.
        std::string prefixWith; //Having removed some Chars from the beginning of the string (may be 0 Chars removed).
        
        /**
           Returns true if phoneNumber matches the dialing rule entry.
           
           Want to control access to this function, as it's really
           only the DialRules class that should be using this function.
           For that reason, DialRules class is declared a friend
           to give is special access to this function.
           
           @param phoneNumber
         */
        bool checkIfRuleMatchesNumber (const std::string  & phoneNumber) const;
        
        /**
           Transform phoneNumber according to dialing rule entry.
           
           Want to control access to this function, as it's really
           only the DialRules class that should be using this function.
           For that reason, DialRules class is declared a friend
           to give is special access to this function.
           
           @param phoneNumber

           @return the number with the transformation 
         */
        std::string transformNumber (const std::string & phoneNumber) const;

	public:
        /**
           Returns a string representation of the dial rule. To be used
           only for illustration purposes (logging etc).
           
         */
        std::string toString () const;
    };

    //
    typedef std::vector<DialRule> DialRuleVector;
    //

	class DialRules
	{
	private:
        int errorCode;
        bool rulesAreValid;
        DialRuleVector dialRules;

        static bool generateDialRulesFromXML (const std::string & dialRulesXMLString, DialRuleVector & dialRuleVector, int * pErrorCode);

	public:

        DialRules(const std::string & dialRulesXMLString);

        bool areRulesValid();
        int getErrorCode();
        int numRules() { return (int) dialRules.size(); }

		std::string applyRulesTo (const std::string & phoneNumber) const;
        static void setErrorCode (int * pErrorCode, int value);

        /**
           Returns a string representation of the dial rule. To be used
           only for illustration purposes (logging etc).
           
         */
        std::string toString () const;
	};
}//end namespace.

#endif
