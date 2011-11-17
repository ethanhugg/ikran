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

#include <stdio.h>
#include <cstring>
#include "EventPrinter.h"

#include <set>

#include "csf_common.h"
#include "CSFLogStream.h"
#include "CC_Device.h"
#include "CC_DeviceInfo.h"
#include "CC_FeatureInfo.h"
#include "CC_CallServerInfo.h"
#include "CC_Line.h"
#include "CC_LineInfo.h"
#include "CC_Call.h"
#include "CC_CallInfo.h"
#include "debug-psipcc-types.h"
#include "PhoneDetails.h"

#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"

using namespace CSF;
using namespace std;

static const char* logTag = "EventPrinter";

#ifdef ECC_INCLUDE_THREADID
#define THREADID_OFFSET 7
#else
#define THREADID_OFFSET 0
#endif

#define EVENT_OFFSET                    (2 + THREADID_OFFSET)
#define EVENT_NAME_FIELD_WIDTH          35
#define HANDLE_FIELD_WIDTH              7
#define VARIABLE_NAME_FIELD_WIDTH       24
#define WRAPPED_LINE_ALIGNMENT_OFFSET_1 (EVENT_OFFSET + EVENT_NAME_FIELD_WIDTH + HANDLE_FIELD_WIDTH + 2)//+2 for 2 spaces either size of handle
#define WRAPPED_LINE_ALIGNMENT_OFFSET_2 (WRAPPED_LINE_ALIGNMENT_OFFSET_1 + VARIABLE_NAME_FIELD_WIDTH + 3)

static void printHeaderBlockIfNotAlreadyPrinted()
{
    static bool printedHeaderBlockAlready=false;

    if (!printedHeaderBlockAlready)
    {
#ifdef ECC_INCLUDE_THREADID
    	CSFLogDebugS(logTag, "Ev  TID              Event Name              Handle                           Event-related info");
    	CSFLogDebugS(logTag, "== ====== ================================== ======= ===========================================================================");
#else
        CSFLogDebugS(logTag, "Ev            Event Name              Handle                           Event-related info");
        CSFLogDebugS(logTag, "== ================================== ======= ===========================================================================");
#endif
        printedHeaderBlockAlready = true;
    }
}

static void printCurrentThreadID()
{
#ifdef ECC_INCLUDE_THREADID
    long tid = Timer::getCurrentThreadId();

    CSFLogDebug(logTag, " 0x%04lx", tid);
#endif
}

static std::string truncateLeft (cc_string_t pEventName, int maxAllowedLength)
{
    std::string eventNameStr = pEventName;
    int len = (int) eventNameStr.length();

    if (len > maxAllowedLength)
    {
        eventNameStr = eventNameStr.substr(len - maxAllowedLength, EVENT_NAME_FIELD_WIDTH);
    }

    return eventNameStr;
}


#define PRINT_EVENT_INFO_NICELY_USING_GETNAME(CCDataItemName, sharedPtr, CCAPI_MemberFunctionName, CCEnumTypeToStringFunc)\
        CSFLogDebug(logTag, "(%s ", CCDataItemName );\
        CSFLogDebugS(logTag, "----------");\
        CSFLogDebug(logTag, " %s)", CCEnumTypeToStringFunc(sharedPtr->CCAPI_MemberFunctionName()))

#define PRINT_EVENT_INFO_NICELY_FOR_STRING(CCDataItemName, sharedPtr, CCAPI_MemberFunctionName)\
        CSFLogDebug(logTag, "(%s ", CCDataItemName );\
        CSFLogDebugS(logTag, "----------");\
        CSFLogDebug(logTag, " %s)", sharedPtr->CCAPI_MemberFunctionName().c_str())

#define PRINT_EVENT_INFO_NICELY_FOR_INT32(CCDataItemName, sharedPtr, CCAPI_MemberFunctionName)\
        CSFLogDebug(logTag, "(%s ", CCDataItemName );\
        CSFLogDebugS(logTag, "----------");\
        CSFLogDebug(logTag, " %d)", sharedPtr->CCAPI_MemberFunctionName())

#define PRINT_EVENT_INFO_NICELY_FOR_BOOL(CCDataItemName, sharedPtr, CCAPI_MemberFunctionName)\
        {\
        CSFLogDebug(logTag, "(%s ", CCDataItemName );\
        bool boolValue = sharedPtr->CCAPI_MemberFunctionName();\
        CSFLogDebugS(logTag, "----------");\
        CSFLogDebug(logTag, " %s)", (boolValue) ? "true" : "false");\
        }

#define PRINT_OFFSET(offsetSize)\
    CSFLogDebug(logTag, "%*s", offsetSize, "")

