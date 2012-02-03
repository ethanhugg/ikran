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

#include "cc_device_feature.h"
#include "sessionConstants.h"
#include "sessionTypes.h"
#include "CCProvider.h"
#include "phone_debug.h"
#include "ccapp_task.h"

/**
 * Internal method
 */
void cc_invokeDeviceFeature(session_feature_t *feature) {

    if (ccappTaskPostMsg(CCAPP_INVOKEPROVIDER_FEATURE, feature,
                       sizeof(session_feature_t), CCAPP_CCPROVIER) == CPR_FAILURE) {
        CCAPP_DEBUG(DEB_F_PREFIX"cc_invokeDeviceFeature failed\n",
                DEB_F_PREFIX_ARGS("cc_device_feature", "cc_invokeDeviceFeature"));
    }

}

void CC_DeviceFeature_supportsVideo(boolean enable) {
    session_feature_t feat;

    feat.session_id = (SESSIONTYPE_CALLCONTROL << CC_SID_TYPE_SHIFT);
    feat.featureID = DEVICE_SUPPORTS_NATIVE_VIDEO;
    feat.featData.ccData.info = NULL;
    feat.featData.ccData.info1 = NULL;
    feat.featData.ccData.state = enable;
    cc_invokeDeviceFeature(&feat);
}

/**
 * Enable video/camera.
 * @param enable true or false
 * @return void
 */
void CC_DeviceFeature_enableVideo(boolean enable) {
    session_feature_t feat;

    feat.session_id = (SESSIONTYPE_CALLCONTROL << CC_SID_TYPE_SHIFT);
    feat.featureID = DEVICE_ENABLE_VIDEO;
    feat.featData.ccData.info = NULL;
    feat.featData.ccData.info1 = NULL;
    feat.featData.ccData.state = enable;
    cc_invokeDeviceFeature(&feat);
}

void CC_DeviceFeature_enableCamera(boolean enable) {
    session_feature_t feat;

    feat.session_id = (SESSIONTYPE_CALLCONTROL << CC_SID_TYPE_SHIFT);
    feat.featureID = DEVICE_ENABLE_CAMERA;
    feat.featData.ccData.info = NULL;
    feat.featData.ccData.info1 = NULL;
    feat.featData.ccData.state = enable;
    cc_invokeDeviceFeature(&feat);
}

