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

#ifndef __SIP_CSPS_TRANSPORT_H__
#define __SIP_CSPS_TRANSPORT_H__

#include "cpr_types.h"
#include "phone_types.h"
#include "phone_debug.h"
#include "cfgfile_utils.h"
#include "configmgr.h"
#include "ccsip_protocol.h"
#include "ccsip_pmh.h"
#include "ccsip_platform_timers.h"
#include "ccsip_platform_udp.h"
#include "ccsip_messaging.h"

/*
 * Defines for Primary, Secondary and Tertiary CC
 */
typedef enum {
    PRIMARY_CSPS = 0,
    MAX_CSPS
} CSPS_ID;

//extern csps_config_info_t CSPS_Config_Table;

cpr_socket_t sipTransportCSPSGetProxyHandleByDN(line_t dn);
short    sipTransportCSPSGetProxyPortByDN(line_t dn);
uint16_t sipTransportCSPSGetProxyAddressByDN(cpr_ip_addr_t *pip_addr,
                                             line_t dn);

uint16_t sip_config_get_proxy_port(line_t line);
uint16_t sip_config_get_backup_proxy_port(void);
void     sip_config_get_proxy_addr(line_t line, char *buffer, int buffer_len);
uint16_t sip_config_get_backup_proxy_addr(cpr_ip_addr_t *IPAddress,
                                           char *buffer, int buffer_len);

extern sipPlatformUITimer_t sipPlatformUISMTimers[];
extern ccsipGlobInfo_t gGlobInfo;

extern int dns_error_code; // DNS errror code global
void sipTransportCSPSClearProxyHandle(cpr_ip_addr_t *ipaddr, uint16_t port,
                                      cpr_socket_t this_fd);

#endif /* __SIP_CSPS_TRANSPORT_H__ */
