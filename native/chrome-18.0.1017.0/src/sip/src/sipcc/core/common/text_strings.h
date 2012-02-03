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

#ifndef TEXT_STRINGS_H
#define TEXT_STRINGS_H

#include "cpr_types.h"
#include "phone_types.h"
#include "string_lib.h"


#define DEF_NOTIFY_PRI    20
#define HR_NOTIFY_PRI     1

/*
 * Define hard coded Anonymous string to be used in From header
 * when call id blocking is enabled. This is also used to
 * compare to when anonymous call block is enabled. We can
 * can not use the localized version of Anonymous for these 
 * actions.
 */
#define SIP_HEADER_ANONYMOUS_STR        "Anonymous"

/*
 * Define special strings that are not localized to be used for
 * comparing special display names whether the names are special
 * and not to display the number associated with the name.
 */
#define CONFERENCE_STR                  "conference"   
#define CONFERENCE_STR_LEN              (sizeof(CONFERENCE_STR)-1)
#define CONFERENCE_LOCALE_CODE			 964
/*
 * Constants for dictionary index
 */

// Hardcoding phrases that need to be dropped on floor for RT to 1

#define STR_INDEX_GRP_CALL_PICKUP       47
#define STR_INDEX_LST_CALL_PICKUP       49
#define STR_INDEX_FEAT_UNAVAIL          148
#define STR_INDEX_CNFR_FAIL_NOCODEC     1054
#define STR_INDEX_REORDER               1055
#define STR_INDEX_ANONYMOUS_SPACE       1056
#define STR_INDEX_NO_FREE_LINES         1057 
#define STR_INDEX_REGISTRATION_REJECTED 1058
#define STR_INDEX_REGISTERING           1
#define STR_INDEX_WHISPER               1
#define STR_INDEX_PROXY_UNAVAIL         1
#define STR_INDEX_CALL_REDIRECTED       1
#define STR_INDEX_TRANSFERRING          1
#define STR_INDEX_SESSION_PROGRESS      1
#define STR_INDEX_CALLING               1
#define STR_INDEX_USE_LINE_OR_JOIN_TO_COMPLETE  1
#define STR_INDEX_CONF_LOCAL_MIXED      1
#define STR_INDEX_NO_CALL_FOR_PICKUP    195

#define STR_INDEX_CALL_FORWARD          105
#define STR_INDEX_CALL_PICKUP           117
#define STR_INDEX_NO_LINE_FOR_PICKUP    198


#define STR_INDEX_ERROR_PASS_LIMIT  149
#define STR_INDEX_TRANS_COMPLETE    918
#define STR_INDEX_END_CALL          877
#define STR_INDEX_NO_BAND_WIDTH     1158

#define STR_INDEX_RESP_TIMEOUT      1160

#define CALL_BUBBLE_STR_MAX_LEN     32

#define STATUS_LINE_MAX_LEN         128

/*
 * Escape codes definitions
 */
#define OLD_CUCM_DICTIONARY_ESCAPE_TAG 		 '\x80'
#define NEW_CUCM_DICTIONARY_ESCAPE_TAG 		 '\x1E'
#define CALL_CONTROL_PHRASE_OFFSET 			 '\x64'  // offset 100
#define MAX_LOCALE_PHRASE_LEN				  256
#define MAX_LOCALE_STRING_LEN 				  1024

/*
 * Escaped index string codes for Call Mgr dictionary phrases
 */
#define INDEX_STR_KEY_NOT_ACTIVE    (char *) "\x80\x2D"
#define INDEX_STR_BARGE             (char *) "\x80\x43"
#define INDEX_STR_PRIVATE           (char *) "\x80\x36"
#define INDEX_STR_HOLD_REVERSION    (char *) "\x1E\x23"
#define INDEX_STR_MONITORING        (char *) "\x1E\x27"
#define INDEX_STR_COACHING          (char *) "\x1E\x46"


/*
 * Index value for phrase strings subject to localization
 */
enum PHRASE_STRINGS_ENUM {
    LOCALE_START,
    IDLE_PROMPT,
    ANONYMOUS,                  /* fsmdef.o */
    CALL_PROCEEDING_IN,
    CALL_PROCEEDING_OUT,
    CALL_ALERTING,
    CALL_ALERTING_SECONDARY,
    CALL_ALERTING_LOCAL,
    CALL_CONNECTED,
    CALL_INITIATE_HOLD,
    PROMPT_DIAL,
    LINE_BUSY,
    CALL_WAITING,
    TRANSFER_FAILED,
    CONF_CANNOT_COMPLETE,
    UI_CONFERENCE,
    UI_UNKNOWN,
    REMOTE_IN_USE,
    NUM_NOT_CONFIGURED,
    UI_FROM,
    INVALID_CONF_PARTICIPANT,
    UI_PRIVATE,
    LOCALE_END
};

