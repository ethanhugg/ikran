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

#include "cpr_stdio.h"
#include "ccapi_call.h"
#include "sessionHash.h"
#include "CCProvider.h"
#include "cc_call_feature.h"
#include "cc_info.h"
#include "lsm.h"
#include "prot_configmgr.h"
#include "ccapi_call_info.h"
#include "util_string.h"

/**
 * Get call info snapshot
 * @param [in] handle - call handle
 * @return cc_call_info_snap_t
 */
cc_callinfo_ref_t CCAPI_Call_getCallInfo(cc_call_handle_t handle) {
   unsigned int session_id = ccpro_get_sessionId_by_callid(GET_CALL_ID(handle));
   cc_callinfo_ref_t snapshot=NULL;
   session_data_t * data;

   if ( session_id != 0 ) {
      data = findhash(session_id);
      if ( data != NULL ) {
        snapshot = getDeepCopyOfSessionData(data);
        if (snapshot == NULL) {
            return NULL;
        }
        snapshot->ref_count = 1;
      }
   }
   return snapshot;
}
/**
 * Retain the snapshot
 * @param cc_callinfo_ref_t - refrence to the block to be retained
 * @return void
 */
void CCAPI_Call_retainCallInfo(cc_callinfo_ref_t ref) {
    if (ref != NULL ) {
        ref->ref_count++;
    }
}
/**
 * Free the snapshot
 * @param cc_callinfo_ref_t - refrence to the block to be freed
 * @return void
 */
