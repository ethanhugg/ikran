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

#include <stdarg.h>

#include "CSFLogStream.h"
#include "jsonparser.h"

static const char* logTag = "RoapProxy";

void JsonParser::InsertNameValue(map<string,string>* pMap, const char* name, const char* value)
{
  CSFLogDebugS(logTag, "Adding name/value " << name << " : " << value);
  pMap->insert(pair<string,string>(name, value));
}

map<string, string> JsonParser::Parse(string message)
{
  const int state_outside = 0;
  const int state_toplevel = 1;
  const int state_inname = 2;
  const int state_value = 3;
  const int state_instringvalue = 4;
  const int state_inintvalue = 5;
  const int state_done = 6;
  const int state_error = 7;
  
  map<string,string> result;
  int state = state_outside;
  string currentName;
  string currentValue;
  
  unsigned index;
  for (index = 0; 
       (index < message.length()) && (state != state_done) && (state != state_error); 
       index++)
  {
    char nextChar = message[index];
    
    switch (state)
    {
      case state_outside:
        switch (nextChar)
      {
        case '{':
          state = state_toplevel;
          break;
        case ' ':
        case '\r':
        case '\n':
        case '\t':
          break;
        default:
          CSFLogDebugS(logTag, "Parse error looking for '{'");
          state = state_error;
      }
        break;
      case state_toplevel:
        switch (nextChar)
      {
        case '}':
          state = state_done;
          break;
        case '\"':
          state = state_inname;
          break;
        case ' ':
        case '\r':
        case '\n':
        case '\t':
        case ',':
          break;
        default:
          CSFLogDebugS(logTag, "Invalid char looking for name: " << nextChar);
          state = state_error;
      }
        break;
      case state_inname:
        switch (nextChar)
      {
        case '\"':
          state = state_value;
          break;
        default:
          currentName += nextChar;
          break;
      }
        break;
      case state_value:
        switch (nextChar)
      {
        case ':':
        case ' ':
        case '\r':
        case '\n':
        case '\t':
          break;
        case '\"':
          state = state_instringvalue;
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          currentValue += nextChar;
          state = state_inintvalue;
          break;
        default:
          CSFLogDebugS(logTag, "Invalid char looking for value start: " << nextChar);
          state = state_error;
      }
        break;
      case state_instringvalue:
        switch (nextChar)
      {
        case '\"':
          state = state_toplevel;
          // Got name and value, add to map
          InsertNameValue(&result, currentName.c_str(), currentValue.c_str());
          currentName.clear();
          currentValue.clear();
          break;
        default:
          currentValue += nextChar;
          break;
      }
        break;
      case state_inintvalue:
        switch (nextChar)
      {
        case ' ':
        case '\r':
        case '\n':
        case '\t':
        case ',':
          state = state_toplevel;
          // Got name and value, add to map
          InsertNameValue(&result, currentName.c_str(), currentValue.c_str());
          currentName.clear();
          currentValue.clear();
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          currentValue += nextChar;
          break;
        default:
          CSFLogDebugS(logTag, "Invalid char looking for int value: " << nextChar);
          state = state_error;
          break;
      }
        break;
      case state_done:
        break;
      default:
        CSFLogDebugS(logTag, "Unknown State: " << state);
        break;
    }
  }
  
  return result;
}

