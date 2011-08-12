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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "cpr_types.h"
#include "cpr_timers.h"
#include "cfgfile_utils.h"
#include "configmgr.h"
#include "prot_configmgr.h"
#include "phone.h"

/*
 * List of timers that the CFG task is responsible for.
 * CPR will send a msg to the CFG task when these
 * timers expire. CPR expects a timer id when the timer
 * is created, this enum serves that purpose.
 */
typedef enum {
    CFG_TFTP_RETRY_TIMER
} cfgTimerList_t;

#define MAX_REQ_FIELDS          20
#define MAX_FIELD_NAME_SIZE     30
#define MAX_FIELD_VALUE_SIZE    30
#define MAX_FIELDDESC_STR       (MAX_FIELD_VALUE_SIZE+2)
#define MAX_TFTP_PATH_LEN       64

/*
 * Definition of bits used in pDHCPInfo->extFields.ext.appStatus (32bits).
 * Please note that some of these flags correspond to status flags
 * asserted by Little App.  Please see little_app.c, and make sure these
 * flags are kept consistent.
 */

#define APPSTATUS_UPGRADE_FAILED  0x80000000L

/* maximum length of config file including comments */
#define MAX_CFG_FILE_LENGTH 0x2000

/* minimum length of a registrion in seconds */
#define MIN_REGISTRATION_PERIOD  20

#define BUF_TEST();

typedef enum {
    ERASE_PROGRAM,
    ERASE_ONLY,
    PROGRAM_ONLY
} flash_mode_t;

extern var_t prot_cfg_table[];
extern cfg_rom_t *const prot_startup_config;
extern cfg_rom_t *const prot_running_config;
extern cfg_rom_t *const prot_temp_config;
extern int config_commit(int);
extern int prot_sanity_check_config_settings(void);
extern boolean prot_option_allowed_in_cfg_file(const char *);
extern void prot_shutdown(void);
extern int prot_config_change_notify(int);
extern void prot_disconnected(int);
extern int FlashaProgram(uint16_t *Source, uint16_t *Dest, uint32_t Size,
                         flash_mode_t mode);

extern uint16_t g_NewVlan;
extern uint16_t g_NewRelease;
extern uint16_t g_NewErase;
extern char local_media_type_string[];
extern char local_net_dev_type_string[];

void CFGTftpTimeout(int /*cpr_timer_t*/ *tmr);
int CFGProgramFlash(uint8_t *src, uint8_t *dst, uint32_t len);
int CFGEraseFlash(uint8_t *src, uint8_t *dst, uint32_t len);
int CFGEraseProgramFlash(uint8_t *src, uint8_t *dst, uint32_t len);
int CFGChksum(uint8_t *ptr, uint32_t len, uint32_t *csum, int dspmode,
              int cmpmode);
int CFGGetUISettings(uint8_t *pData);
int CFGSetUISettings(void);
void CFGSetLockState(boolean);
boolean CFGIsLocked(void);
void cfg_set_running_config(void);
void cfg_get_stored_prot_settings(void);
void LoadTempConfigData(void);
void config_handle_cdp(int);
void cfg_sanity_check_media_range(void);
void cfg_check_la_appStatus(unsigned long appStatus);
void cfg_set_inhibitLoading(int yesno);
int cfg_get_inhibitLoading(void);

#endif /* _CONFIG_H_ */