#define PRINT_OFFSET_1 PRINT_OFFSET(WRAPPED_LINE_ALIGNMENT_OFFSET_1)
#define PRINT_OFFSET_2 PRINT_OFFSET(WRAPPED_LINE_ALIGNMENT_OFFSET_2)

static void printCallCaps (const set<CC_CallCapabilityEnum::CC_CallCapability> & caps, int wrapOffsetValue)
{
    int numCapsPrinted = 0;
    int numCaps = caps.size();

    if (numCaps == 0)
    {
        CSFLogDebugS(logTag, "(empty caps)");
    }
    else
    {
        bool havePrintedACap = false;

    	for(std::set<CC_CallCapabilityEnum::CC_CallCapability>::const_iterator it = caps.begin(); it != caps.end(); it++)
        {
			if (!havePrintedACap)
			{
				havePrintedACap = true;
			}
			else
			{
				if ((numCapsPrinted % 2) == 0)
				{
					PRINT_OFFSET(wrapOffsetValue);
				}
			}

			CSFLogDebug(logTag, "%s", CC_CallCapabilityEnum::toString(*it).c_str());
			++numCapsPrinted;
        }
    }
}

static void printLineCapabilities (const CC_LineInfoPtr & info, int wrapOffsetValue)
{
    int numCapsPrinted = 0;
    bool havePrintedACap = false;


    for (int i=0; i < CCAPI_CALL_CAP_MAX; i++)
    {
        if (info->hasCapability((ccapi_call_capability_e)i))
        {
            if (!havePrintedACap)
            {
                havePrintedACap = true;
            }
            else
            {
                if ((numCapsPrinted % 2) == 0)
                {
                    PRINT_OFFSET(wrapOffsetValue);
                }
            }

            CSFLogDebug(logTag, "%s", call_capability_getname((ccapi_call_capability_e) i));
            ++numCapsPrinted;
        }
    }

    if (numCapsPrinted == 0)
    {
        CSFLogDebugS(logTag, "empty caps");
    }
}

static void printCallCapabilities (const CC_CallInfoPtr& info, int wrapOffsetValue)
{
	printCallCaps(info->getCapabilitySet(), wrapOffsetValue);
}

static void printCallPartiesInfo(const CC_CallInfoPtr & info)
{
    bool isConference = info->getIsConference();;

    if (isConference)
    {
        CSFLogDebugS(logTag, "(IsConference ");
        CSFLogDebugS(logTag, "----------");
        CSFLogDebug(logTag, " %s )", (isConference) ? "true" : "false");
    }
    else
    {
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallType", info, getCallType, call_type_getname);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_STRING   ("CalledPartyName", info, getCalledPartyName);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_STRING   ("CalledPartyNumber", info, getCalledPartyNumber);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_STRING   ("CallingPartyName", info, getCallingPartyName);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_STRING   ("CallingPartyNumber", info, getCallingPartyNumber);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_STRING   ("AlternateNumber", info, getAlternateNumber);
    }
}

static base::Lock eventPrinterMutex;

void ECC_CapsPrinter::onAvailablePhoneEvent (AvailablePhoneEventType::AvailablePhoneEvent phoneEvent,
                                             const PhoneDetailsPtr availablePhoneDetails)
{
	base::AutoLock lock(eventPrinterMutex);
    printHeaderBlockIfNotAlreadyPrinted();

    CSFLogDebugS(logTag, "P:");
    printCurrentThreadID();
    CSFLogDebug(logTag, "%*s ------- %s", EVENT_NAME_FIELD_WIDTH, toString(phoneEvent).c_str(), availablePhoneDetails->getName().c_str());
}

void ECC_CapsPrinter::onAuthenticationStatusChange (AuthenticationStatusEnum::AuthenticationStatus status)
{
	base::AutoLock lock(eventPrinterMutex);
    printHeaderBlockIfNotAlreadyPrinted();

    CSFLogDebugS(logTag, "A:");
    printCurrentThreadID();
    CSFLogDebug(logTag, "%*s ", EVENT_NAME_FIELD_WIDTH, toString(status).c_str());
}

void ECC_CapsPrinter::onConnectionStatusChange(ConnectionStatusEnum::ConnectionStatus status)
{
	base::AutoLock lock(eventPrinterMutex);
    printHeaderBlockIfNotAlreadyPrinted();

    CSFLogDebugS(logTag, "S:");
    printCurrentThreadID();
    CSFLogDebug(logTag, "%*s ", EVENT_NAME_FIELD_WIDTH, toString(status).c_str());
}