/*
 * Index value for Debug strings NOT subject to localization
 */
enum DEBUG_STRINGS_ENUM {
    DEBUG_START,
    DEBUG_SEPARATOR_BAR,
    DEBUG_CONSOLE_PASSWORD,
    DEBUG_CONSOLE_KEYWORD_CONSOLE_STALL,
    DEBUG_CONSOLE_KEYWORD_MEMORYMAP,
    DEBUG_CONSOLE_KEYWORD_MALLOCTABLE,
    DEBUG_CONSOLE_KEYWORD_MEMORYDUMP,
    DEBUG_CONSOLE_KEYWORD_DNS,
    DEBUG_CONSOLE_KEYWORD_DSPSTATE,
    DEBUG_CONSOLE_USAGE_MEMORYDUMP,
    DEBUG_CONSOLE_BREAK,

    DEBUG_FUNCTION_ENTRY,
    DEBUG_FUNCTION_ENTRY2,
    DEBUG_SIP_ENTRY,
    DEBUG_SIP_URL_ERROR,
    DEBUG_LINE_NUMBER_INVALID,
    DEBUG_SIP_SPI_SEND_ERROR,
    DEBUG_SIP_SDP_CREATE_BUF_ERROR,
    DEBUG_SIP_PARSE_SDP_ERROR,
    DEBUG_SIP_FEATURE_UNSUPPORTED,
    DEBUG_SIP_DEST_SDP,
    DEBUG_SIP_MSG_SENDING_REQUEST,
    DEBUG_SIP_MSG_SENDING_RESPONSE,
    DEBUG_SIP_MSG_RECV,
    DEBUG_SIP_STATE_UNCHANGED,
    DEBUG_SIP_FUNCTIONCALL_FAILED,
    DEBUG_SIP_BUILDFLAG_ERROR,
    DEBUG_GENERAL_FUNCTIONCALL_FAILED,
    DEBUG_GENERAL_SYSTEMCALL_FAILED,
    DEBUG_GENERAL_FUNCTIONCALL_BADARGUMENT,
    DEBUG_FUNCTIONNAME_SIPPMH_PARSE_FROM,
    DEBUG_FUNCTIONNAME_SIPPMH_PARSE_TO,
    DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE,