void CCAPI_Call_releaseCallInfo(cc_callinfo_ref_t ref) {
    if (ref != NULL ) {
	DEF_DEBUG(DEB_F_PREFIX"ref=0x%x: count=%d", 
           DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_releaseCallInfo"), ref, ref->ref_count);
	ref->ref_count--;
	if ( ref->ref_count == 0 ) {
            cleanSessionData(ref);
            cpr_free(ref);
	}
    }
}

/**
 * get the line associated with this call
 * @param [in] handle - call handle
 * @return cc_lineid_t
 */
cc_lineid_t CCAPI_Call_getLine(cc_call_handle_t call_handle){
	static const char *fname="CCAPI_Call_getLine";

	if ( call_handle != 0 ) {
    	cc_lineid_t lineid = GET_LINE_ID(call_handle);
    	CCAPP_DEBUG(DEB_F_PREFIX"returned %u\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), lineid);
    	return lineid;
	}
	return 0;
}

/**
 * Originate call 
 * Goes offhook and dials digits if specified
 * @param [in] handle - call handle
 * @param [in] video_pref - video direction desired on call
 * @param [in] digits - digits to be dialed
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_originateCall(cc_call_handle_t handle, cc_sdp_direction_t video_pref, cc_string_t digits, char* ipaddress, int audioPort, int videoPort){
	
	int roapproxy = 0;
	config_get_value(CFGID_ROAPPROXY, &roapproxy, sizeof(roapproxy));
	
	if (roapproxy == TRUE)	{
		init_empty_str(gROAPSDP.offerAddress);
		init_empty_str(gROAPSDP.answerSDP);
		init_empty_str(gROAPSDP.offerSDP);
		strcpy(gROAPSDP.offerAddress, ipaddress);
		gROAPSDP.audioPort = audioPort;
		gROAPSDP.videoPort = videoPort;
	}
	
	return CC_CallFeature_dial(handle, video_pref, digits);
}

/**
 * Dial digits on the call
 * @param [in] handle - call handle
 * @paraqm [in] digits - digits to be dialed
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_sendDigit(cc_call_handle_t handle, cc_digit_t digit){
   return CC_CallFeature_sendDigit(handle, digit);
}

/**
 * Send Backspace
 * @param [in] handle - call handle
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_backspace(cc_call_handle_t handle){
   return CC_CallFeature_backSpace(handle);
}

/**
 * Answer Call
 * @param [in] handle - call handle
 * @param [in] video_pref - video direction desired on call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_answerCall(cc_call_handle_t handle, cc_sdp_direction_t video_pref) {
  return CC_CallFeature_answerCall(handle, video_pref);
}

/**
 * Redial
 * @param [in] handle - call handle
 * @param [in] video_pref - video direction desired on call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_redial(cc_call_handle_t handle, cc_sdp_direction_t video_pref){
  return CC_CallFeature_redial(handle, video_pref);
}

/**
 * Initiate Call Forward All 
 * @param [in] handle - call handle
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_initiateCallForwardAll(cc_call_handle_t handle){
  return CC_CallFeature_callForwardAll(handle);
}
/**
 * Hold
 * @param [in] handle - call handle
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_hold(cc_call_handle_t handle, cc_hold_reason_t reason){
  return CC_CallFeature_holdCall(handle, reason);
}

/**
 * Resume
 * @param [in] handle - call handle
 * @param [in] video_pref - video direction desired on call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_resume(cc_call_handle_t handle, cc_sdp_direction_t video_pref) {
  return CC_CallFeature_resume(handle, video_pref);
}

/**
 * end Consult leg
 * @param [in] handle - call handle
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_endConsultativeCall(cc_call_handle_t handle){
  cc_callinfo_ref_t info_handle = CCAPI_Call_getCallInfo(handle);
  cc_call_attr_t attr = CCAPI_CallInfo_getCallAttr(info_handle);
  if (attr != CC_ATTR_CONF_CONSULT &&
    attr != CC_ATTR_XFR_CONSULT &&
    attr != CC_ATTR_LOCAL_CONF_CONSULT &&
    attr != CC_ATTR_LOCAL_XFER_CONSULT) {
    DEF_DEBUG(DEB_F_PREFIX"This method only calls on a consultative call",
      DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_endConsultativeCall"), handle);
    return CC_FAILURE;
  }

  return CC_CallFeature_endConsultativeCall(handle);
}

/**
 * end Call
 * @param [in] handle - call handle
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_endCall(cc_call_handle_t handle){
  return CC_CallFeature_terminateCall(handle);
}

/**
 * Initiate a conference
 * @param [in] handle - call handle
 * @param [in] video_pref - video direction desired on consult call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_conferenceStart(cc_call_handle_t handle, cc_sdp_direction_t video_pref){
  return CC_CallFeature_conference(handle, TRUE,//not used
                  CC_EMPTY_CALL_HANDLE, video_pref);
}

/**
 * complete conference
 * @param [in] handle - call handle
 * @param [in] phandle - call handle of the other leg
 * @param [in] video_pref - video direction desired on consult call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_conferenceComplete(cc_call_handle_t handle, cc_call_handle_t phandle,
                          cc_sdp_direction_t video_pref){
  return CC_CallFeature_conference(handle, TRUE,//not used
                  phandle, video_pref);

}

/**
 * start transfer
 * @param [in] handle - call handle
 * @param [in] video_pref - video direction desired on consult call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_transferStart(cc_call_handle_t handle, cc_sdp_direction_t video_pref){
  return CC_CallFeature_transfer(handle, CC_EMPTY_CALL_HANDLE, video_pref);
}

/**
 * complete transfer
 * @param [in] handle - call handle
 * @param [in] phandle - call handle of the other leg
 * @param [in] video_pref - video direction desired on consult call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_transferComplete(cc_call_handle_t handle, cc_call_handle_t phandle, 
                              cc_sdp_direction_t video_pref){
  return CC_CallFeature_transfer(handle, phandle, video_pref);
}

/**
 * cancel conference or transfer
 * @param [in] handle - call handle
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_cancelTransferOrConferenceFeature(cc_call_handle_t handle){
  return CC_CallFeature_cancelXfrerCnf(handle);
}

/**
 * direct Transfer
 * @param [in] handle - call handle
 * @param [in] handle - transfer target call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_directTransfer(cc_call_handle_t handle, cc_call_handle_t target){
  return CC_CallFeature_directTransfer(handle, target);
}

/**
 * Join Across line
 * @param [in] handle - call handle
 * @param [in] handle - join target
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_joinAcrossLine(cc_call_handle_t handle, cc_call_handle_t target){
  return CC_CallFeature_joinAcrossLine(handle, target);
}

/**
 * BLF Call Pickup
 * @param [in] handle - call handle
 * @param [in] speed - speedDial Number
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_blfCallPickup(cc_call_handle_t handle, 
                  cc_sdp_direction_t video_pref, cc_string_t speed){
  return CC_CallFeature_blfCallPickup(handle, video_pref, speed);
}

/**
 * Select a call
 * @param [in] handle - call handle
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_select(cc_call_handle_t handle){
  return CC_CallFeature_select(handle);
}

/**
 * Update Video Media Cap for the call
 * @param [in] handle - call handle
 * @param [in] video_pref - video direction desired on call
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_updateVideoMediaCap (cc_call_handle_t handle, cc_sdp_direction_t video_pref) {
  return CC_CallFeature_updateCallMediaCapability(handle, video_pref);
}

/**
 * send INFO method for the call
 * @param [in] handle - call handle
 * @param [in] infopackage - Info-Package header value
 * @param [in] infotype - Content-Type header val
 * @param [in] infobody - Body of the INFO message
 * @return SUCCESS or FAILURE
 */
cc_return_t CCAPI_Call_sendInfo (cc_call_handle_t handle, cc_string_t infopackage, cc_string_t infotype, cc_string_t infobody)
{
	CC_Info_sendInfo(handle, infopackage, infotype, infobody);
        return CC_SUCCESS;
}

/**
 * API to mute/unmute audio
 * @param [in] val - TRUE=> mute FALSE => unmute
 * @return SUCCESS or FAILURE
 * NOTE: The mute state is persisted within the stack and shall be remembered across hold/resume.
 * This API doesn't perform the mute operation but simply caches the mute state of the session.
 */
cc_return_t CCAPI_Call_setAudioMute (cc_call_handle_t handle, cc_boolean val) {
	unsigned int session_id = ccpro_get_sessionId_by_callid(GET_CALL_ID(handle));
        session_data_t * sess_data_p = (session_data_t *)findhash(session_id);
	DEF_DEBUG(DEB_F_PREFIX": val=%d, handle=%d datap=%x", 
           DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_setAudioMute"), val, handle, sess_data_p);
	if ( sess_data_p != NULL ) {
		sess_data_p->audio_mute = val;
	}
        return CC_SUCCESS;
}

/**
 * API to mute/unmute Video
 * @param [in] val - TRUE=> mute FALSE => unmute
 * @return SUCCESS or FAILURE
 * NOTE: The mute state is persisted within the stack and shall be remembered across hold/resume
 * This API doesn't perform the mute operation but simply caches the mute state of the session.
 */
cc_return_t CCAPI_Call_setVideoMute (cc_call_handle_t handle, cc_boolean val){
	unsigned int session_id = ccpro_get_sessionId_by_callid(GET_CALL_ID(handle));
        session_data_t * sess_data_p = (session_data_t *)findhash(session_id);
	DEF_DEBUG(DEB_F_PREFIX": val=%d, handle=%d datap=%x", 
           DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_setVideoMute"), val, handle, sess_data_p);
	if ( sess_data_p != NULL ) {
		sess_data_p->video_mute = val;
		lsm_set_video_mute(GET_CALL_ID(handle), val);
	}
        return CC_SUCCESS;
}