void CC_CapsPrinter::onCallEvent (ccapi_call_event_e callEvent, CC_CallPtr call, CC_CallInfoPtr info, char* sdp )
{
	base::AutoLock lock(eventPrinterMutex);
    printHeaderBlockIfNotAlreadyPrinted();

    CSFLogDebugS(logTag, "C:");
    printCurrentThreadID();

    std::string eventNameStr = truncateLeft(call_event_getname(callEvent), EVENT_NAME_FIELD_WIDTH);

    CSFLogDebug(logTag, "%*s %s ", EVENT_NAME_FIELD_WIDTH, eventNameStr.c_str(), call->toString().c_str());

    switch (callEvent)
    {
    case CCAPI_CALL_EV_CALLINFO:
        printCallPartiesInfo(info);
        break;
    case CCAPI_CALL_EV_STATE:
        /*
         Whenever we get a CCAPI_CALL_EV_STATE the expectation is that we call getCapabilitySet() to
         get the current list of caps, or we call hasCapability(ccapi_call_capability_e) to determine
         if a given capability is available or not.
         */
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallState", info, getCallState, call_state_getname);
        PRINT_OFFSET_1;
        if (info->getCallState() == RINGIN)
        {
            printCallPartiesInfo(info);
        }
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallType", info, getCallType, call_type_getname);
        PRINT_OFFSET_1;
		PRINT_EVENT_INFO_NICELY_USING_GETNAME("VideoDirection", info, getVideoDirection, sdp_direction_getname);
	    PRINT_OFFSET_1;
        CSFLogDebugS(logTag, "(CallCaps ");
        CSFLogDebugS(logTag, "----------");

        printCallCapabilities(info, WRAPPED_LINE_ALIGNMENT_OFFSET_2);
        break;
    case CCAPI_CALL_EV_STATUS:
        PRINT_EVENT_INFO_NICELY_FOR_STRING   ("CallStatus", info, getStatus);
        break;
    case CCAPI_CALL_EV_ATTR:
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallAttr", info, getCallAttr, call_attr_getname);
        break;
    case CCAPI_CALL_EV_SECURITY:
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallSecurity", info, getSecurity, call_security_getname);
        break;
    case CCAPI_CALL_EV_CAPABILITY:
        //
        printCallCapabilities(info, WRAPPED_LINE_ALIGNMENT_OFFSET_1);
        break;
    case CCAPI_CALL_EV_VIDEO_AVAIL:
		PRINT_EVENT_INFO_NICELY_USING_GETNAME("VideoDirection", info, getVideoDirection, sdp_direction_getname);
	    PRINT_OFFSET_1;
	    break;
    case CCAPI_CALL_EV_VIDEO_OFFERED:
		PRINT_EVENT_INFO_NICELY_USING_GETNAME("VideoDirection", info, getVideoDirection, sdp_direction_getname);
	    PRINT_OFFSET_1;
	    break;
    default:
    	break;
    }//end switch
}

void CC_CapsPrinter::onDeviceEvent (ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_DeviceInfoPtr info )
{
	base::AutoLock lock(eventPrinterMutex);
    printHeaderBlockIfNotAlreadyPrinted();

    CSFLogDebugS(logTag, "D:");
    printCurrentThreadID();

    std::string eventNameStr = truncateLeft(device_event_getname(deviceEvent), EVENT_NAME_FIELD_WIDTH);

    CSFLogDebug(logTag, "%*s %s ", EVENT_NAME_FIELD_WIDTH, eventNameStr.c_str(), device->toString().c_str());

    switch (deviceEvent)
    {
    case CCAPI_DEVICE_EV_STATE:
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("ServiceState", info, getServiceState, service_state_getname);
        break;
    case CCAPI_DEVICE_EV_SERVER_STATUS:
    {
        vector<CC_CallServerInfoPtr> callServers = info->getCallServers();
        for (size_t i=0; i<callServers.size(); i++)
        {
            CSFLogDebug(logTag, "%*s(CallServer %lu: ", WRAPPED_LINE_ALIGNMENT_OFFSET_1, "", (unsigned long) i);
            CSFLogDebugS(logTag, "----------");
            CC_CallServerInfoPtr callServerInfoPtr = callServers[i];
            PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallServerMode", callServerInfoPtr, getCallServerMode, cucm_mode_getname);
            PRINT_OFFSET_2;
            PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallServerStatus", callServerInfoPtr, getCallServerStatus, ccm_status_getname);
        }
        break;
    }
    default:
    	break;
    }//end switch
}

void CC_CapsPrinter::onFeatureEvent (ccapi_device_event_e deviceEvent, CC_DevicePtr device, CC_FeatureInfoPtr feature_info)
{
	base::AutoLock lock(eventPrinterMutex);
    printHeaderBlockIfNotAlreadyPrinted();

    CSFLogDebugS(logTag, "F:");
    printCurrentThreadID();
    CSFLogDebug(logTag, "%*s %s ", EVENT_NAME_FIELD_WIDTH, device_event_getname(deviceEvent), device->toString().c_str());
}