    DEBUG_SNTP_LI_ERROR,
    DEBUG_SNTP_MODE_ERROR,
    DEBUG_SNTP_STRATUM_ERROR,
    DEBUG_SNTP_TIMESTAMP_ERROR,
    DEBUG_SNTP_TIMESTAMP1,
    DEBUG_SNTP_TIMESTAMP2,
    DEBUG_SNTP_TIME_UPDATE,
    DEBUG_SNTP_TS_HEADER,
    DEBUG_SNTP_TS_PRINT,
    DEBUG_SNTP_SOCKET_REOPEN,
    DEBUG_SNTP_DISABLED,
    DEBUG_SNTP_REQUEST,
    DEBUG_SNTP_RESPONSE,
    DEBUG_SNTP_RETRANSMIT,
    DEBUG_SNTP_UNICAST_MODE,
    DEBUG_SNTP_MULTICAST_MODE,
    DEBUG_SNTP_ANYCAST_MODE,
    DEBUG_SNTP_VALIDATION,
    DEBUG_SNTP_VALIDATION_PACKET,
    DEBUG_SNTP_WRONG_SERVER,
    DEBUG_SNTP_NO_REQUEST,
    DEBUG_SNTP_ANYCAST_RESET,
    DEBUG_SOCKET_UDP_RTP,
    DEBUG_MAC_PRINT,
    DEBUG_IP_PRINT,
    DEBUG_SYSBUF_UNAVAILABLE,
    DEBUG_MSG_BUFFER_TOO_BIG,
    DEBUG_UNKNOWN_TIMER_BLOCK,
    DEBUG_CREDENTIALS_BAG_CORRUPTED,
    DEBUG_INPUT_EMPTY,
    DEBUG_INPUT_NULL,
    DEBUG_STRING_DUP_FAILED,
    DEBUG_PARSER_STRING_TOO_LARGE,
    DEBUG_PARSER_NULL_KEY_TABLE,
    DEBUG_PARSER_UNKNOWN_KEY,
    DEBUG_PARSER_UNKNOWN_KEY_ENUM,
    DEBUG_PARSER_INVALID_START_VAR,
    DEBUG_PARSER_INVALID_VAR_CHAR,
    DEBUG_PARSER_MISSING_COLON,
    DEBUG_PARSER_NO_VALUE,
    DEBUG_PARSER_EARLY_EOL,
    DEBUG_PARSER_INVALID_VAR_NAME,
    DEBUG_PARSER_INVALID_VAR_VALUE,
    DEBUG_PARSER_UNKNOWN_VAR,
    DEBUG_PARSER_NAME_VALUE,
    DEBUG_PARSER_UNKNOWN_NAME_VALUE,
    DEBUG_PARSER_UNKNOWN_ERROR,
    DEBUG_PARSER_NUM_ERRORS,
    DEBUG_PARSER_SET_DEFAULT,
    DEBUG_SDP_ERROR_BODY_FIELD,
    DEBUG_UDP_OPEN_FAIL,
    DEBUG_UDP_PAYLOAD_TOO_LARGE,
    DEBUG_TCP_PAYLOAD_TOO_LARGE = DEBUG_UDP_PAYLOAD_TOO_LARGE,
    DEBUG_RTP_TRANSPORT,
    DEBUG_RTP_INVALID_VOIP_TYPE,
    DEBUG_RTP_INVALID_RTP_TYPE,
    DEBUG_MEMORY_ALLOC,
    DEBUG_MEMORY_FREE,
    DEBUG_MEMORY_MALLOC_ERROR,
    DEBUG_MEMORY_REALLOC_ERROR,
    DEBUG_MEMORY_OUT_OF_MEM,
    DEBUG_MEMORY_ENTRY,
    DEBUG_MEMORY_SUMMARY,
    DEBUG_MEMORY_ADDRESS_HEADER,
    DEBUG_MEMORY_DUMP,
    DEBUG_DNS_GETHOSTBYNAME,
    DEBUG_PMH_INCORRECT_SYNTAX,
    DEBUG_PMH_INVALID_FIELD_VALUE,
    DEBUG_PMH_INVALID_SCHEME,
    DEBUG_PMH_UNKNOWN_SCHEME,
    DEBUG_PMH_NOT_ENOUGH_PARAMETERS,
    DEBUG_REG_DISABLED,
    DEBUG_REG_PROXY_EXPIRES,
    DEBUG_REG_SIP_DATE,
    DEBUG_REG_SIP_RESP_CODE,
    DEBUG_REG_SIP_RESP_FAILURE,
    DEBUG_REG_INVALID_LINE,
    CC_NO_MSG_BUFFER,
    CC_SEND_FAILURE,
    GSM_UNDEFINED,
    GSM_DBG_PTR,
    GSM_FUNC_ENTER,
    GSM_DBG1,
    FSM_DBG_SM_DEFAULT_EVENT,
    FSM_DBG_SM_FTR_ENTRY,
    FSM_DBG_FAC_ERR,
    FSM_DBG_FAC_FOUND,
    FSM_DBG_IGNORE_FTR,
    FSM_DBG_IGNORE_SRC,
    FSM_DBG_CHANGE_STATE,
    FSM_DBG_SDP_BUILD_ERR,
    FSMDEF_DBG_PTR,
    FSMDEF_DBG1,
    FSMDEF_DBG2,
    FSMDEF_DBG_SDP,
    FSMDEF_DBG_CLR_SPOOF_APPLD,
    FSMDEF_DBG_CLR_SPOOF_RQSTD,
    FSMDEF_DBG_INVALID_DCB,
    FSMDEF_DBG_FTR_REQ_ACT,
    FSMDEF_DBG_TMR_CREATE_FAILED,
    FSMDEF_DBG_TMR_START_FAILED,
    FSMDEF_DBG_TMR_CANCEL_FAILED,
    FSMXFR_DBG_XFR_INITIATED,
    FSMXFR_DBG_PTR,
    FSMCNF_DBG_CNF_INITIATED,
    FSMCNF_DBG_PTR,
    FSMB2BCNF_DBG_CNF_INITIATED,
    FSMB2BCNF_DBG_PTR,
    FSMSHR_DBG_BARGE_INITIATED,
    LSM_DBG_ENTRY,
    LSM_DBG_INT1,
    LSM_DBG_CC_ERROR,
    VCM_DEBUG_ENTRY,
    SM_PROCESS_EVENT_ERROR,
    REG_SM_PROCESS_EVENT_ERROR,
    DEBUG_END
};


typedef struct {
    const char *text;
} debug_string_table_entry;

extern debug_string_table_entry debug_string_table[];

#define get_debug_string(index)               ((char*)debug_string_table[(index)].text)



typedef struct {
    const char *index_str;
} tnp_phrase_index_str_table_entry;


extern tnp_phrase_index_str_table_entry tnp_phrase_index_str_table[];

#define platform_get_phrase_index_str(index)  ((char*)tnp_phrase_index_str_table[(index)].index_str)
#define get_info_string(index) "NotImplemented"


#endif
