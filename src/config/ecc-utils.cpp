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

#include "ecc-utils.h"
#include <algorithm>
#include <cctype>

#define WHITESPACE " \t\n\r"

using namespace std;

void trim_left (string & str)
{
    // trim leading spaces
    size_t startpos = str.find_first_not_of(WHITESPACE);
    if( string::npos != startpos )
    {
        str = str.substr( startpos );
    }
}

void trim_right (string & str)
{
    // trim trailing spaces
    size_t endpos = str.find_last_not_of(WHITESPACE);
    if( string::npos != endpos )
    {
        str = str.substr( 0, endpos+1 );
    }
}

void trim_left_and_right (string & str)
{
    trim_left(str);
    trim_right(str);
}

bool getValueOfSubElement (XmlNode parent, const char * pText, string & returnValueStr)
{
    if ((!parent) || (pText == NULL) || (*pText == '\0'))
    {
        return false;
    }

    XmlNode node = parent.getChildByName(pText);

    if (node)
    {
        returnValueStr = node.getString();
        trim_left_and_right(returnValueStr);
    }
    else
    {
        return false;
    }

    return true;
}

int getIntValueFromString (const string & valueStr)
{
    int returnValue = 0;

    istringstream converter(valueStr);

    if (!(converter >> returnValue))
    {
        returnValue = 0;
    }

    return returnValue;
}

void toUpper (string & strToConvertToUpper)
{
    std::transform(strToConvertToUpper.begin(), strToConvertToUpper.end(), strToConvertToUpper.begin(), (int (*)(int))std::toupper);
}

bool equals (const string & str1, const string & str2)
{
    return (str1.compare(str2) == 0);
}

inline bool caseIgnoreCharCompare(char a, char b) {
   return(toupper(a) == toupper(b));
}

bool equalsIgnoreCase (const string & str1, const string & str2)
{
   return((str1.size( ) == str2.size( )) && equal(str1.begin( ), str1.end( ), str2.begin( ), caseIgnoreCharCompare));
}