void CC_CapsPrinter::onLineEvent (ccapi_line_event_e lineEvent, CC_LinePtr line, CC_LineInfoPtr info)
{
	base::AutoLock lock(eventPrinterMutex);
    printHeaderBlockIfNotAlreadyPrinted();

    CSFLogDebugS(logTag, "L:");
    printCurrentThreadID();
    CSFLogDebug(logTag, "%*s %s ", EVENT_NAME_FIELD_WIDTH, line_event_getname(lineEvent), line->toString().c_str());

    switch (lineEvent)
    {
    case CCAPI_LINE_EV_REG_STATE:
        PRINT_EVENT_INFO_NICELY_FOR_BOOL("RegState", info, getRegState);

        if (info->getRegState())
        {
            PRINT_OFFSET_1;
            CSFLogDebugS(logTag, "(LineCaps ");
            CSFLogDebugS(logTag, "----------");

            printLineCapabilities(info, WRAPPED_LINE_ALIGNMENT_OFFSET_2);
            PRINT_OFFSET_1;
            CSFLogDebugS(logTag, "(Line DN#: ");
            CSFLogDebugS(logTag, "----------");
            CSFLogDebug(logTag, "%s", info->getNumber().c_str());
        }
        break;
    case CCAPI_LINE_EV_CAPSET_CHANGED:
        printLineCapabilities(info, WRAPPED_LINE_ALIGNMENT_OFFSET_1);
        break;
    case CCAPI_LINE_EV_CFWDALL:
        PRINT_EVENT_INFO_NICELY_FOR_BOOL("CFWDActive", info, isCFWDActive);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_STRING("CFWDName", info, getCFWDName);
        break;
    case CCAPI_LINE_EV_MWI:
        PRINT_EVENT_INFO_NICELY_FOR_BOOL("MWIStatus", info, getMWIStatus);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_INT32("MWIType", info, getMWIType);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_INT32("MWINewMsgCount", info, getMWINewMsgCount);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_INT32("MWIOldMsgCount", info, getMWIOldMsgCount);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_INT32("MWIPrioNewMsgCount", info, getMWIPrioNewMsgCount);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_FOR_INT32("MWIPrioOldMsgCount", info, getMWIPrioOldMsgCount);
        break;
    default:
    	break;
    }//end switch
}

void printCallSummary(CC_DevicePtr devicePtr)
{
    if (devicePtr == NULL)
    {
        return;
    }

    CC_DeviceInfoPtr deviceInfoPtr = devicePtr->getDeviceInfo();

    if (deviceInfoPtr == NULL)
    {
        return;
    }

    vector<CC_CallPtr> calls = deviceInfoPtr->getCalls();

    CSFLogDebugS(logTag, " List of all calls ");
    for (vector<CC_CallPtr>::iterator it = calls.begin(); it != calls.end(); it++)
    {
        CC_CallPtr call = *it;

        if (call==NULL)
        {
            continue;
        }

        CC_CallInfoPtr callInfoPtr = call->getCallInfo();

        if (callInfoPtr==NULL)
        {
            continue;
		}
        bool audioMute = callInfoPtr->isAudioMuted();
        bool videoMute = callInfoPtr->isVideoMuted();
        
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallState", callInfoPtr, getCallState, call_state_getname);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("CallType", callInfoPtr, getCallType, call_type_getname);
        PRINT_OFFSET_1;
        PRINT_EVENT_INFO_NICELY_USING_GETNAME("VideoDirection", callInfoPtr, getVideoDirection, sdp_direction_getname);
        PRINT_OFFSET_1;
        CSFLogDebugS(logTag, "(AudioMute ");
        CSFLogDebugS(logTag, "----------");
        CSFLogDebug(logTag, " %s )", (audioMute) ? "true" : "false");
        PRINT_OFFSET_1;
        CSFLogDebugS(logTag, "(VideoMute ");
        CSFLogDebugS(logTag, "----------");
        CSFLogDebug(logTag, " %s )", (videoMute) ? "true" : "false");
        PRINT_OFFSET_1;
        printCallPartiesInfo(callInfoPtr);
        PRINT_OFFSET_1;
        CSFLogDebugS(logTag, "(CallCaps ");
        CSFLogDebugS(logTag, "----------");
        printCallCapabilities(callInfoPtr, WRAPPED_LINE_ALIGNMENT_OFFSET_2);

    }//end for (calls)
    CSFLogDebugS(logTag, "End of List");
}


