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

#include "cpr_in.h"
#include "cpr_rand.h"
#include "cpr_stdlib.h"
#include "lsm.h"
#include "fsm.h"
#include "ccapi.h"
#include "ccsip_sdp.h"
#include "sdp.h"
#include "gsm.h"
#include "gsm_sdp.h"
#include "util_string.h"
#include "rtp_defs.h"
#include "debug.h"
#include "dtmf.h"
#include "prot_configmgr.h"
#include "dns_utils.h"
#include "sip_interface_regmgr.h"
#include "platform_api.h"
#include "vcm.h"

//TODO Need to place this in a portable location
#define MULTICAST_START_ADDRESS 0xe1000000
#define MULTICAST_END_ADDRESS   0xefffffff

/* Only first octet contains codec type */
#define GET_CODEC_TYPE(a)    ((uint8_t)((a) & 0XFF))

#define GSMSDP_SET_MEDIA_DIABLE(media) \
     (media->src_port = 0) 

#define CAST_DEFAULT_BITRATE 320000
/*
 * The maximum number of media lines per call. This puts the upper limit
 * on * the maximum number of media lines per call to resource hogging.
 * The value of 8 is intended up to 2 audio and 2 video streams with 
 * each stream can offer IPV4 and IPV6 alternate network address type
 * in ANAT group (RFC-4091). 
 */
#define GSMSDP_MAX_MLINES_PER_CALL  (8)

/*
 * Permanent number of free media structure elments for media structure
 * that represents media line in the SDP. The maximum number of elements
 * is set to equal number of call or LSM_MAX_CALLS. This should be enough
 * to minimumly allow typical a single audio media stream per call scenario
 * without using dynamic memory.
 *
 * If more media structures are needed than this number, the addition
 * media structures are allocated from heap and they will be freed back
 * from heap after thehy are not used. The only time where the heap
 * is used when phone reaches the maximum call capacity and each one
 * of the call is using more than one media lines. 
 */
#define GSMSDP_PERM_MEDIA_ELEMS   (LSM_MAX_CALLS)

/*
 * The permanent free media structure elements use static array. 
 * It is to ensure a low overhead for this a typical single audio call.
 */ 
static fsmdef_media_t gsmsdp_free_media_chunk[GSMSDP_PERM_MEDIA_ELEMS]; 
static sll_lite_list_t gsmsdp_free_media_list;

typedef enum {
    MEDIA_TABLE_GLOBAL,
    MEDIA_TABLE_SESSION
} media_table_e;

/* Forward references */
static cc_causes_t
gsmsdp_init_local_sdp (cc_sdp_t **sdp_pp);

static void
gsmsdp_set_media_capability(fsmdef_media_t *media,
                            const cc_media_cap_t *media_cap);
static fsmdef_media_t *
gsmsdp_add_media_line(fsmdef_dcb_t *dcb_p, const cc_media_cap_t *media_cap,
                      uint8_t cap_index, uint16_t level,
                      cpr_ip_type addr_type);


extern cc_media_cap_table_t g_media_table;

extern boolean g_disable_mass_reg_debug_print; 
/**
 * A wraper function to return the media capability supported by
 * the platform and session. This is a convient place if policy 
 * to get the capability table as it applies to the session
 * updates the media_cap_tbl ptr in dcb
 *
 * @param[in]dcb     - pointer to the fsmdef_dcb_t

 *
 * @return           - pointer to the the media capability table for session
 */
static const cc_media_cap_table_t *gsmsdp_get_media_capability (fsmdef_dcb_t *dcb_p)
{
    static const char *fname = "gsmsdp_get_media_capability";

    if (g_disable_mass_reg_debug_print == FALSE) {
        GSM_DEBUG(DEB_F_PREFIX"dcb video pref %x\n", 
                               DEB_F_PREFIX_ARGS(GSM, fname), dcb_p->video_pref);
    }
    
    
    if ( dcb_p->media_cap_tbl == NULL ) {
         dcb_p->media_cap_tbl = (cc_media_cap_table_t*) cpr_malloc(sizeof(cc_media_cap_table_t));
         if ( dcb_p->media_cap_tbl == NULL ) {
             GSM_ERR_MSG(GSM_L_C_F_PREFIX"media table malloc failed.\n", 
                    dcb_p->line, dcb_p->call_id, fname);
             return NULL;
         }
    } 
 
    *(dcb_p->media_cap_tbl) = g_media_table;

    if ( dcb_p->video_pref == SDP_DIRECTION_INACTIVE) {
     // do not enable video
       dcb_p->media_cap_tbl->cap[CC_VIDEO_1].enabled = FALSE;
    } 

    if ( dcb_p->video_pref == SDP_DIRECTION_RECVONLY ) {
       if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_SENDRECV ) {
           dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = dcb_p->video_pref;
       }
      
       if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_SENDONLY ) {
           dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = SDP_DIRECTION_INACTIVE;
            DEF_DEBUG(GSM_L_C_F_PREFIX"video capability disabled to SDP_DIRECTION_INACTIVE from sendonly\n", 
                dcb_p->line, dcb_p->call_id, fname);
       }
    } else if ( dcb_p->video_pref == SDP_DIRECTION_SENDONLY ) {
       if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_SENDRECV ) {
           dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = dcb_p->video_pref;
       }

       if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_RECVONLY ) {
           dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = SDP_DIRECTION_INACTIVE;
            DEF_DEBUG(GSM_L_C_F_PREFIX"video capability disabled to SDP_DIRECTION_INACTIVE from recvonly\n", 
                dcb_p->line, dcb_p->call_id, fname);
       }
    } // else if requested is SENDRECV just go by capability 


    return (dcb_p->media_cap_tbl);
}


/**
 * The function creates a free media structure elements list. The
 * free list is global for all calls. The function must be called once
 * during GSM initializtion. 
 *
 * @param            None.
 *
 * @return           TRUE  - free media structure list is created
 *                           successfully.
 *                   FALSE - failed to create free media structure 
 *                           list.
 */
boolean
gsmsdp_create_free_media_list (void)
{
    uint32_t i;
    fsmdef_media_t *media;

    /* initialize free media_list structure */
    (void)sll_lite_init(&gsmsdp_free_media_list);

    /*
     * Populate the free list:
     * Break the entire chunk into multiple free elments and link them
     * onto to the free media list.
     */
    media = &gsmsdp_free_media_chunk[0];    /* first element */
    for (i = 0; i < GSMSDP_PERM_MEDIA_ELEMS; i++) {
        (void)sll_lite_link_head(&gsmsdp_free_media_list, 
                                 (sll_lite_node_t *)media);
        media = media + 1; /* next element */
    }

    /* Successful create media free list */
    return (TRUE);
}

/**
 * The function destroys the free media structure list. It should be
 * call during GSM shutdown.
 *
 * @param            None.
 *
 * @return           None.
 */
void 
gsmsdp_destroy_free_media_list (void)
{
    /*
     * Although the free chunk is not allocated but,
     * NULL out the free list header to indicate that the 
     * there is not thing from the free chunk.
     */
    (void)sll_lite_init(&gsmsdp_free_media_list);
}

/**
 * The function allocates a media structure. The function
 * attempts to obtain a free media structure from the free media 
 * structure list first. If free list is empty then the media structure
 * is allocated from a memory pool.
 *
 * @param            None.
 *
 * @return           pointer to the fsmdef_media_t if successful or
 *                   NULL when there is no free media structure 
 *                   is available.
 */
static fsmdef_media_t *
gsmsdp_alloc_media (void)
{
    static const char fname[] = "gsmsdp_alloc_media";
    fsmdef_media_t *media = NULL;

    /* Get a media element from the free list */
    media = (fsmdef_media_t *)sll_lite_unlink_head(&gsmsdp_free_media_list);
    if (media == NULL) {
        /* no free element from cache, allocate it from the pool */
        media = cpr_malloc(sizeof(fsmdef_media_t));
        GSM_DEBUG(DEB_F_PREFIX"get from dynamic pool, media %x\n", 
                               DEB_F_PREFIX_ARGS(GSM, fname), media);
    }
    return (media);
}

/** 
 * The function frees a media structure back to the free list or
 * heap. If the media structure is from the free list then it
 * is put back to the free list otherwise it will be freed
 * back to the dynamic pool.
 *  
 * @param[in]media   - pointer to fsmdef_media_t to free back to
 *                   free list.
 *
 * @return           pointer to the fsmdef_media_t if successful
 *                   NULL when there is no free media structure
 *                   is available.
 */
static void
gsmsdp_free_media (fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_free_media";

    if (media == NULL) {
        return;
    }

    if (media-> video != NULL ) {
      vcmFreeMediaPtr(media->video);
    }
    /*
     * Check to see if the element is part of the
     * free chunk space.
     */
    if ((media >= &gsmsdp_free_media_chunk[0]) &&
        (media <= &gsmsdp_free_media_chunk[GSMSDP_PERM_MEDIA_ELEMS-1])) {
        /* the element is part of free chunk, put it back to the list */
        (void)sll_lite_link_head(&gsmsdp_free_media_list, 
                                 (sll_lite_node_t *)media);
    } else {
        /* this element is from the dynamic pool, free it back */
        cpr_free(media);
        GSM_DEBUG(DEB_F_PREFIX"free media 0x%x to dynamic pool\n", 
                  DEB_F_PREFIX_ARGS(GSM, fname), media);
    }
}

/**
 * Initialize the media entry. The function initializes media
 * entry.
 *
 * @param[in]media - pointer to fsmdef_media_t of the media entry to be
 *                   initialized.
 *
 * @return  none
 *
 * @pre     (media not_eq NULL)
 */
static void
gsmsdp_init_media (fsmdef_media_t *media)
{
    media->refid = CC_NO_MEDIA_REF_ID;
    media->type = SDP_MEDIA_INVALID; /* invalid (free entry) */
    media->packetization_period = 20;
    media->mode = (uint16_t)vcmGetILBCMode();
    media->vad = VCM_VAD_OFF;
    /* Default to audio codec */
    media->payload = RTP_NONE;
    media->level = 0;
    media->dest_port = 0;
    media->dest_addr = ip_addr_invalid;
    media->is_multicast = FALSE;
    media->multicast_port = 0;
    media->avt_payload_type = RTP_NONE;
    media->src_port = 0;
    media->src_addr = ip_addr_invalid;
    media->rcv_chan = FALSE;
    media->xmit_chan = FALSE;

    media->direction = SDP_DIRECTION_INACTIVE;
    media->direction_set = FALSE;
    media->transport = SDP_TRANSPORT_INVALID;
    media->remote_dynamic_payload_type_value = 0;
    media->tias_bw = SDP_INVALID_VALUE;
    media->profile_level = 0;


    media->previous_sdp.avt_payload_type = RTP_NONE;
    media->previous_sdp.dest_addr = ip_addr_invalid;
    media->previous_sdp.dest_port = 0;
    media->previous_sdp.direction = SDP_DIRECTION_INACTIVE;
    media->previous_sdp.packetization_period = media->packetization_period;
    media->previous_sdp.payload_type = media->payload;
    media->previous_sdp.local_payload_type = media->payload;
    media->previous_sdp.tias_bw = SDP_INVALID_VALUE;
    media->previous_sdp.profile_level = 0;
    media->local_dynamic_payload_type_value = media->payload;
    media->hold  = FSM_HOLD_NONE;
    media->flags = 0;                    /* clear all flags      */
    media->cap_index = CC_MAX_MEDIA_CAP; /* max is invalid value */
    media->video = NULL;
}

/**
 *
 * Returns a pointer to a new the fsmdef_media_t for a given dcb.
 * The default media parameters will be intialized for the known or 
 * supported media types. The new media is also added to the media list
 * in the dcb.
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t
 * @param[in]media_type - sdp_media_e.
 * @param[in]level      - uint16_t for media line level.
 *
 * @return           pointer to the fsmdef_media_t of the corresponding
 *                   media entry in the dcb.
 * @pre              (dcb not_eq NULL)
 */
static fsmdef_media_t *
gsmsdp_get_new_media (fsmdef_dcb_t *dcb_p, sdp_media_e media_type,
                      uint16_t level)
{
    static const char fname[] = "gsmsdp_get_new_media";
    fsmdef_media_t *media;
    static media_refid_t media_refid = CC_NO_MEDIA_REF_ID;
    sll_lite_return_e sll_lite_ret;

    /* check to ensue we do not handle too many media lines */
    if (GSMSDP_MEDIA_COUNT(dcb_p) >= GSMSDP_MAX_MLINES_PER_CALL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"exceeding media lines per call\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return (NULL); 
    }

    /* allocate new media entry */
    media = gsmsdp_alloc_media();
    if (media != NULL) {
        /* initialize the media entry */
        gsmsdp_init_media(media);

        /* assigned media reference id */
        if (++media_refid == CC_NO_MEDIA_REF_ID) {
            media_refid = 1;
        }
        media->refid = media_refid;
        media->type  = media_type;
        media->level = level;

        /* append the media to the active list */
        sll_lite_ret = sll_lite_link_tail(&dcb_p->media_list,
                           (sll_lite_node_t *)media);
        if (sll_lite_ret != SLL_LITE_RET_SUCCESS) {
            /* fails to put the new media entry on to the list */
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"error %d when add media to list\n", 
                        dcb_p->line, dcb_p->call_id, fname, sll_lite_ret);
            gsmsdp_free_media(media);
            media = NULL;
        } 
    } 
    return (media);
}

/**
 * The function removes the media entry from the list of a given call and
 * then deallocates the media entry.
 * 
 * @param[in]dcb   - pointer to fsmdef_def_t for the dcb whose
 *                   media to be removed from.
 * @param[in]media - pointer to fsmdef_media_t for the media
 *                   entry to be removed.
 *
 * @return  none
 * 
 * @pre     (dcb not_eq NULL)
 */
static void gsmsdp_remove_media (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_remove_media";
    cc_action_data_t data;

    if (media == NULL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"removing NULL media\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return;
    }

    if (media->rcv_chan || media->xmit_chan) {
        /* stop media, if it is opened */
        data.stop_media.media_refid = media->refid;
        (void)cc_call_action(dcb_p->call_id, dcb_p->line, CC_ACTION_STOP_MEDIA,
                             &data);
    }
    /* remove this media off the list */
    (void)sll_lite_remove(&dcb_p->media_list, (sll_lite_node_t *)media); 

    /* Release the port */
    vcmRxReleasePort(media->cap_index, dcb_p->group_id, media->refid,
                 lsm_get_ms_ui_call_handle(dcb_p->line, dcb_p->call_id, CC_NO_CALL_ID), media->src_port);

    /* free media structure */
    gsmsdp_free_media(media);
}   

/**
 * The function performs cleaning media list of a given call. It walks
 * through the list and deallocates each media entries.
 *
 * @param[in]dcb   - pointer to fsmdef_def_t for the dcb whose
 *                   media list to be cleaned.
 *  
 * @return  none 
 *  
 * @pre     (dcb not_eq NULL)
 */
void gsmsdp_clean_media_list (fsmdef_dcb_t *dcb_p)
{
    fsmdef_media_t *media = NULL; 
 
    while (TRUE) {
        /* unlink head and free the media */
        media = (fsmdef_media_t *)sll_lite_unlink_head(&dcb_p->media_list);
        if (media != NULL) { 
            gsmsdp_free_media(media);
        } else {
            break;
        }
    }
}

/**
 *
 * The function is used for per call media list initializatoin. It is
 * an interface function to other module for initializing the media list
 * used during a call.
 *
 * @param[in]dcb_p   - pointer to the fsmdef_dcb_t where the media list
 *                   will be attached to.
 *
 * @return           None.
 * @pre              (dcb not_eq NULL)
 */
void gsmsdp_init_media_list (fsmdef_dcb_t *dcb_p)
{
    const cc_media_cap_table_t *media_cap_tbl;
    const char                 fname[] = "gsmsdp_init_media_list";

    /* do the actual media element list initialization */
    (void)sll_lite_init(&dcb_p->media_list);
 
    media_cap_tbl = gsmsdp_get_media_capability(dcb_p);

    if (media_cap_tbl == NULL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capbility available\n", 
                    dcb_p->line, dcb_p->call_id, fname);
    }
}

/**
 *
 * Returns a pointer to the fsmdef_media_t in the dcb for the
 * correspoinding media level in the SDP.
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t
 * @param[in]level      - uint16_t for media line level.
 *
 * @return           pointer to the fsmdef_media_t of the corresponding
 *                   media entry in the dcb.
 * @pre              (dcb not_eq NULL)
 */
static fsmdef_media_t *
gsmsdp_find_media_by_level (fsmdef_dcb_t *dcb_p, uint16_t level)
{
    fsmdef_media_t *media = NULL;

    /*
     * search the all entries that has a valid media and matches
     * the level.
     */
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->level == level) {
            /* found a match */   
            return (media);
        }
    }
    return (NULL);
}

/** 
 *
 * Returns a pointer to the fsmdef_media_t in the dcb for the
 * correspoinding reference ID. 
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t
 * @param[in]refid      - media reference ID to look for.
 * 
 * @return           pointer to the fsmdef_media_t of the corresponding
 *                   media entry in the dcb.
 * @pre              (dcb not_eq NULL)
 */
fsmdef_media_t * 
gsmsdp_find_media_by_refid (fsmdef_dcb_t *dcb_p, media_refid_t refid)
{
    fsmdef_media_t *media = NULL;
 
    /*
     * search the all entries that has a valid media and matches
     * the reference ID.
     */
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->refid == refid) {
            /* found a match */
            return (media);
        }
    }
    return (NULL);
}

/**
 *
 * Returns a pointer to the fsmdef_media_t in the dcb for the
 * correspoinding capability index.
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t
 * @param[in]cap_index  - capability table index.
 * 
 * @return           pointer to the fsmdef_media_t of the corresponding
 *                   media entry in the dcb.
 * @pre              (dcb not_eq NULL)
 */
static fsmdef_media_t *
gsmsdp_find_media_by_cap_index (fsmdef_dcb_t *dcb_p, uint8_t cap_index)
{
    fsmdef_media_t *media = NULL;
    
    /*
     * search the all entries that has a valid media and matches
     * the reference ID.
     */
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->cap_index == cap_index) {
            /* found a match */ 
            return (media);
        }
    }
    return (NULL);

}

/**
 *
 * Returns a pointer to the fsmdef_media_t in the dcb for the
 * first audio type in the SDP.
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t.
 *
 * @return           pointer to the fsmdef_media_t of the corresponding
 *                   media entry in the dcb.
 * @pre              (dcb not_eq NULL)
 */
fsmdef_media_t *gsmsdp_find_audio_media (fsmdef_dcb_t *dcb_p)
{
    fsmdef_media_t *media = NULL;

    /*
     * search the all entries that has a valid media and matches
     * SDP_MEDIA_AUDIO type.
     */
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->type == SDP_MEDIA_AUDIO) {
            /* found a match */   
            return (media);
        }
    }
    return (NULL);
}

/**
 * 
 * The function finds an unused media line given type.  
 * 
 * @param[in]sdp        - void pointer of thd SDP libray handle. 
 * @param[in]media_type - media type of the unused line. 
 * 
 * @return           level (line) of the unused one if found or
 *                   0 if there is no unused one found. 
 */
static uint16_t 
gsmsdp_find_unused_media_line_with_type (void *sdp, sdp_media_e media_type)
{   
    uint16_t num_m_lines, level;
    int32_t  port;

    num_m_lines  = sdp_get_num_media_lines(sdp);
    for (level = 1; level <= num_m_lines; level++) {
        port = sdp_get_media_portnum(sdp, level);
        if (port == 0) {
            /* This slot is not used, check the type */
            if (sdp_get_media_type(sdp, level) == media_type) {
                /* Found an empty slot that has the same media type */
                return (level);
            }
        }
    }
    /* no unused media line of the given type found */
    return (0);
}   

/**
 *
 * The function returns the media cap entry pointer to the caller based
 * on the index.
 *
 * @param[in]cap_index  - uint8_t for index of the media cap table.
 *
 * @return           pointer to the media cap entry if one is available.
 *                   NULL if none is available.
 *
 */
static const cc_media_cap_t *
gsmsdp_get_media_cap_entry_by_index (uint8_t cap_index, fsmdef_dcb_t *dcb_p)
{
    const cc_media_cap_table_t *media_cap_tbl;

    media_cap_tbl = dcb_p->media_cap_tbl;

    if (media_cap_tbl == NULL) {
        return (NULL);
    }

    if (cap_index >= CC_MAX_MEDIA_CAP) {
        return (NULL);
    }
    return (&media_cap_tbl->cap[cap_index]);
}

/**
 *
 * Returns a pointer to the fsmdef_media_t in the dcb for the
 * corresponding media line. It looks for another media line
 * with the same type and cap_index but different level
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t
 * @param[in]media      - current media level.
 * 
 * @return           pointer to the fsmdef_media_t of the corresponding
 *                   media entry in the dcb.
 * @pre              (dcb not_eq NULL)
 */
fsmdef_media_t * 
gsmsdp_find_anat_pair (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    fsmdef_media_t *searched_media = NULL;
 
    /*
     * search the all entries that has a the same capability index
     * but at a different level. The only time that this is true is
     * both media are in the same ANAT group.
     */
    GSMSDP_FOR_ALL_MEDIA(searched_media, dcb_p) {
        if ((searched_media->cap_index == media->cap_index) &&
            (searched_media->level != media->level)) {
            /* found a match */
            return (searched_media);
        }
    }
    return (NULL);
}

/**
 *
 * The function queries platform to see if the platform is capable
 * of handle mixing additional media or not. 
 * 
 * P2: This may go away when integrate with the platform.
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t structure.
 * @param[in]media_type - media type to be mixed.
 *
 * @return          TRUE the media can be mixed. 
 *                  FALSE the media can not be mixed
 *
 * @pre            (dcb_p not_eq NULL)
 */
static boolean
gsmsdp_platform_addition_mix (fsmdef_dcb_t *dcb_p, sdp_media_e media_type)
{
    return (FALSE);
}


/**
 *
 * The function updates the local time stamp during SDP offer/answer
 * processing.
 *
 * @param[in]dcb_p         - pointer to the fsmdef_dcb_t
 * @param[in]offer         - boolean indicates this is procssing an offered
 *                           SDP
 * @param[in]initial_offer - boolean indicates this is processin an 
 *                           initial offered SDP.
 *
 * @return           none.
 * @pre              (dcb not_eq NULL)
 */
static void
gsmsdp_update_local_time_stamp (fsmdef_dcb_t *dcb_p, boolean offer,
                                boolean initial_offer)
{
    const char                 fname[] = "gsmsdp_update_local_time_stamp";
    void           *local_sdp_p;
    void           *remote_sdp_p;

    local_sdp_p  = dcb_p->sdp->src_sdp;
    remote_sdp_p = dcb_p->sdp->dest_sdp;

    /*
     * If we are processing an offer sdp, need to set the
     * start time and stop time based on the remote SDP
     */
    if (initial_offer) {
        /*
         * Per RFC3264, time description of answer must equal that
         * of the offer.
         */
        (void) sdp_set_time_start(local_sdp_p, 
                                  sdp_get_time_start(remote_sdp_p));
        (void) sdp_set_time_stop(local_sdp_p, sdp_get_time_stop(remote_sdp_p));
    } else if (offer) {
        /*
         * Set t= line based on remote SDP
         */
        if (sdp_timespec_valid(remote_sdp_p) != TRUE) {
            GSM_DEBUG(DEB_L_C_F_PREFIX"\nTimespec is invalid.\n",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            (void) sdp_set_time_start(local_sdp_p, "0");
            (void) sdp_set_time_stop(local_sdp_p, "0");
        } else {
            if (sdp_get_time_start(local_sdp_p) !=
                sdp_get_time_start(remote_sdp_p)) {
                (void) sdp_set_time_start(local_sdp_p,
                                          sdp_get_time_start(remote_sdp_p));
            }
            if (sdp_get_time_stop(local_sdp_p) !=
                sdp_get_time_stop(remote_sdp_p)) {
                (void) sdp_set_time_stop(local_sdp_p,
                                         sdp_get_time_stop(remote_sdp_p));
            }
        }
    }
}

/**
 *
 * The function gets the local source address address and puts it into
 * the media entry.
 *
 * @param[in]media   - pointer to fsmdef_media_t structure to
 *                     get the local address into.
 *
 * @return           none.
 * @pre              (media not_eq NULL)
 */
static void
gsmsdp_get_local_source_v4_address (fsmdef_media_t *media)
{
    int              nat_enable = 0;
    char             curr_media_ip[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t    addr;                
    const char       fname[] = "gsmsdp_get_local_source_v4_address";
    int roapproxy;

    /*
     * Get device address.
     */;
    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        init_empty_str(curr_media_ip);
        config_get_value(CFGID_MEDIA_IP_ADDR, curr_media_ip,
                        MAX_IPADDR_STR_LEN);
        if (is_empty_str(curr_media_ip) == FALSE) {

        	//<em>
            roapproxy = 0;
        //	config_get_value(CFGID_ROAPPROXY, &roapproxy, sizeof(roapproxy));
        //	if (roapproxy == TRUE) {
        //		str2ip(gROAPSDP.offerAddress, &addr);
        //	} else {
        		str2ip(curr_media_ip, &addr);
        //	}
             util_ntohl(&addr, &addr);
             if (util_check_if_ip_valid(&media->src_addr) == FALSE)  {
                 // Update the media Src address only if it is invalid
                 media->src_addr = addr;
                 GSM_ERR_MSG("%s:  Update IP %s", fname, curr_media_ip);
             }
        } else {
            sip_config_get_net_device_ipaddr(&media->src_addr);
        }
    } else {
        sip_config_get_nat_ipaddr(&media->src_addr);
    }
}

/*
 *
 * The function gets the local source address address and puts it into
 * the media entry.
 *
 * @param[in]media   - pointer to fsmdef_media_t structure to
 *                     get the local address into.
 *
 * @return           none.
 * @pre              (media not_eq NULL)
 */
static void
gsmsdp_get_local_source_v6_address (fsmdef_media_t *media)
{
    int      nat_enable = 0;

    /*
     * Get device address.
     */
    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        sip_config_get_net_ipv6_device_ipaddr(&media->src_addr);
    } else {
        sip_config_get_nat_ipaddr(&media->src_addr);
    }
}

/**
 * Set the connection address into the SDP.
 *
 * @param[in]sdp_p   - pointer to SDP (type void)
 * @param[in]level   - media level or line.
 * @param[in]addr    - cpr_ip_addr_t of the address to be set in the SDP.
 *
 * @return           none.
 * @pre              (sdp_p not_eq NULL) and (addr not_eq NULL)
 *
 */
static void
gsmsdp_set_connection_address (void *sdp_p, uint16_t level, 
                               cpr_ip_addr_t *addr)
{
    char     addr_str[MAX_IPADDR_STR_LEN];
    char     *p_addr_str;

    ipaddr2dotted(addr_str, addr);
    p_addr_str = strtok(addr_str, "[ ]");

    /*
     * c= line <network type><address type><connection address>
     */
    (void) sdp_set_conn_nettype(sdp_p, level, SDP_NT_INTERNET);
    if (addr->type == CPR_IP_ADDR_IPV4) {
        (void) sdp_set_conn_addrtype(sdp_p, level, SDP_AT_IP4);
    } else if (addr->type == CPR_IP_ADDR_IPV6) {
        (void) sdp_set_conn_addrtype(sdp_p, level, SDP_AT_IP6);
    }
    (void) sdp_set_conn_address(sdp_p, level, p_addr_str);

}

/*
 * gsmsdp_set_2543_hold_sdp
 *
 * Description:
 *
 * Manipulates the local SDP of the specified DCB to indicate hold
 * to the far end using 2543 style signaling.
 *
 * Parameters:
 *
 * dcb_p - Pointer to the DCB whose SDP is to be manipulated.
 *
 */
static void
gsmsdp_set_2543_hold_sdp (fsmdef_dcb_t *dcb_p, uint16 level)
{
    (void) sdp_set_conn_nettype(dcb_p->sdp->src_sdp, level, SDP_NT_INTERNET);
    (void) sdp_set_conn_addrtype(dcb_p->sdp->src_sdp, level, SDP_AT_IP4);
    (void) sdp_set_conn_address(dcb_p->sdp->src_sdp, level, "0.0.0.0");
}


/*
 * gsmsdp_set_video_media_attributes
 *
 * Description:
 *
 * Add the specified video media format to the SDP.
 *
 * Parameters:
 *
 * media_type - The media type (format) to add to the specified SDP.
 * sdp_p - Pointer to the SDP the media attribute is to be added to.
 * level - The media level of the SDP where the media attribute is to be added.
 * payload_number - AVT payload type if the media attribute being added is
 *                  RTP_AVT.
 *
 */
static void
gsmsdp_set_video_media_attributes (uint32_t media_type, void *cc_sdp_p, uint16_t level,
                             uint16_t payload_number)
{
    uint16_t a_inst;
    void *sdp_p = ((cc_sdp_t*)cc_sdp_p)->src_sdp;

    switch (media_type) {
        case RTP_H263:
        case RTP_H264_P0:
        case RTP_H264_P1:
        /*
         * add a=rtpmap line
         */
        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTPMAP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }

        (void) sdp_attr_set_rtpmap_payload_type(sdp_p, level, 0, a_inst,
                                                payload_number);

        switch (media_type) {
        case RTP_H263:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_H263v2);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                             RTPMAP_VIDEO_CLOCKRATE);
            break;
        case RTP_H264_P0: 
        case RTP_H264_P1: 
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_H264);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                             RTPMAP_VIDEO_CLOCKRATE);
            break;
        }
    GSM_DEBUG("gsmsdp_set_video_media_attributes- populate attribs %d\n", payload_number );

        vcmPopulateAttribs(cc_sdp_p, level, media_type, payload_number, FALSE);

        break;

        default: 
            break;
    }
}

/*
 * gsmsdp_set_media_attributes
 *
 * Description:
 *
 * Add the specified media format to the SDP.
 *
 * Parameters:
 *
 * media_type - The media type (format) to add to the specified SDP.
 * sdp_p - Pointer to the SDP the media attribute is to be added to.
 * level - The media level of the SDP where the media attribute is to be added.
 * payload_number - AVT payload type if the media attribute being added is
 *                  RTP_AVT.
 *
 */
static void
gsmsdp_set_media_attributes (uint32_t media_type, void *sdp_p, uint16_t level,
                             uint16_t payload_number)
{
    uint16_t a_inst, a_inst2;

    switch (media_type) { 
    case RTP_PCMU:             // type 0
    case RTP_PCMA:             // type 8
    case RTP_G729:             // type 18
    case RTP_G722:             // type 9
    case RTP_ILBC:             
    case RTP_L16:
    case RTP_ISAC:
        /*
         * add a=rtpmap line
         */
        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTPMAP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }

        (void) sdp_attr_set_rtpmap_payload_type(sdp_p, level, 0, a_inst,
                                                payload_number);

        switch (media_type) {
        case RTP_PCMU:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_PCMU);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);               
            break;
        case RTP_PCMA:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_PCMA);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);               
            break;
        case RTP_G729:
            {

            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_G729);
            if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst2)
                    != SDP_SUCCESS) {
                return;
            }
            (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst2,
                                                  payload_number);
            (void) sdp_attr_set_fmtp_annexb(sdp_p, level, 0, a_inst2, FALSE);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);
            }
            break;
            
        case RTP_G722:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_G722);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);               
            break;

        case RTP_L16:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_L16_256K);
                                             
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_L16_CLOCKRATE);               
            break;            

        case RTP_ILBC:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_ILBC);
            if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst2)
                    != SDP_SUCCESS) {
                return;
            }
            (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst2,
                                                  payload_number);
            (void) sdp_attr_set_fmtp_mode(sdp_p, level, 0, a_inst2, vcmGetILBCMode());

            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                RTPMAP_CLOCKRATE);
            break;

        case RTP_ISAC:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_ISAC);

            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_ISAC_CLOCKRATE);
            break;

        }    
        break;

    case RTP_AVT:
        /*
         * add a=rtpmap line
         */
        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTPMAP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }
        (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                           SIPSDP_ATTR_ENCNAME_TEL_EVENT);
        (void) sdp_attr_set_rtpmap_payload_type(sdp_p, level, 0, a_inst,
                                                payload_number);
        (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                             RTPMAP_CLOCKRATE);

        /*
         * Malloc the mediainfo structure
         */
        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }
        (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst,
                                              payload_number);
        (void) sdp_attr_set_fmtp_range(sdp_p, level, 0, a_inst,
                                       SIPSDP_NTE_DTMF_MIN,
                                       SIPSDP_NTE_DTMF_MAX);


        break;

    default:
        /* The remaining coded types aren't supported, but are listed below
         * as a reminder
         *   RTP_CELP         = 1,
         *   RTP_GSM          = 3,
         *   RTP_G726         = 2,
         *   RTP_G723         = 4,
         *   RTP_DVI4         = 5,
         *   RTP_DVI4_II      = 6,
         *   RTP_LPC          = 7,
         *   RTP_G722         = 9,
         *   RTP_G728         = 15,
         *   RTP_JPEG         = 26,
         *   RTP_NV           = 28,
         *   RTP_H261         = 31
         */

        break;
    }
}

/*
 * gsmsdp_set_remote_sdp
 *
 * Description:
 *
 * Sets the specified SDP as the remote SDP in the DCB.
 *
 * Parameters:
 *
 * dcb_p - Pointer to the DCB.
 * sdp_p - Pointer to the SDP to be set as the remote SDP in the DCB.
 */
static void
gsmsdp_set_remote_sdp (fsmdef_dcb_t *dcb_p, cc_sdp_t *sdp_p)
{
    dcb_p->remote_sdp_present = TRUE;
}

/*
 * gsmsdp_get_sdp_direction_attr
 *
 * Description:
 *
 * Given a sdp_direction_e enumerated type, returns a sdp_attr_e
 * enumerated type.
 *
 * Parameters:
 *
 * direction - The SDP direction used to determine which sdp_attr_e to return.
 *
 */
static sdp_attr_e
gsmsdp_get_sdp_direction_attr (sdp_direction_e direction)
{
    sdp_attr_e sdp_attr = SDP_ATTR_SENDRECV;

    switch (direction) {
    case SDP_DIRECTION_INACTIVE:
        sdp_attr = SDP_ATTR_INACTIVE;
        break;
    case SDP_DIRECTION_SENDONLY:
        sdp_attr = SDP_ATTR_SENDONLY;
        break;
    case SDP_DIRECTION_RECVONLY:
        sdp_attr = SDP_ATTR_RECVONLY;
        break;
    case SDP_DIRECTION_SENDRECV:
        sdp_attr = SDP_ATTR_SENDRECV;
        break;
    default:
        GSM_ERR_MSG("\nFSMDEF ERROR: replace with formal error text");
    }

    return sdp_attr;
}

/*
 * gsmsdp_set_sdp_direction
 *
 * Description:
 *
 * Adds a direction attribute to the given media line in the
 * specified SDP.
 *
 * Parameters:
 *
 * media     - pointer to the fsmdef_media_t for the media entry.
 * direction - The direction to use when setting the direction attribute.
 * sdp_p - Pointer to the SDP to set the direction attribute against.
 */
static void
gsmsdp_set_sdp_direction (fsmdef_media_t *media,
                          sdp_direction_e direction, void *sdp_p)
{
    sdp_attr_e    sdp_attr = SDP_ATTR_SENDRECV;
    uint16_t      a_instance = 0;

    /*
     * Convert the direction to an SDP direction attribute.
     */
    sdp_attr = gsmsdp_get_sdp_direction_attr(direction);
    if (media->level) {
       (void) sdp_add_new_attr(sdp_p, media->level, 0, sdp_attr, &a_instance);
    } else {
       /* Just in case that there is no level defined, add to the session */
       (void) sdp_add_new_attr(sdp_p, SDP_SESSION_LEVEL, 0, sdp_attr, 
                               &a_instance);
    }
}

/*
 * gsmsdp_remove_sdp_direction
 *
 * Description:
 *
 * Removes the direction attribute corresponding to the passed in direction
 * from the media line of the specified SDP.
 *
 * Parameters:
 *
 * media     - pointer to the fsmdef_media_t for the media entry.
 * direction - The direction whose corresponding direction attribute
 *             is to be removed.
 * sdp_p - Pointer to the SDP where the direction attribute is to be
 *         removed.
 */
static void
gsmsdp_remove_sdp_direction (fsmdef_media_t *media, 
                             sdp_direction_e direction, void *sdp_p)
{
    sdp_attr_e    sdp_attr = SDP_ATTR_SENDRECV;

    sdp_attr = gsmsdp_get_sdp_direction_attr(direction);
    (void) sdp_delete_attr(sdp_p, media->level, 0, sdp_attr, 1);
}

/*
 * gsmsdp_set_local_sdp_direction
 *
 * Description:
 *
 * Sets the direction attribute for the local SDP.
 *
 * Parameters:
 *
 * dcb_p - The DCB where the local SDP is located.
 * media - Pointer to fsmdef_media_t for the media entry of the SDP.
 * direction - The media direction to set into the local SDP.
 */
void
gsmsdp_set_local_sdp_direction (fsmdef_dcb_t *dcb_p, 
                                fsmdef_media_t *media, 
                                sdp_direction_e direction)
{
    /*
     * If media direction was previously set, remove the direction attribute
     * before adding the specified direction. Save the direction in previous
     * direction before clearing it.
     */
    if (media->direction_set) {
        media->previous_sdp.direction = media->direction;
        gsmsdp_remove_sdp_direction(media, media->direction, 
                                    dcb_p->sdp ? dcb_p->sdp->src_sdp: NULL );
        media->direction_set = FALSE;
    }
    gsmsdp_set_sdp_direction(media, direction, dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL);
    /*
     * We could just get the direction from the local SDP when we need it in
     * GSM, but setting the direction in the media structure gives a quick way
     * to access the media direction.
     */
    media->direction = direction;
    media->direction_set = TRUE;
}

/*
 * gsmsdp_get_remote_sdp_direction
 *
 * Description:
 *
 * Returns the media direction from the specified SDP. We will check for the
 * media direction attribute at the session level and the first AUDIO media line.
 * If the direction attribute is specified at the media level, the media level setting
 * overrides the session level attribute.
 *
 * Parameters:
 *
 * dcb_p - pointer to the fsmdef_dcb_t.
 * level - media line level.
 * dest_addr - pointer to the remote address.
 */
static sdp_direction_e
gsmsdp_get_remote_sdp_direction (fsmdef_dcb_t *dcb_p, uint16_t level,
                                 cpr_ip_addr_t *dest_addr)
{
    sdp_direction_e direction = SDP_DIRECTION_SENDRECV;
    cc_sdp_t       *sdp_p = dcb_p->sdp;
    uint16_t       media_attr;
    uint16_t       i;
    static const sdp_attr_e  dir_attr_array[] = {
        SDP_ATTR_INACTIVE,
        SDP_ATTR_RECVONLY,
        SDP_ATTR_SENDONLY,
        SDP_ATTR_SENDRECV,
        SDP_MAX_ATTR_TYPES
    };

    if (!sdp_p->dest_sdp) {
        return direction;
    }

    media_attr = 0; /* media level attr. count */
    /*
     * Now check for direction as a media attribute. If found, the
     * media attribute takes precedence over direction specified
     * as a session attribute.
     *
     * In order to find out whether there is a direction attribute
     * associated with a media line (or even at the
     * session level) or not is to get the number of instances of
     * that attribute via the sdp_attr_num_instances() first. The is
     * because the sdp_get_media_direction() always returns a valid
     * direction value even when there is no direction attribute with
     * the media line (or session level).
     *
     * Note: there is no single attribute value to pass to the
     * sdp_attr_num_instances() to get number a direction attribute that
     * represents inactive, recvonly, sendonly, sendrcv. We have to
     * look for each one of them individually.
     */
    for (i = 0; (dir_attr_array[i] != SDP_MAX_ATTR_TYPES); i++) {
        if (sdp_attr_num_instances(sdp_p->dest_sdp, level, 0,
                                   dir_attr_array[i], &media_attr) ==
            SDP_SUCCESS) {
            if (media_attr) {
                /* There is direction attribute in the media line */
                direction = sdp_get_media_direction(sdp_p->dest_sdp,
                                                    level, 0);
                break;
            }
        }
    }

    /*
     * Check for the direction attribute. The direction can be specified
     * as a session attribute or a media stream attribute. If the direction
     * is specified as a session attribute, the direction is applicable to
     * all media streams in the SDP.
     */
    if (media_attr == 0) {
        /* no media level direction, get the direction from session */
        direction = sdp_get_media_direction(sdp_p->dest_sdp,
                                            SDP_SESSION_LEVEL, 0);
    }

    /*
     * To support legacy way of signaling remote hold, we will interpret
     * c=0.0.0.0 to be a=inactive
     */
    if (dest_addr->type == CPR_IP_ADDR_IPV4 &&  
        dest_addr->u.ip4 == 0) {

        direction = SDP_DIRECTION_INACTIVE;
    } else {

        //todo IPv6: reject the request.
    }
    return direction;
}

/**
 *
 * The function overrides direction for some special feature
 * processing.
 *
 * @param[in]dcb_p  - pointer to the fsmdef_dcb_t
 * @param[in]media  - pointer to fsmdef_media_t for the media to
 *                    override the direction.
 *
 * @return           None.
 * @pre              (dcb_p not_eq NULL) and (media not_eq NULL)
 */
static void
gsmsdp_feature_overide_direction (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    /* 
     * Disable video if this is a BARGE with video
     */
    if ( CC_IS_VIDEO(media->cap_index) && 
                   dcb_p->join_call_id != CC_NO_CALL_ID ){
        media->support_direction = SDP_DIRECTION_INACTIVE;
    }

    if (CC_IS_VIDEO(media->cap_index) && media->support_direction == SDP_DIRECTION_INACTIVE) {
        DEF_DEBUG(GSM_F_PREFIX"video capability disabled to SDP_DIRECTION_INACTIVE \n", "gsmsdp_feature_overide_direction");
    }
}

/*
 * gsmsdp_negotiate_local_sdp_direction
 *
 * Description:
 *
 * Given an offer SDP, return the corresponding answer SDP direction.
 *
 * local hold   remote direction support direction new local direction
 * enabled      inactive            any                 inactive
 * enabled      sendrecv          sendonly              sendonly
 * enabled      sendrecv          recvonly              inactive
 * enabled      sendrecv          sendrecv              sendonly
 * enabled      sendrecv          inactive              inactive
 * enabled      sendonly            any                 inactive
 * enabled      recvonly          sendrecv              sendonly
 * enabled      recvonly          sendonly              sendonly
 * enabled      recvonly          recvonly              inactive
 * enabled      recvonly          inactive              inactive
 * disabled     inactive            any                 inactive
 * disabled     sendrecv          sendrecv              sendrecv
 * disabled     sendrecv          sendonly              sendonly
 * disabled     sendrecv          recvonly              recvonly
 * disabled     sendrecv          inactive              inactive
 * disabled     sendonly          sendrecv              recvonly           
 * disabled     sendonly          sendonly              inactive           
 * disabled     sendonly          recvonly              recvonly
 * disabled     sendonly          inactive              inactive
 * disabled     recvonly          sendrecv              sendonly
 * disabled     recvonly          sendonly              sendonly
 * disabled     recvonly          recvonly              inactive
 * disabled     recvonly          inactive              inactive
 *
 * Parameters:
 *
 * dcb_p - pointer to the fsmdef_dcb_t.
 * media - pointer to the fsmdef_media_t for the current media entry.
 * local_hold - Boolean indicating if local hold feature is enabled
 */
static sdp_direction_e
gsmsdp_negotiate_local_sdp_direction (fsmdef_dcb_t *dcb_p,
                                      fsmdef_media_t *media,
                                      boolean local_hold)
{
    sdp_direction_e direction = SDP_DIRECTION_SENDRECV;
    sdp_direction_e remote_direction = gsmsdp_get_remote_sdp_direction(dcb_p,
                                           media->level, &media->dest_addr);
    
    if (remote_direction == SDP_DIRECTION_SENDRECV) {
        if (local_hold) {
            if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
                (media->support_direction == SDP_DIRECTION_SENDONLY)) { 
                direction = SDP_DIRECTION_SENDONLY;
            } else {
                direction = SDP_DIRECTION_INACTIVE;
            }
        } else {
            direction = media->support_direction;
        }
    } else if (remote_direction == SDP_DIRECTION_SENDONLY) {
        if (local_hold) {
            direction = SDP_DIRECTION_INACTIVE;
        } else {
            if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
                (media->support_direction == SDP_DIRECTION_RECVONLY)) {
                direction = SDP_DIRECTION_RECVONLY;
            } else {
                direction = SDP_DIRECTION_INACTIVE;
            }
        }
    } else if (remote_direction == SDP_DIRECTION_INACTIVE) {
        direction = SDP_DIRECTION_INACTIVE;
    } else if (remote_direction == SDP_DIRECTION_RECVONLY) {
        if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
            (media->support_direction == SDP_DIRECTION_SENDONLY)) {
            direction = SDP_DIRECTION_SENDONLY;
        } else {
            direction = SDP_DIRECTION_INACTIVE;
        }
    }

    return direction;
}

/*
 * gsmsdp_add_default_audio_formats_to_local_sdp
 *
 * Description:
 *
 * Add all supported media formats to the local SDP of the specified DCB
 * at the specified media level. If the call is involved in a conference
 * call, only add G.711 formats.
 *
 * Parameters
 *
 * dcb_p - The DCB whose local SDP is to be updated with the default media formats.
 * sdp_p - Pointer to the local sdp structure. This is added so call to this
 *         routine can be made irrespective of whether we have a dcb or not(To
 *         handle out-of-call options request for example)
 * media - Pointer to fsmdef_media_t for the media entry of the SDP.
 *
 */
static void
gsmsdp_add_default_audio_formats_to_local_sdp (fsmdef_dcb_t *dcb_p,
                                               cc_sdp_t * sdp_p,
                                               fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_add_default_audio_formats_to_local_sdp";
    int             local_media_types[CC_MAX_MEDIA_TYPES];
    int16_t         local_avt_payload_type = RTP_NONE;
    DtmfOutOfBandTransport_t transport = DTMF_OUTOFBAND_NONE;
    int             type_cnt;
    void           *local_sdp_p = NULL;
    uint16_t        media_format_count;
    uint16_t        level;

    if (media) {
        level = media->level;
    } else {
        level = 1;
    }
    local_sdp_p = (void *) sdp_p->src_sdp;

    /*
     * Create list of supported codecs. Get all the codecs that the phone supports.
     */
    media_format_count = sip_config_local_supported_codecs_get(
                                (rtp_ptype *) local_media_types,
                                CC_MAX_MEDIA_TYPES);
    /*
     * If the media payload type is set to RTP_NONE, its because we are making an
     * initial offer. We will be opening our receive port so we need to specify
     * the media payload type to be used initially. We set the media payload
     * type in the dcb to do this. Until we receive an answer from the far
     * end, we will use our first choice payload type. i.e. the first payload
     * type sent in our AUDIO media line.
     */
    if (dcb_p && media && media->payload == RTP_NONE) {
        media->payload = local_media_types[0];
        media->previous_sdp.payload_type = local_media_types[0];
        media->previous_sdp.local_payload_type = local_media_types[0];
    }
    /* reset the local dynamic payload to NONE */
    if (media) {
        media->local_dynamic_payload_type_value = RTP_NONE;
    }

    /*
     * Get configured OOB DTMF setting and avt payload type if applicable
     */
    config_get_value(CFGID_DTMF_OUTOFBAND, &transport, sizeof(transport));

    if ((transport == DTMF_OUTOFBAND_AVT) ||
        (transport == DTMF_OUTOFBAND_AVT_ALWAYS)) {
        int temp_payload_type = RTP_NONE;

        config_get_value(CFGID_DTMF_AVT_PAYLOAD,
                         &(temp_payload_type),
                         sizeof(temp_payload_type));
        local_avt_payload_type = (uint16_t) temp_payload_type;
    }

    /*
     * add all the audio media types
     */
    for (type_cnt = 0;
         (type_cnt < media_format_count) &&
         (local_media_types[type_cnt] > RTP_NONE);
         type_cnt++) {

        if (sdp_add_media_payload_type(local_sdp_p, level,
                                       (uint16_t)local_media_types[type_cnt],
                                       SDP_PAYLOAD_NUMERIC) != SDP_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"Adding media payload type failed\n", 
                        DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        }

        gsmsdp_set_media_attributes(local_media_types[type_cnt], local_sdp_p,
                                    level, (uint16_t)local_media_types[type_cnt]);
    }

    /*
     * add the avt media type
     */
    if (local_avt_payload_type > RTP_NONE) {
        if (sdp_add_media_payload_type(local_sdp_p, level,
                                       local_avt_payload_type,
                                       SDP_PAYLOAD_NUMERIC) != SDP_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"Adding AVT payload type failed\n",  
                        dcb_p->line, dcb_p->call_id, fname);
        }

        gsmsdp_set_media_attributes(RTP_AVT, local_sdp_p, level,
                                    local_avt_payload_type);
        if (media) {
            media->avt_payload_type = local_avt_payload_type;
        }
    }
}

/*
 * gsmsdp_add_default_video_formats_to_local_sdp
 *
 * Description:
 *
 * Add all supported media formats to the local SDP of the specified DCB
 * at the specified media level. If the call is involved in a conference
 * call, only add G.711 formats.
 *
 * Parameters
 *
 * dcb_p - The DCB whose local SDP is to be updated with the default media formats.
 * sdp_p - Pointer to the local sdp structure. This is added so call to this
 *         routine can be made irrespective of whether we have a dcb or not(To
 *         handle out-of-call options request for example)
 * media - Pointer to fsmdef_media_t for the media entry of the SDP.
 *
 */
static void
gsmsdp_add_default_video_formats_to_local_sdp (fsmdef_dcb_t *dcb_p,
                                               cc_sdp_t * sdp_p,
                                               fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_add_default_video_formats_to_local_sdp";
    int             video_media_types[CC_MAX_MEDIA_TYPES];
    int             type_cnt;
    void           *local_sdp_p = NULL;
    uint16_t        video_format_count;
    uint16_t        level;
    line_t          line = 0;
    callid_t        call_id = 0;

    if (dcb_p != NULL && media != NULL) {
        line = dcb_p->line;
        call_id = dcb_p->call_id;
    }
    GSM_DEBUG(DEB_L_C_F_PREFIX"\n", DEB_L_C_F_PREFIX_ARGS(GSM, line, call_id, fname));

    if (media) {
        level = media->level;
    } else {
        level = 2;
    }
    local_sdp_p = (void *) sdp_p->src_sdp;

    /*
     * Create list of supported codecs. Get all the codecs that the phone supports.
     */

    video_format_count = sip_config_video_supported_codecs_get( (rtp_ptype *) video_media_types,
                                                 CC_MAX_MEDIA_TYPES, TRUE /*offer*/);

    GSM_DEBUG(DEB_L_C_F_PREFIX"video_count=%d\n", DEB_L_C_F_PREFIX_ARGS(GSM, line, call_id, fname), video_format_count);
    /*
     * If the media payload type is set to RTP_NONE, its because we are making an
     * initial offer. We will be opening our receive port so we need to specify
     * the media payload type to be used initially. We set the media payload
     * type in the dcb to do this. Until we receive an answer from the far
     * end, we will use our first choice payload type. i.e. the first payload
     * type sent in our video media line.
     */
    if (dcb_p && media && media->payload == RTP_NONE) {
        media->payload = video_media_types[0];
        media->previous_sdp.payload_type = video_media_types[0];
        media->previous_sdp.local_payload_type = video_media_types[0];
    }
    /* reset the local dynamic payload to NONE */
    if (media) {
        media->local_dynamic_payload_type_value = RTP_NONE;
    }

    /*
     * add all the video media types
     */
    for (type_cnt = 0;
         (type_cnt < video_format_count) &&
         (video_media_types[type_cnt] > RTP_NONE);
         type_cnt++) {

        if (sdp_add_media_payload_type(local_sdp_p, level,
                                       (uint16_t)video_media_types[type_cnt],
                                       SDP_PAYLOAD_NUMERIC) != SDP_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"SDP ERROR(1)\n", 
                        line, call_id, fname);
        }

        gsmsdp_set_video_media_attributes(video_media_types[type_cnt], sdp_p,
                           level, (uint16_t)video_media_types[type_cnt]);
    }
}

/**
 * This function sets the mid attr at the media level
 * and labels the relevant media streams when phone is
 * operating in a dual stack mode
 *
 * @param[in]src_sdp_p  - Our source sdp.
 * @param[in]level      - The level of the media that we are operating on.
 *
 * @return           - None
 *
 */
static void gsmsdp_set_mid_attr (void *src_sdp_p, uint16_t level)
{
    uint16         inst_num;

    if (platform_get_ip_address_mode() == CPR_IP_MODE_DUAL) {
        /*
         * add a=mid line
         */
        (void) sdp_add_new_attr(src_sdp_p, level, 0, SDP_ATTR_MID, &inst_num);

        (void) sdp_attr_set_simple_u32(src_sdp_p, SDP_ATTR_MID, level, 0, 
                                       inst_num, level);
    }
}

/**
 * This function sets the anat attr to the session level
 * and labels the relevant media streams  
 *
 * @param[in]media   - The media line that we are operating on. 
 * @param[in]dcb_p   - Pointer to the DCB whose local SDP is to be updated.
 *
 * @return           - None
 *                   
 */
static void gsmsdp_set_anat_attr (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    void           *src_sdp_p = (void *) dcb_p->sdp->src_sdp;
    void           *dest_sdp_p = (void *) dcb_p->sdp->dest_sdp;
    uint16         inst_num;
    uint16_t       num_group_lines= 0;
    uint16_t       num_anat_lines = 0;
    u32            group_id_1, group_id_2;
    uint16_t       i;
    fsmdef_media_t *group_media;
   

    if (dest_sdp_p == NULL) {
        /* If this is our initial offer */
        if (media->addr_type == SDP_AT_IP4) {      
            group_media = gsmsdp_find_anat_pair(dcb_p, media);
            if (group_media != NULL) {
                /*
                 * add a=group line
                 */
                 (void) sdp_add_new_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP, &inst_num);

                 (void) sdp_set_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, SDP_GROUP_ATTR_ANAT);             

                 (void) sdp_set_group_num_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, 2);
                 (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, group_media->level);
                 (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, media->level);       
            }
        }
    } else {
        /* This is an answer, check if the offer rcvd had anat grouping */
        (void) sdp_attr_num_instances(dest_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);
                            
        for (i = 1; i <= num_group_lines; i++) {
             if (sdp_get_group_attr(dest_sdp_p, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {                                  
                 num_anat_lines++;
             }
        }

        for (i = 1; i <= num_anat_lines; i++) {
             group_id_1 = sdp_get_group_id(dest_sdp_p, SDP_SESSION_LEVEL, 0, i, 1);
             group_id_2 = sdp_get_group_id(dest_sdp_p, SDP_SESSION_LEVEL, 0, i, 2);
 
             if ((media->level == group_id_1)  || (media->level == group_id_2)) {       
            
                 group_media = gsmsdp_find_anat_pair(dcb_p, media);
                 if (group_media != NULL) { 
                     if (sdp_get_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, i) != SDP_GROUP_ATTR_ANAT) {
                         /*
                          * add a=group line
                          */
                         (void) sdp_add_new_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP, &inst_num);
                         (void) sdp_set_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, SDP_GROUP_ATTR_ANAT);
            
                     }                
                     (void) sdp_set_group_num_id(src_sdp_p, SDP_SESSION_LEVEL, 0, i, 2);
                     (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, i, group_media->level);
                     (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, i, media->level);
               
                 } else {
                     /*
                      * add a=group line
                      */
                     (void) sdp_add_new_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP, &inst_num);
                     (void) sdp_set_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, SDP_GROUP_ATTR_ANAT);

                     (void) sdp_set_group_num_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, 1);
                     (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, media->level);
                 }
                
             }
        }
    }        
    gsmsdp_set_mid_attr (src_sdp_p, media->level);
}

/*
 * gsmsdp_update_local_sdp_media
 *
 * Description:
 *
 * Adds an AUDIO media line to the local SDP of the specified DCB. If all_formats
 * is TRUE, sets all media formats supported by the phone into the local SDP, else
 * only add the single negotiated media format. If an AUDIO media line already
 * exists in the local SDP, remove it as this function completely rebuilds the
 * AUDIO media line and will do so at the same media level as the pre-existing
 * AUDIO media line.
 *
 * Parameters:
 *
 * dcb_p - Pointer to the DCB whose local SDP is to be updated.
 * cc_sdp_p - Pointer to the SDP being updated.
 * all_formats - If true, all supported media formats will be added to the
 *               AUDIO media line of the SDP. Otherwise, only the single
 *               negotiated media format is added.
 * media     - Pointer to fsmdef_media_t for the media entry of the SDP.
 * transport - transport type to for this media line.
 *
 */
static void
gsmsdp_update_local_sdp_media (fsmdef_dcb_t *dcb_p, cc_sdp_t *cc_sdp_p,
                              boolean all_formats, fsmdef_media_t *media,
                              sdp_transport_e transport)
{
    static const char fname[] = "gsmsdp_update_local_sdp_media";
    uint16_t        port;
    sdp_result_e    result;
    int             dynamic_payload_type;
    uint16_t        level;
    void           *sdp_p; 

    if (!dcb_p || !media)  {
        GSM_ERR_MSG(get_debug_string(FSMDEF_DBG_INVALID_DCB), fname);
        return;
    }
    level = media->level;
    port  = media->src_port;

    sdp_p = cc_sdp_p ? (void *) cc_sdp_p->src_sdp : NULL;
    
    if (sdp_p == NULL) {

        gsmsdp_init_local_sdp(&(dcb_p->sdp));

        cc_sdp_p = dcb_p->sdp;
        if ((cc_sdp_p == NULL) || (cc_sdp_p->src_sdp == NULL)) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"sdp is NULL and init failed \n", 
                    dcb_p->line, dcb_p->call_id, fname);
            return;
        }
        sdp_p = (void *) cc_sdp_p->src_sdp; 
    } else {

    /*
     * Remove the audio stream. Reset direction_set flag since
     * all media attributes have just been removed.
     */
    sdp_delete_media_line(sdp_p, level);
    media->direction_set = FALSE;
    }

    result = sdp_insert_media_line(sdp_p, level);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"Inserting media line to Sdp failed\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return;
    }

    gsmsdp_set_connection_address(sdp_p, media->level, &media->src_addr);
    (void) sdp_set_media_type(sdp_p, level, media->type);
    (void) sdp_set_media_portnum(sdp_p, level, port);

    /* Set media transport and crypto attributes if it is for SRTP */
    gsmsdp_update_local_sdp_media_transport(dcb_p, sdp_p, media, transport,
                                            all_formats);

    if (all_formats) {
        /*
         * Add all supported media formats to the local sdp.
         */
        switch (media->type) {
        case SDP_MEDIA_AUDIO:
            gsmsdp_add_default_audio_formats_to_local_sdp(dcb_p, cc_sdp_p,
                                                          media);
            break;
        case SDP_MEDIA_VIDEO:
            gsmsdp_add_default_video_formats_to_local_sdp(dcb_p, cc_sdp_p,
                                                          media);
            break;
        default:
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"SDP ERROR media %d for level %d is not"
                        " supported\n", 
                        dcb_p->line, dcb_p->call_id, fname, media->level);
            break;
        }
    } else {

        /*
         * add the single negotiated media format
         * answer with the same dynamic payload type value, if a dynamic payload is choosen.
         */
        if (media->remote_dynamic_payload_type_value > 0) {
            dynamic_payload_type = media->remote_dynamic_payload_type_value;
        } else {
            dynamic_payload_type = media->payload;
        }
        result =
            sdp_add_media_payload_type(sdp_p, level,
                                       (uint16_t)dynamic_payload_type,
                                       SDP_PAYLOAD_NUMERIC);
        if (result != SDP_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"Adding dynamic payload type failed\n", 
                        dcb_p->line, dcb_p->call_id, fname);
        }
        switch (media->type) {
        case SDP_MEDIA_AUDIO:
            gsmsdp_set_media_attributes(media->payload, sdp_p, level,
                                    (uint16_t)dynamic_payload_type);
            break;
        case SDP_MEDIA_VIDEO:
            gsmsdp_set_video_media_attributes(media->payload, cc_sdp_p, level,
                            (uint16_t)dynamic_payload_type);
            break;
        default:
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"SDP ERROR media %d for level %d is not"
                        " supported\n",
                        dcb_p->line, dcb_p->call_id, fname, media->level);
            break;
        }

        /*
         * add the avt media type
         */
        if (media->avt_payload_type > RTP_NONE) {
            result = sdp_add_media_payload_type(sdp_p, level,
                         (uint16_t)media->avt_payload_type,
                         SDP_PAYLOAD_NUMERIC);
            if (result != SDP_SUCCESS) {
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"Adding AVT payload type failed\n", 
                            dcb_p->line, dcb_p->call_id, fname);
            }
            gsmsdp_set_media_attributes(RTP_AVT, sdp_p, level,
                (uint16_t) media->avt_payload_type);
        }
    }
    gsmsdp_set_anat_attr(dcb_p, media);
}

/*
 * gsmsdp_update_local_sdp
 *
 * Description:
 *
 * Updates the local SDP of the DCB based on the remote SDP.
 *
 * Parameters:
 *
 * dcb_p - Pointer to the DCB whose local SDP is to be updated.
 * offer - Indicates whether the remote SDP was received in an offer
 *               or an answer.
 * initial_offer - this media line is initial offer.
 * media - Pointer to fsmdef_media_t for the media entry of the SDP.
 *
 * Return:
 *    TRUE  - update the local SDP was successfull.
 *    FALSE - update the local SDP failed.
 */
static boolean
gsmsdp_update_local_sdp (fsmdef_dcb_t *dcb_p, boolean offer, 
                         boolean initial_offer,
                         fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_update_local_sdp";
    cc_action_data_t data;
    void           *local_sdp_p;
    void           *remote_sdp_p;
    sdp_direction_e direction;
    boolean         local_hold = (boolean)FSM_CHK_FLAGS(media->hold, FSM_HOLD_LCL);

    local_sdp_p = dcb_p->sdp->src_sdp;
    remote_sdp_p = dcb_p->sdp->dest_sdp;

    if (media->src_port == 0) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"allocate receive port for media line\n", 
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        /*
         * Source port has not been allocated, this could mean we
         * processing an initial offer SDP, SDP that requets to insert
         * a media line or re-insert a media line.
         */
        data.open_rcv.is_multicast = FALSE;
        data.open_rcv.listen_ip = ip_addr_invalid;
        data.open_rcv.port = 0;
        data.open_rcv.keep = FALSE;
        /*
         * Indicate type of media (audio/video etc) becase some for supporting
         * video over vieo, the port is obtained from other entity.
         */
        data.open_rcv.media_type = media->type;
        data.open_rcv.media_refid = media->refid;
        if (cc_call_action(dcb_p->call_id, dcb_p->line, CC_ACTION_OPEN_RCV,
                           &data) == CC_RC_SUCCESS) {
            /* allocate port successful, save the port */
            media->src_port = data.open_rcv.port;
            media->rcv_chan = FALSE;  /* mark no RX chan yet */
        } else {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"allocate rx port failed\n", 
                        dcb_p->line, dcb_p->call_id, fname);
            return (FALSE);
        }
    }

    /*
     * Negotiate direction based on remote SDP.
     */
    direction = gsmsdp_negotiate_local_sdp_direction(dcb_p, media, local_hold);

    /*
     * Update Transmit SRTP transmit key if this SRTP session.
     */
    if (media->transport == SDP_TRANSPORT_RTPSAVP) {
    gsmsdp_update_crypto_transmit_key(dcb_p, media, offer, 
                                      initial_offer, direction);
    }

    if (offer == TRUE) {
        gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, FALSE, media,
                                      media->transport);
    }

    /*
     * Set local sdp direction.
     */
    if (media->direction_set) {
        if (media->direction != direction) {
            gsmsdp_set_local_sdp_direction(dcb_p, media, direction);
        }
    } else {
        gsmsdp_set_local_sdp_direction(dcb_p, media, direction);
    }
    return (TRUE);
}

/*
 * gsmsdp_update_local_sdp_for_multicast
 *
 * Description:
 *
 * Updates the local SDP of the DCB based on the remote SDP for
 * multicast. Populates the local sdp with the same addr:port as
 * in the offer and the same direction as in the offer (as per
 * rfc3264).
 *
 * Parameters:
 *
 * dcb_p         - Pointer to the DCB whose local SDP is to be updated.
 * portnum       - Remote port.
 * media         - Pointer to fsmdef_media_t for the media entry of the SDP. 
 * offer         - boolean indicating an offer SDP if true.
 * initial_offer - boolean indicating an initial offer SDP if true.
 *
 */
static boolean
gsmsdp_update_local_sdp_for_multicast (fsmdef_dcb_t *dcb_p,
                                      uint16_t portnum,
                                      fsmdef_media_t *media,
                                      boolean offer,
                                      boolean initial_offer)
{
   static const char fname[] = "gsmsdp_update_local_sdp_for_multicast";
    void           *local_sdp_p;
    void           *remote_sdp_p;
    sdp_direction_e direction;
    char            addr_str[MAX_IPADDR_STR_LEN];
    uint16_t        level;
    char            *p_addr_str;

    level = media->level;
    local_sdp_p = dcb_p->sdp->src_sdp;
    remote_sdp_p = dcb_p->sdp->dest_sdp;

    GSM_DEBUG(DEB_L_C_F_PREFIX"%d %d %d\n",
			  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
			  portnum, level, initial_offer);

    direction = gsmsdp_get_remote_sdp_direction(dcb_p, media->level,
                                                &media->dest_addr);
    GSM_DEBUG(DEB_L_C_F_PREFIX"sdp direction: %d\n",
              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), direction);
    /*
     * Update Transmit SRTP transmit key any way to clean up the
     * tx condition that we may have offered prior.
     */
    gsmsdp_update_crypto_transmit_key(dcb_p, media, offer, initial_offer, 
                                      direction);

    gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, FALSE,
                                  media, media->transport);

    /*
     * Set local sdp direction same as on remote SDP for multicast
     */
    if ((direction == SDP_DIRECTION_RECVONLY) || (direction == SDP_DIRECTION_INACTIVE)) {
        if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
            (media->support_direction == SDP_DIRECTION_RECVONLY)) { 
            /*
             * Echo same direction back in our local SDP but set the direction
             * in DCB to recvonly so that LSM operations on rcv port work
             * without modification.
             */
        } else {
            direction = SDP_DIRECTION_INACTIVE;
            GSM_DEBUG(DEB_L_C_F_PREFIX"media line"
                      " does not support receive stream\n",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        }
        gsmsdp_set_local_sdp_direction(dcb_p, media, direction);
        media->direction_set = TRUE;
    } else {
        /*
         * return FALSE indicating error
         */
        return (FALSE);
    }

    /*
     * Set the ip addr to the multicast ip addr.
     */
    ipaddr2dotted(addr_str, &media->dest_addr);
    p_addr_str = strtok(addr_str, "[ ]");

    /*
     * Set the local SDP port number to match far ends port number.
     */
    (void) sdp_set_media_portnum(dcb_p->sdp->src_sdp, level, portnum);

    /*
     * c= line <network type><address type><connection address>
     */
    (void) sdp_set_conn_nettype(dcb_p->sdp->src_sdp, level, SDP_NT_INTERNET);
    (void) sdp_set_conn_addrtype(dcb_p->sdp->src_sdp, level, media->addr_type);
    (void) sdp_set_conn_address(dcb_p->sdp->src_sdp, level, p_addr_str);

    return (TRUE);
}

/*
 * gsmsdp_get_remote_avt_payload_type
 *
 * Description:
 *
 * Returns the AVT payload type of the given audio line in the specified SDP.
 *
 * Parameters:
 *
 * level - The media level of the SDP where the media attribute is to be added.
 * sdp_p - Pointer to the SDP whose AVT payload type is being searched.
 *
 */
static int
gsmsdp_get_remote_avt_payload_type (uint16_t level, void *sdp_p)
{
    uint16_t        i;
    uint16_t        ptype;
    int             remote_avt_payload_type = RTP_NONE;
    uint16_t        num_a_lines = 0;
    const char     *encname = NULL;

    /*
     * Get number of RTPMAP attributes for the media line
     */
    (void) sdp_attr_num_instances(sdp_p, level, 0, SDP_ATTR_RTPMAP,
                                  &num_a_lines);

    /*
     * Loop through AUDIO media line RTPMAP attributes. The last
     * NET dynamic payload type will be returned.
     */
    for (i = 0; i < num_a_lines; i++) {
        ptype = sdp_attr_get_rtpmap_payload_type(sdp_p, level, 0,
                                                 (uint16_t) (i + 1));
        if (sdp_media_dynamic_payload_valid(sdp_p, ptype, level)) {
            encname = sdp_attr_get_rtpmap_encname(sdp_p, level, 0,
                                                  (uint16_t) (i + 1));
            if (encname) {
                if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_TEL_EVENT) == 0) {
                    remote_avt_payload_type = ptype;
                }
            }
        }
    }
    return (remote_avt_payload_type);
}


#define MIX_NEAREND_STRING  "X-mix-nearend"

/*
 *  gsmsdp_negotiate_codec
 *
 *  Description:
 *
 *  Negotiates an acceptable codec from the local and remote SDPs
 *
 *  Parameters:
 *
 *  dcb_p - Pointer to DCB whose codec is being negotiated
 *  sdp_p - Pointer to local and remote SDP
 *  media - Pointer to the fsmdef_media_t for a given media entry whose
 *          codecs are being negotiated.
 *  offer - Boolean indicating if the remote SDP came in an OFFER.
 *
 *  Returns:
 *
 *  payload  >  0: negotiated payload
 *           <= 0: negotiation failed
 */
static int
gsmsdp_negotiate_codec (fsmdef_dcb_t *dcb_p, cc_sdp_t *sdp_p, 
                        fsmdef_media_t *media, boolean offer, boolean initial_offer)
{
    static const char fname[] = "gsmsdp_negotiate_codec";
    rtp_ptype       pref_codec = RTP_NONE;
    uint16_t        i;
    uint16_t        j;
    int            *master_list_p = NULL;
    int            *slave_list_p = NULL;
    DtmfOutOfBandTransport_t transport = DTMF_OUTOFBAND_NONE;
    int             avt_payload_type;
    uint16_t        num_types;
    uint16_t        num_local_types;
    uint16_t        num_master_types, num_slave_types;
    int             remote_media_types[CC_MAX_MEDIA_TYPES];
    int             local_media_types[CC_MAX_MEDIA_TYPES];
    sdp_payload_ind_e pt_indicator;
    uint32          ptime = 0;
    const char*     attr_label;
    uint16_t        level;
    boolean         explicit_reject = FALSE;
    
    if (!dcb_p || !sdp_p || !media) {
        return (RTP_NONE);
    }

    level = media->level;
    attr_label = sdp_attr_get_simple_string(sdp_p->dest_sdp,
                                            SDP_ATTR_LABEL, level, 0, 1);

    if (attr_label != NULL) {
        if (strcmp(attr_label, MIX_NEAREND_STRING) == 0) {
            dcb_p->session = WHISPER_COACHING;
        }
    }
    /*
     * Obtain list of payload types from the remote SDP
     */
    num_types = sdp_get_media_num_payload_types(sdp_p->dest_sdp, level);

    if (num_types > CC_MAX_MEDIA_TYPES) {
        num_types = CC_MAX_MEDIA_TYPES;
    }

    for (i = 0; i < num_types; i++) {
        remote_media_types[i] = sdp_get_media_payload_type(sdp_p->dest_sdp,
                                                           level,
                                                           (uint16_t) (i + 1),
                                                           &pt_indicator);
    }

    /*
     * Get codecs that the phone supports. Get all the codecs that the phone supports.
     */
    if (media->type == SDP_MEDIA_AUDIO) {
        num_local_types = sip_config_local_supported_codecs_get(
                                    (rtp_ptype *)local_media_types,
                                    CC_MAX_MEDIA_TYPES);
    } else if (media->type == SDP_MEDIA_VIDEO) {
        num_local_types = sip_config_video_supported_codecs_get( (rtp_ptype *)local_media_types,
                                                  CC_MAX_MEDIA_TYPES, offer);
    } else {
        GSM_DEBUG(DEB_L_C_F_PREFIX"unsupported media type %d\n", 
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), media->type);
        return (RTP_NONE);
    }

    /*
     * Set the AVT payload type to whatever value the farend wants to use,
     * but only if we have AVT turned on and the farend wants it
     * or if we are configured to always send it
     */
    config_get_value(CFGID_DTMF_OUTOFBAND, &transport, sizeof(transport));

    /*
     * Save AVT payload type for use by gsmsdp_compare_to_previous_sdp
     */
    media->previous_sdp.avt_payload_type = media->avt_payload_type;

    switch (transport) {
    case DTMF_OUTOFBAND_AVT:
        avt_payload_type = gsmsdp_get_remote_avt_payload_type(
                               media->level, sdp_p->dest_sdp);
        if (avt_payload_type > RTP_NONE) {
            media->avt_payload_type = avt_payload_type;
        } else {
            media->avt_payload_type = RTP_NONE;
        }
        break;

    case DTMF_OUTOFBAND_AVT_ALWAYS:
        avt_payload_type = gsmsdp_get_remote_avt_payload_type(
                               media->level, sdp_p->dest_sdp);
        if (avt_payload_type > RTP_NONE) {
            media->avt_payload_type = avt_payload_type;
        } else {
            /*
             * If we are AVT_ALWAYS and the remote end is not using AVT,
             * then send DTMF as out-of-band and use our configured
             * payload type.
             */
            config_get_value(CFGID_DTMF_AVT_PAYLOAD,
                             &media->avt_payload_type,
                             sizeof(media->avt_payload_type));

            GSM_DEBUG(DEB_L_C_F_PREFIX"AVT_ALWAYS forcing out-of-band DTMF, "
                      "payload_type = %d\n",
					  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
					  media->avt_payload_type);
        }
        break;

    case DTMF_OUTOFBAND_NONE:
    default:
        media->avt_payload_type = RTP_NONE;
        break;
    }

    /*
     * Find a matching codec in our local list with the remote list.
     * The local list was created with our preferred codec first in the list,
     * so this will ensure that we will match the preferred codec with the
     * remote list first, before matching other codecs.
     */
    pref_codec = sip_config_preferred_codec(); 
    if (pref_codec != RTP_NONE) {
        /*
         * If a preferred codec was configured and the platform
         * currently can do this codec, then it would be the
         * first element of the local_media_types because of the
         * logic in sip_config_local_supported_codec_get().
         */
        if (local_media_types[0] != pref_codec) {
            /*
             * preferred codec is configured but it is not avaible 
             * currently, treat it as there is no codec available.
             */
            pref_codec = RTP_NONE;
        }
    }
    if (pref_codec == RTP_NONE) { // for CUCM, pref_codec is hardcoded to be NONE.
        master_list_p = remote_media_types;
        slave_list_p = local_media_types;
        num_master_types = num_types;
        num_slave_types = num_local_types;
        GSM_DEBUG(DEB_L_C_F_PREFIX"Remote Codec list is Master\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
    } else {
        master_list_p = local_media_types;
        slave_list_p = remote_media_types;
        num_master_types = num_local_types;
        num_slave_types = num_types;
        GSM_DEBUG(DEB_L_C_F_PREFIX"Local Codec list is Master\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
    }

    /*
     * Save pay load type for use by gsmspd_compare_to_previous_sdp
     */
    media->previous_sdp.payload_type = media->payload;
    media->previous_sdp.local_payload_type = media->local_dynamic_payload_type_value;

    for (i = 0; i < num_master_types; i++) {
        for (j = 0; j < num_slave_types; j++) {
            if (GET_CODEC_TYPE(master_list_p[i]) == GET_CODEC_TYPE(slave_list_p[j])) {
                media->payload = GET_CODEC_TYPE(slave_list_p[j]);
                if (offer == TRUE) { // if remote SDP is an offer
                    /* we answer with same dynamic payload type value for a given dynamic payload type */   
                    if (master_list_p == remote_media_types) {
                        media->remote_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(master_list_p[i]);
                        media->local_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(master_list_p[i]);
                    } else {
                        media->remote_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(slave_list_p[j]);
                        media->local_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(slave_list_p[j]);
                    }
                } else { //if remote SDP is an answer
                      if (media->local_dynamic_payload_type_value == RTP_NONE ||
                          media->payload !=  media->previous_sdp.payload_type) {
                        /* If the the negotiated payload type is different from previous,
                           set it the local dynamic to payload type  as this is what we offered*/
                        media->local_dynamic_payload_type_value =  media->payload;
                    }
                    /* remote answer may not use the value that we offered for a given dynamic payload type */
                    if (master_list_p == remote_media_types) {
                        media->remote_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(master_list_p[i]);
                    } else {
                        media->remote_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(slave_list_p[j]);
                    }
                }

                if (media->type == SDP_MEDIA_AUDIO) {
                    ptime = sdp_attr_get_simple_u32(sdp_p->dest_sdp,
                                                SDP_ATTR_PTIME, level, 0, 1);
                    if (ptime != 0) {
                        media->packetization_period = (uint16_t) ptime;
                    }
                    if (media->payload == RTP_ILBC) {
                        media->mode = (uint16_t)sdp_attr_get_fmtp_mode_for_payload_type
                                                       (sdp_p->dest_sdp, level, 0,
                                                        media->remote_dynamic_payload_type_value);
                    }

                    GSM_DEBUG(DEB_L_C_F_PREFIX"codec= %d\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
                          media->payload);

                    return (media->payload);
                } else if (media->type == SDP_MEDIA_VIDEO) {
                    if ( media-> video != NULL ) {
                       vcmFreeMediaPtr(media->video);
                       media->video = NULL;
                    }

                  if ( vcmCheckAttribs(media->payload, sdp_p, level,
                                       &media->video) == FALSE ) {
                        GSM_DEBUG(DEB_L_C_F_PREFIX"codec= %d ignored - attribs not accepted\n", 
                             DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, 
                             dcb_p->call_id, fname), media->payload);
			explicit_reject = TRUE;
                       continue; // keep looking
                    }

                    // cache the negotiated profile_level and bandwidth 
                    media->previous_sdp.tias_bw = media->tias_bw;
                    media->tias_bw =  ccsdpGetBandwidthValue(sdp_p,level, 1);
                    if ( (attr_label = 
                        ccsdpAttrGetFmtpProfileLevelId(sdp_p,level,0,1)) != NULL ) {
                        media->previous_sdp.profile_level = media->profile_level;
                        sscanf(attr_label,"%x", &media->profile_level);
                    }

                    GSM_DEBUG(DEB_L_C_F_PREFIX"codec= %d\n",
                         DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, 
                         dcb_p->call_id, fname), media->payload);
                    return (media->payload);
                }
            }
        }
    }

    /*
     * CSCsv84705 - we could not negotiate a common codec because the local list is empty. This condition could happen 
     * when using g729 in locally mixed conference in which another call to vcm_get_codec_list() would return 0 or no codec.
     * So if this is a not an init offer, we should just go ahead and use the last negotiated codec if the remote list
     * matches with currently used. 
     */ 
    if (!initial_offer && explicit_reject == FALSE) {
        for (i = 0; i < num_types; i++) {
            if (media->payload == GET_CODEC_TYPE(remote_media_types[i])) {
                /*
                 * CSCta40560 - DSP runs out of bandwidth as the current API's do not consider the request is for 
                 * the same call. Need to update dynamic payload types for dynamic PT codecs. Would need to possibly 
                 * add other video codecs as support is added here.
                 */
                if ( (media->payload == RTP_H264_P1 || media->payload == RTP_H264_P0) && offer == TRUE )  {
                   media->remote_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(master_list_p[i]);
                   media->local_dynamic_payload_type_value = GET_DYN_PAYLOAD_TYPE_VALUE(master_list_p[i]);
                }
                GSM_DEBUG(DEB_L_C_F_PREFIX"local codec list was empty codec= %d local=%d remote =%d\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
                          media->payload, media->local_dynamic_payload_type_value, media->remote_dynamic_payload_type_value);
                return (media->payload);
            }
        }
    }

    return (RTP_NONE);
}

/*
 * gsmsdp_add_unsupported_stream_to_local_sdp
 *
 * Description:
 *
 * Adds a rejected media line to the local SDP. If there is already a media line at
 * the specified level, check to see if it matches the corresponding media line in the
 * remote SDP. If it does not, remove the media line from the local SDP so that
 * the corresponding remote SDP media line can be added. Note that port will be set
 * to zero indicating the media line is rejected.
 *
 * Parameters:
 *
 * scp_p - Pointer to the local and remote SDP.
 * level - The media line level being rejected.
 */
static void
gsmsdp_add_unsupported_stream_to_local_sdp (cc_sdp_t *sdp_p,
                                            uint16_t level)
{
    static const char fname[] = "gsmsdp_add_unsupported_stream_to_local_sdp";
    uint32_t          remote_pt;
    sdp_payload_ind_e remote_pt_indicator;
    cpr_ip_addr_t     addr;

    if (sdp_p == NULL) {
        GSM_ERR_MSG(GSM_F_PREFIX"sdp is null.\n", fname);
        return;
    }

    if (sdp_get_media_type(sdp_p->src_sdp, level) != SDP_MEDIA_INVALID) {
        sdp_delete_media_line(sdp_p->src_sdp, level);
    }

    if (sdp_p->dest_sdp == NULL) {
        GSM_ERR_MSG(GSM_F_PREFIX"no remote SDP available\n", fname);
        return;
    }

    /*
     * Insert media line at the specified level.
     */
    if (sdp_insert_media_line(sdp_p->src_sdp, level) != SDP_SUCCESS) {
        GSM_ERR_MSG(GSM_F_PREFIX"failed to insert a media line\n", fname);
        return;
    }

    /*
     * Set the attributes of the media line. Specify port = 0 to
     * indicate media line is rejected.
     */
    (void) sdp_set_media_type(sdp_p->src_sdp, level,
                              sdp_get_media_type(sdp_p->dest_sdp, level));
    (void) sdp_set_media_portnum(sdp_p->src_sdp, level, 0);
    (void) sdp_set_media_transport(sdp_p->src_sdp, level,
                    sdp_get_media_transport(sdp_p->dest_sdp, level));

    remote_pt = sdp_get_media_payload_type(sdp_p->dest_sdp, level, 1,
                                           &remote_pt_indicator);
    /*
     * Don't like having to cast the payload type but sdp_get_media_payload_type
     * returns a uint32_t but sdp_add_media_payload_type takes a uint16_t payload type.
     * This needs to be fixed in Rootbeer.
     */
    (void) sdp_add_media_payload_type(sdp_p->src_sdp, level,
                                      (uint16_t) remote_pt,
                                      remote_pt_indicator);
    /*
     * The rejected media line needs to have "c=" line since
     * we currently do not include the "c=" at the session level.
     * The sdp parser in other end point such as the SDP parser
     * in the CUCM and in the phone ensures that there
     * is at least one "c=" line that can be used with each media
     * line. Such the parser will flag unsupported media line without
     * "c=" in that media line and at the session as error.
     *
     * The solution to have "c=" at the session level and
     * omitting "c=" at the media level all together can also
     * resolve this problem. Since the phone is also supporting
     * ANAT group for IPV4/IPV6 offering therefore selecting
     * session level and determining not to include "c=" line
     * at the media level can become complex. For this reason, the
     * unsupported media line will have "c=" with 0.0.0.0 address instead.
     */
    addr.type  = CPR_IP_ADDR_IPV4;
    addr.u.ip4 = 0; 
    gsmsdp_set_connection_address(sdp_p->src_sdp, level, &addr); 
}

/*
 * gsmsdp_get_remote_media_address
 *
 * Description:
 *
 * Extract the remote address from the given sdp.
 *
 * Parameters:
 *
 * fcb_p - Pointer to the FCB containing thhe DCB whose media lines are
 *         being negotiated
 * sdp_p - Pointer to the the remote SDP
 * level - media line level.
 * dest_addr - pointer to the cpr_ip_addr_t structure to return
 *             remote address.
 *
 *  Returns:
 *  FALSE - fails.
 *  TRUE  - success.
 *
 */
static boolean
gsmsdp_get_remote_media_address (fsmdef_dcb_t *dcb_p,
                                 cc_sdp_t * sdp_p, uint16_t level,
                                 cpr_ip_addr_t *dest_addr)
{
   const char fname[] = "gsmsdp_get_remote_media_address";
    const char     *addr_str = NULL;
    int             dns_err_code;
    boolean         stat;

    *dest_addr = ip_addr_invalid;

    stat = sdp_connection_valid(sdp_p->dest_sdp, level);
    if (stat) {
        addr_str = sdp_get_conn_address(sdp_p->dest_sdp, level);
    } else {
        /* Address not at the media level. Try the session level. */
        stat = sdp_connection_valid(sdp_p->dest_sdp, SDP_SESSION_LEVEL);
        if (stat) {
            addr_str = sdp_get_conn_address(sdp_p->dest_sdp, SDP_SESSION_LEVEL);
        }
    }

    if (stat && addr_str) {
        /* Assume that this is dotted address */
        if (str2ip(addr_str, dest_addr) != 0) {
            /* It could be Fully Qualify DN, need to add DNS look up here */
            dns_err_code = dnsGetHostByName(addr_str, dest_addr, 100, 1);
            if (dns_err_code) {
                *dest_addr = ip_addr_invalid;
                stat = FALSE;
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"DNS remote address error %d"
                            " with media at %d\n", dcb_p->line, dcb_p->call_id, 
                            fname, dns_err_code, level);
            }
        }
    } else {
        /*
         * No address the media level or the session level.
         */
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"No remote address from SDP with at %d\n", 
                    dcb_p->line, dcb_p->call_id, fname, level);
    }
    /*
     * Convert the remote address to host address to be used. It was
     * found out that without doing so, the softphone can crash when
     * in attempt to setup remote address to transmit API of DSP
     * implementation on win32.
     */
    util_ntohl(dest_addr, dest_addr);
    return (stat);
}

/* Function     gsmsdp_is_multicast_address
 *
 * Inputs:      - IP Address
 *
 * Returns:     YES if is multicast address, no if otherwise
 *
 * Purpose:     This is a utility function that tests to see if the passed
 *      address is a multicast address.  It does so by verifying that the
 *      address is between 225.0.0.0 and 239.255.255.255.
 *
 * Note:        Addresses passed are not in network byte order.
 *
 * Note2:       Addresses between 224.0.0.0 and 224.255.255.225 are also multicast
 *      addresses, but 224.0.0.0 to 224.0.0.255 are reserved and it is recommended
 *      to start at 225.0.0.0.  We need to research to see if this is a reasonable
 *      restriction.
 *
 */
int
gsmsdp_is_multicast_address (cpr_ip_addr_t theIpAddress)
{
    if  (theIpAddress.type == CPR_IP_ADDR_IPV4) {
    /*
     * Address already in host format
     */
        if ((theIpAddress.u.ip4 >= MULTICAST_START_ADDRESS) &&
            (theIpAddress.u.ip4 <= MULTICAST_END_ADDRESS)) {
        return (TRUE);
    }
    } else { 
        //todo IPv6: Check IPv6 multicast address here.

    }
    return (FALSE);
}

/**
 *
 * The function assigns or associate the new media line in the
 * offered SDP to an entry in the media capability table.
 *
 * @param[in]dcb_p       - pointer to the fsmdef_dcb_t
 * @param[in]sdp_p       - pointer to cc_sdp_t that contains the retmote SDP.
 * @param[in]level       - uint16_t for media line level.
 *
 * @return           Pointer to the fsmdef_media_t if successfully
 *                   found the anat pair media line otherwise return NULL.
 *
 * @pre              (dcb not_eq NULL)
 * @pre              (sdp_p not_eq NULL)
 * @pre              (media not_eq NULL)
 */
static fsmdef_media_t*
gsmsdp_find_anat_media_line (fsmdef_dcb_t *dcb_p, cc_sdp_t *sdp_p, uint16_t level)
{
    fsmdef_media_t *anat_media = NULL;
    u32            group_id_1, group_id_2;
    u32            dst_mid, group_mid;
    uint16_t       num_group_lines= 0;
    uint16_t       num_anat_lines = 0;
    uint16_t       i;

    /*
     * Get number of ANAT groupings at the session level for the media line
     */
    (void) sdp_attr_num_instances(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);
                            
    for (i = 1; i <= num_group_lines; i++) {
         if (sdp_get_group_attr(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {                                  
             num_anat_lines++;
         }
    }

    for (i = 1; i <= num_anat_lines; i++) {
    
        dst_mid = sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, level, 0, 1);
        group_id_1 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 1);
        group_id_2 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 2);

        if (dst_mid == group_id_1) {
            GSMSDP_FOR_ALL_MEDIA(anat_media, dcb_p) {
                group_mid = sdp_attr_get_simple_u32(sdp_p->src_sdp,
                                                    SDP_ATTR_MID, (uint16_t) group_id_2, 0, 1);            
                if (group_mid == group_id_2) {
                    /* found a match */   
                    return (anat_media);
                }
            }
        } else if (dst_mid == group_id_2) {
            GSMSDP_FOR_ALL_MEDIA(anat_media, dcb_p) {
                group_mid = sdp_attr_get_simple_u32(sdp_p->src_sdp,
                                                    SDP_ATTR_MID, (uint16_t) group_id_1, 0, 1);            
                if (group_mid == group_id_1) {
                    /* found a match */   
                    return (anat_media);
                } 
            }
        }
    }
    return (anat_media);
}

/**
 *
 * The function validates if all the anat groupings
 * have the right number of ids and their media type
 * is not the same
 *
 * @param[in]sdp_p      - pointer to the cc_sdp_t.
 *
 * @return           TRUE - anat validation passes
 *                   FALSE - anat validation fails
 *
 * @pre              (dcb not_eq NULL)
 * @pre              (sdp_p not_eq NULL)
 */
static boolean
gsmsdp_validate_anat (cc_sdp_t *sdp_p)
{
    u16          i, num_group_id;
    u32          group_id_1, group_id_2;
    sdp_media_e  media_type_gid1, media_type_gid2;
    uint16_t     num_group_lines= 0;
    uint16_t     num_anat_lines = 0;

    /*
     * Get number of ANAT groupings at the session level for the media line
     */
    (void) sdp_attr_num_instances(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);
                            
    for (i = 1; i <= num_group_lines; i++) {
         if (sdp_get_group_attr(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {                                  
             num_anat_lines++;
         }
    }

    for (i = 1; i <= num_anat_lines; i++) {
         num_group_id = sdp_get_group_num_id (sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i); 
         if ((num_group_id <=0) || (num_group_id > 2)) {
             /* This anat line has zero or more than two grouping, this is invalid */
             return (FALSE);
         } else if (num_group_id == 2) {
            /* Make sure that these anat groupings are not of same type */
            group_id_1 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 1);
            group_id_2 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 2);
            media_type_gid1 = sdp_get_media_type(sdp_p->dest_sdp, (u16) group_id_1);
            media_type_gid2 = sdp_get_media_type(sdp_p->dest_sdp, (u16) group_id_2);
            if (media_type_gid1 != media_type_gid2) {
                /* Group id types do not match */
                return (FALSE);            
            }
            if (group_id_1 != sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, (u16) group_id_1, 0, 1)) {
                /* Group id does not match the mid at the corresponding line */
                return (FALSE);            
            }
            if (group_id_2 != sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, (u16) group_id_2, 0, 1)) {
                return (FALSE);            
            }                        
         }
    }

    return (TRUE);
}
    
/**
 *
 * The function validates if all the destination
 * Sdp m lines have mid values and if those mid values match
 * the source Sdp mid values
 *
 * @param[in]sdp_p      - pointer to the cc_sdp_t
 * @param[in]level      - uint16_t for media line level.
 *
 * @return           TRUE - mid validation passes
 *                   FALSE - mid validation fails
 *
 * @pre              (dcb not_eq NULL)
 * @pre              (sdp_p not_eq NULL)
 */
static boolean
gsmsdp_validate_mid (cc_sdp_t *sdp_p, uint16_t level)
{
    int32     src_mid, dst_mid;
    u16       i;
    uint16_t  num_group_lines= 0;
    uint16_t  num_anat_lines = 0;

    /*
     * Get number of ANAT groupings at the session level for the media line
     */
    (void) sdp_attr_num_instances(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);
                            
    for (i = 1; i <= num_group_lines; i++) {
         if (sdp_get_group_attr(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {                                  
             num_anat_lines++;
         }
    }


    if (num_anat_lines > 0) {
        dst_mid = sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, level, 0, 1);            
        if (dst_mid == 0) {
            return (FALSE);
        }
        if (sdp_get_group_attr(sdp_p->src_sdp, SDP_SESSION_LEVEL, 0, 1) == SDP_GROUP_ATTR_ANAT) {
            src_mid = sdp_attr_get_simple_u32(sdp_p->src_sdp, SDP_ATTR_MID, level, 0, 1);
            if (dst_mid != src_mid) {
                return (FALSE);
             }
        }
        
    }
    return (TRUE);
}
    
/**
 *
 * The function negotiates the type of the media lines
 * based on anat attributes and ipv4/ipv6 settinsg.
 *
 * @param[in]dcb_p      - pointer to the fsmdef_dcb_t
 * @param[in]media      - pointer to the fsmdef_media_t
 *
 * @return           TRUE - this media line can be kept
 *                   FALSE - this media line can not be kept
 *
 * @pre              (dcb not_eq NULL)
 * @pre              (sdp_p not_eq NULL)
 */
static boolean
gsmsdp_negotiate_addr_type (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_negotiate_addr_type";
    cpr_ip_type     media_addr_type;
    cpr_ip_mode_e   ip_mode;
    fsmdef_media_t  *group_media;
  
    media_addr_type = media->dest_addr.type;
    if ((media_addr_type != CPR_IP_ADDR_IPV4) && 
        (media_addr_type != CPR_IP_ADDR_IPV6)) {
        /* Unknown/unsupported address type */
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"address type is not IPv4 or IPv6\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return (FALSE);
    }
    ip_mode = platform_get_ip_address_mode();
    /*
     * find out whether this media line is part of an ANAT group or not.
     */
    group_media = gsmsdp_find_anat_pair(dcb_p, media);

    /*
     * It is possible that we have a media sink/source device that
     * attached to the phone, then we only accept IPV4 for these device.
     *
     * The code below is using FSM_MEDIA_F_SUPPORT_SECURITY as indication
     * whether this media line is mapped to the off board device or not.
     * When we get a better API to find out then use the better API than
     * checking the FSM_MEDIA_F_SUPPORT_SECURITY.
     */
    if (!FSM_CHK_FLAGS(media->flags, FSM_MEDIA_F_SUPPORT_SECURITY)) {
        if (media_addr_type != CPR_IP_ADDR_IPV4) {  
            /* off board device we do not allow other address type but IPV4 */
            GSM_DEBUG(DEB_L_C_F_PREFIX"offboard device does not support IPV6\n", 
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            return (FALSE);
        }

        /*
         * P2:
         * Need an API to get address from the off board device. For now,
         * use our local address.
         */
        if ((ip_mode == CPR_IP_MODE_DUAL) || (ip_mode == CPR_IP_MODE_IPV4)) {
            if (group_media != NULL) {
                /* 
                 * this media line is part of ANAT group, keep the previous
                 * one negotiated media line.
                 */
                return (FALSE);
            }
            gsmsdp_get_local_source_v4_address(media);
            return (TRUE);
        } 
        /* phone is IPV6 only mode */
        return (FALSE);
    }

    if (ip_mode == CPR_IP_MODE_DUAL) {
         /*
          * In dual mode, IPV6 is preferred address type. If there is an
          * ANAT then select the media line that has IPV6 address.
          */
         if (group_media == NULL) {
             /*
              * no pair media line found, this can be the first media
              * line negotiate. Keep this media line for now.
              */ 
             if (media_addr_type == CPR_IP_ADDR_IPV4) { 
                 gsmsdp_get_local_source_v4_address(media);
             } else {
                 gsmsdp_get_local_source_v6_address(media);
             }
             return (TRUE);
         }

         /*
          * Found a ANAT pair media structure that this media line 
          * is part of.
          */
         if (media_addr_type == CPR_IP_ADDR_IPV4) {
             /*
              * This media line is IPV4, keep the other line that have
              * been accepted before i.e. it shows up first therefore
              * the other one has preference. 
              */
             return (FALSE);
         } 

         /* This media line is IPV6 */
         if (group_media->src_addr.type == CPR_IP_ADDR_IPV4) { 
             /*
              * The previous media line part of ANAT group is IPV4. The
              * phone policy is to select IPV6 for media stream. Remove
              * the previous media line and keep this media line (IPV6).
              */
              gsmsdp_add_unsupported_stream_to_local_sdp(dcb_p->sdp,
                                                         group_media->level);
              gsmsdp_remove_media(dcb_p, group_media);
              /* set this media line source address to IPV6 */
              gsmsdp_get_local_source_v6_address(media);
              return (TRUE);
         }
         /*
          * keep the previous one is also IPV6, remove this one i.e. 
          * the one found has higher preferecne although this is not
          * a valid ANAT grouping.
          */  
         return (FALSE); 
    }

    /*
     * The phone is not in dual mode, the address type must be from the media
     * line must match the address type that the phone is supporting.
     */
    if ((ip_mode == CPR_IP_MODE_IPV6) && 
        (media_addr_type == CPR_IP_ADDR_IPV4)) {
        /* incompatible address type */
        return (FALSE);
    }
    if ((ip_mode == CPR_IP_MODE_IPV4) &&
        (media_addr_type == CPR_IP_ADDR_IPV6)) {
        /* incompatible address type */
        return (FALSE);
    }

    if (group_media != NULL) {
        /*
         * This meida line is part of an ANAT group, keep the previous
         *  media line and throw away this line.
         */
        return (FALSE);
    }

    /*
     * We have a compatible address type, set the source address based on 
     * the address type from the remote media line.
     */
    if (media_addr_type == CPR_IP_ADDR_IPV4) {
        gsmsdp_get_local_source_v4_address(media);
    } else {
        gsmsdp_get_local_source_v6_address(media);
    }
    /* keep this media line */
    return (TRUE);    
}

/**
 *
 * The function finds the best media capability that matches the offer
 * media line according to the media table specified.
 *
 * @param[in]dcb_p       - pointer to the fsmdef_dcb_t
 * @param[in]sdp_p       - pointer to cc_sdp_t that contains the retmote SDP.
 * @param[in]media       - pointer to the fsmdef_media_t.
 * @param[in]media_table - media table to use (global or session)
 *
 * @return     cap_index - the best match for the offer
 *
 * @pre              (dcb_p not_eq NULL)
 * @pre              (sdp_p not_eq NULL)
 * @pre              (media not_eq NULL)
 */
static uint8_t
gsmdsp_find_best_match_media_cap_index (fsmdef_dcb_t    *dcb_p,
                                        cc_sdp_t        *sdp_p,
                                        fsmdef_media_t  *media,
                                        media_table_e   media_table)
{
    const cc_media_cap_t *media_cap;
    uint8_t              cap_index, candidate_cap_index;
    boolean              srtp_fallback;
    sdp_direction_e      remote_direction, support_direction;
    sdp_transport_e      remote_transport;
    sdp_media_e          media_type;

    remote_transport = sdp_get_media_transport(sdp_p->dest_sdp, media->level);
    remote_direction = gsmsdp_get_remote_sdp_direction(dcb_p, media->level,
                                                       &media->dest_addr);
    srtp_fallback    = sip_regmgr_srtp_fallback_enabled(dcb_p->line); 
    media_type       = media->type;


    /*
     * Select the best suitable media capability entry that
     * match this media line.
     * 
     * The following rules are used:
     *
     * 1) rule out entry that is invalid or not enabled or with 
     *    different media type.
     * 2) rule out entry that has been used by other existing
     *    media line.
     * 
     * After the above rules applies look for the better match for
     * direction support and security support. The platform should
     * arrange the capability table in preference order with
     * higher prefered entry placed at the lower index in the table.
     */
    candidate_cap_index = CC_MAX_MEDIA_CAP;
    for (cap_index = 0; cap_index < CC_MAX_MEDIA_CAP; cap_index++) {
        /* Find the cap entry that has the same media type and enabled */
        if (media_table == MEDIA_TABLE_GLOBAL) {
            media_cap = &g_media_table.cap[cap_index];
        } else {
            media_cap = gsmsdp_get_media_cap_entry_by_index(cap_index,dcb_p);
        }
        if ((media_cap == NULL) || !media_cap->enabled ||
            (media_cap->type != media_type)) {
            /* does not exist, not enabled or not the same type */
            continue;
        }

        /* Check for already in used */
        if (gsmsdp_find_media_by_cap_index(dcb_p, cap_index) != NULL) {
            /* this capability entry has been used */
            continue;
        }

        /*
         * Check for security support. The rules below attempts to
         * use entry that support security unless there is no entry
         * and the SRTP fallback is enabled. If the remote offer is not
         * SRTP just ignore the supported security and proceed on i.e. 
         * any entry is ok.
         */
        if (remote_transport == SDP_TRANSPORT_RTPSAVP) {
            if (!media_cap->support_security && !srtp_fallback) {
                /*
                 * this entry does not support security and SRTP fallback
                 * is not enabled.
                 */
                continue;
            }
            if (!media_cap->support_security) {
                /*
                 * this entry is not support security but srtp fallback
                 * is enabled, it potentially can be used
                 */
                candidate_cap_index = cap_index;
            }
        }

        /*
         * Check for suitable direction support. The rules for matching
         * directions are not exact rules. Try to match the closely
         * offer as much as possible. This is the best we know. We can
         * not guess what the real capability of the offer may have or will
         * change in the future (re-invite).
         */ 
        support_direction = media_cap->support_direction;
        if (remote_direction == SDP_DIRECTION_INACTIVE) {
            if (support_direction != SDP_DIRECTION_SENDRECV) {
                /* prefer send and receive for inactive */
                candidate_cap_index = cap_index;
            }
        } else if (remote_direction == SDP_DIRECTION_RECVONLY) {
            if ((support_direction != SDP_DIRECTION_SENDRECV) &&
                (support_direction != SDP_DIRECTION_SENDONLY)) {
                /* incompatible direction */
                continue;
            } else if (support_direction != SDP_DIRECTION_SENDONLY) {
                candidate_cap_index = cap_index;
            }
        } else if (remote_direction == SDP_DIRECTION_SENDONLY) {
            if ((support_direction != SDP_DIRECTION_SENDRECV) &&
                (support_direction != SDP_DIRECTION_RECVONLY)) {
                /* incompatible direction */
                continue;
            } else if (support_direction != SDP_DIRECTION_RECVONLY) {
                candidate_cap_index = cap_index;
            }
        } else if (remote_direction == SDP_DIRECTION_SENDRECV) {
            if (support_direction != SDP_DIRECTION_SENDRECV) {
                candidate_cap_index = cap_index;
            }
        }

        if (candidate_cap_index == cap_index) {
            /* this entry is not exactly best match, try other ones */
            continue;
        }
        /* this is the first best match found, use it */
        break;
    }

    if (cap_index == CC_MAX_MEDIA_CAP) {
        if (candidate_cap_index != CC_MAX_MEDIA_CAP) {
            /* We have a candidate entry to use */
            cap_index = candidate_cap_index;
        }
    }

    return cap_index;
}

/**
 *
 * The function finds the best media capability that matches the offer
 * media line.
 *
 * @param[in]dcb_p       - pointer to the fsmdef_dcb_t
 * @param[in]sdp_p       - pointer to cc_sdp_t that contains the retmote SDP.
 * @param[in]media       - pointer to the fsmdef_media_t.
 *
 * @return           TRUE - successful assigning a capability entry
 *                          to the media line.
 *                   FALSE - failed to assign a capability entry to the
 *                           media line.
 *
 * @pre              (dcb_p not_eq NULL)
 * @pre              (sdp_p not_eq NULL)
 * @pre              (media not_eq NULL)
 */
static boolean
gsmsdp_assign_cap_entry_to_incoming_media (fsmdef_dcb_t    *dcb_p,
                                           cc_sdp_t        *sdp_p,
                                           fsmdef_media_t  *media)
{
    static const char fname[] = "gsmsdp_assign_cap_entry_to_incoming_media";
    const cc_media_cap_t *media_cap;
    uint8_t              cap_index;
    fsmdef_media_t       *anat_media;

    /*
     * Find an existing media line that this media line belongs to the
     * same media group. If found, the same cap_index will be used.
     */
    anat_media = gsmsdp_find_anat_media_line(dcb_p, sdp_p, media->level);
    if (anat_media != NULL) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(anat_media->cap_index, dcb_p);
        if (media_cap == NULL) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capability\n", 
                        dcb_p->line, dcb_p->call_id, fname);
            return (FALSE);
        }
        gsmsdp_set_media_capability(media, media_cap);
        /* found the existing media line in the same ANAT group */
        media->cap_index = anat_media->cap_index;
        return (TRUE);
    }


    cap_index  = gsmdsp_find_best_match_media_cap_index(dcb_p,
                                                        sdp_p,
                                                        media,
                                                        MEDIA_TABLE_SESSION);

    if (cap_index == CC_MAX_MEDIA_CAP) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"reached max streams supported or"
                      " no suitable media capability\n", 
                      dcb_p->line, dcb_p->call_id, fname);
            return (FALSE);
        }

    /* set the capabilities to the media and associate with it */ 
    media_cap = gsmsdp_get_media_cap_entry_by_index(cap_index,dcb_p);
    if (media_cap == NULL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media cap\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return (FALSE);
    }
    gsmsdp_set_media_capability(media, media_cap);

    /* override the direction for special feature */
    gsmsdp_feature_overide_direction(dcb_p, media);
    if (media->support_direction == SDP_DIRECTION_INACTIVE) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"feature overrides direction to inactive,"
                  " no capability assigned\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        return (FALSE);
    }

    media->cap_index = cap_index;
    GSM_DEBUG(DEB_L_C_F_PREFIX"assign media cap index %d\n", 
              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), cap_index);
    return (TRUE);
}
                                 
/**
 *
 * The function handles negotiate adding of a media line. 
 *
 * @param[in]dcb_p       - pointer to the fsmdef_dcb_t
 * @param[in]media_type  - media type. 
 * @param[in]level       - media line.
 * @param[in]remote_port - remote port
 * @param[in]offer       - boolean indicates offer or answer.
 *
 * @return           pointer to fsmdef_media_t if media is successfully
 *                   added or return NULL.
 *
 * @pre              (dcb_p not_eq NULL) 
 * @pre              (sdp_p not_eq NULL) 
 * @pre              (remote_addr not_eq NULL)
 */
static fsmdef_media_t *
gsmsdp_negotiate_add_media_line (fsmdef_dcb_t  *dcb_p, 
                                 sdp_media_e   media_type,
                                 uint16_t      level,
                                 uint16_t      remote_port,
                                 boolean       offer)
{ 
    static const char fname[] = "gsmsdp_negotiate_add_media_line";
    fsmdef_media_t       *media;

    if (remote_port == 0) {
        /*
         * This media line is new but marked as disbaled.
         */
        return (NULL);
    }

    if (!offer) {
        /*
         * This is not an offer, the remote end wants to add
         * a new media line in the answer.
         */
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"remote trying add media in answer SDP\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return (NULL);
    }

    /*
     * Allocate a new raw media structure but not filling completely yet.
     */
    media = gsmsdp_get_new_media(dcb_p, media_type, level);
    if (media == NULL) {
        /* unable to add another media */
        return (NULL);
    }

    /*
     * If this call is locally held, mark the media with local hold so
     * that the negotiate direction will have the correct direction.
     */
    if ((dcb_p->fcb->state == FSMDEF_S_HOLDING) ||  
        (dcb_p->fcb->state == FSMDEF_S_HOLD_PENDING)) {
        /* the call is locally held, set the local held status */
        FSM_SET_FLAGS(media->hold, FSM_HOLD_LCL);
    }
    return (media);
}

/**
 *
 * The function handles negotiate remove of a media line. Note the
 * removal of a media line does not actaully removed from the offer/answer
 * SDP.
 * 
 * @param[in]dcb_p    - pointer to the fsmdef_dcb_t
 * @param[in]media    - pointer to the fsmdef_media_t for the media entry
 *                      to deactivate.
 * @param[in]remote_port - remote port from the remote's SDP.
 * @param[in]offer    - boolean indicates offer or answer.
 *
 * @return           TRUE  - when line is inactive.
 *                   FALSE - when line remains to be further processed.
 *
 * @pre              (dcb not_eq NULL) and (media not_eq NULL)
 */
static boolean
gsmsdp_negotiate_remove_media_line (fsmdef_dcb_t *dcb_p,
                                    fsmdef_media_t *media,
                                    uint16_t remote_port,
                                    boolean offer)
{
    static const char fname[] = "gsmsdp_negotiate_remove_media_line";

    if (offer) {
        /* This is an offer SDP from the remote */
        if (remote_port != 0) {
            /* the remote quests media is not for removal */
            return (FALSE); 
        } 
        /* 
         * Remote wants to remove the media line or to keep the media line
         * disabled. Fall through.
         */
    } else {
        /* This is an answer SDP from the remote */
        if ((media->src_port != 0) && (remote_port != 0)) {
            /* the media line is not for removal */
            return (FALSE);  
        }
        /* 
         * There are 3 possible causes:
         * 1) our offered port is 0 and remote's port is 0
         * 2) our offered port is 0 and remote's port is not 0. 
         * 3) our offered port is not 0 and remote's port is 0.
         * 
         * In any of these cases, the media line will not be used.
         */
        if ((media->src_port == 0) && (remote_port != 0)) {
            /* we offer media line removal but the remote does not comply */
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"remote insists on keeping media line\n",
                        dcb_p->line, dcb_p->call_id, fname);
        }
    }

    /*
     * This media line is to be removed.
     */
    return (TRUE);
}

/*
 * gsmsdp_negotiate_media_lines
 *
 * Description:
 *
 * Walk down the media lines provided in the remote sdp. Compare each
 * media line to the corresponding media line in the local sdp. If
 * the media line does not exist in the local sdp, add it. If the media
 * line exists in the local sdp but is different from the remote sdp,
 * change the local sdp to match the remote sdp. If the media line
 * is an AUDIO format, negotiate the codec and update the local sdp
 * as needed. We will reject all media lines that are not AUDIO and we
 * will only accept the first AUDIO media line located.
 *
 * Parameters:
 *
 * fcb_p - Pointer to the FCB containing thhe DCB whose media lines are being negotiated
 * sdp_p - Pointer to the local and remote SDP
 * initial_offer - Boolean indicating if the remote SDP came in the first OFFER of this session
 * offer - Boolean indicating if the remote SDP came in an OFFER.
 *
 */
static cc_causes_t
gsmsdp_negotiate_media_lines (fsm_fcb_t *fcb_p, cc_sdp_t *sdp_p,
                             boolean initial_offer, boolean offer)
{
    static const char fname[] = "gsmsdp_negotiate_media_lines";
    cc_causes_t     cause = CC_CAUSE_OK;
    uint16_t        num_m_lines = 0;
    uint16_t        num_local_m_lines = 0;
    uint16_t        i = 0;
    sdp_media_e     media_type;
    fsmdef_dcb_t   *dcb_p = fcb_p->dcb;
    uint16_t        port;
    boolean         update_local_ret_value = TRUE;
    sdp_transport_e transport;
    uint16_t        crypto_inst;
    boolean         media_found = FALSE;
    cpr_ip_addr_t   remote_addr;
    boolean         new_media;
    sdp_direction_e video_avail = SDP_DIRECTION_INACTIVE;
    boolean        unsupported_line;
    fsmdef_media_t *media;
    uint8_t         cap_index;
    sdp_direction_e remote_direction;

    num_m_lines = sdp_get_num_media_lines(sdp_p->dest_sdp);
    if (num_m_lines == 0) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"no media lines found.\n", 
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        return CC_CAUSE_NO_MEDIA;
    }

    /*
     * Validate the anat values
     */
    if (gsmsdp_validate_anat(sdp_p) == FALSE) {
        /* Failed anat validation */
        GSM_DEBUG(DEB_L_C_F_PREFIX"failed anat validation\n", 
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        return (CC_CAUSE_NO_MEDIA);
    }

    /*
     * Process each media line in the remote SDP
     */
    for (i = 1; i <= num_m_lines; i++) {

        unsupported_line = FALSE; /* assume line will be supported */
        new_media        = FALSE;
        media            = NULL;
        media_type = sdp_get_media_type(sdp_p->dest_sdp, i);

        port = (uint16_t) sdp_get_media_portnum(sdp_p->dest_sdp, i);
        GSM_DEBUG(DEB_L_C_F_PREFIX"Port is %d at %d %d\n", 
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), 
				  port, i, initial_offer);

        switch (media_type) {
        case SDP_MEDIA_AUDIO:
        case SDP_MEDIA_VIDEO:
            /*
             * Get remote address before other negotiations process in case
             * the address 0.0.0.0 (old style hold) to be used 
             * for direction negotiation.
             */
            if (!gsmsdp_get_remote_media_address(dcb_p, sdp_p, i, 
                                             &remote_addr)) {
                /* failed to get the remote address */
                GSM_DEBUG(DEB_L_C_F_PREFIX"unable to get remote addr at %d\n", 
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                unsupported_line = TRUE;
                break;
            }

            /*
             * Find the corresponding media entry in the dcb to see
             * this has been negiotiated previously (from the
             * last offer/answer session).
             */ 
            media = gsmsdp_find_media_by_level(dcb_p, i);
            if (media == NULL) {
                /* No previous media, negotiate adding new media line. */
                media = gsmsdp_negotiate_add_media_line(dcb_p, media_type, i,
                                                        port, offer);
                if (media == NULL) {
                    /* new one can not be added */
                    unsupported_line = TRUE;
                    break;
                }
                /*
                 * This media is a newly added, it is by itself an
                 * initial offer of this line.
                 */
                new_media = TRUE;
                GSM_DEBUG(DEB_L_C_F_PREFIX"new media entry at %d\n", 
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
            } else if (media->type == media_type) {
                /*
                 * Use the remote port to determine whether the
                 * media line is to be removed from the SDP.
                 */
                if (gsmsdp_negotiate_remove_media_line(dcb_p, media, port,
                                                       offer)) {
                    /* the media line is to be removed from the SDP */
                    unsupported_line = TRUE;
                    GSM_DEBUG(DEB_L_C_F_PREFIX"media at %d is removed\n", 
                              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                   break;
                }
            } else {
                /* The media at the same level but not the expected type */
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"mismatch media type at %d\n", 
                            dcb_p->line, dcb_p->call_id, fname, i);
                unsupported_line = TRUE;
                break;
            }

            /* Reset multicast flag and port */
            media->is_multicast = FALSE;
            media->multicast_port = 0;

            /* Update remote address */
            media->previous_sdp.dest_addr = media->dest_addr;
            media->dest_addr = remote_addr;

            /*
             * Associate the new media (for adding new media line) to
             * the capability table.
             */
            if (media->cap_index == CC_MAX_MEDIA_CAP) {
                if (!gsmsdp_assign_cap_entry_to_incoming_media(dcb_p, sdp_p,
                                                               media)) {
                    unsupported_line = TRUE;
                    GSM_DEBUG(DEB_L_C_F_PREFIX"unable to assign capability entry at %d\n", 
						DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                    // Check if we need to update the UI that video has been offered
                    if ( offer && media_type == SDP_MEDIA_VIDEO &&
                          ( ( g_media_table.cap[CC_VIDEO_1].support_direction != 
                                   SDP_DIRECTION_INACTIVE) )  ) {
                        // passed basic checks, now on to more expensive checks...
                        remote_direction = gsmsdp_get_remote_sdp_direction(dcb_p,
                                                                           media->level,
                                                                           &media->dest_addr);
                        cap_index        = gsmdsp_find_best_match_media_cap_index(dcb_p,
                                                                                  sdp_p,
                                                                                  media,
                                                                                  MEDIA_TABLE_GLOBAL);

                        GSM_DEBUG(DEB_L_C_F_PREFIX"remote_direction: %d global match %sfound\n", 
                            DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
                            remote_direction, (cap_index != CC_MAX_MEDIA_CAP)?"":"not ");
                        if ( cap_index != CC_MAX_MEDIA_CAP &&
                               remote_direction != SDP_DIRECTION_INACTIVE ) {
                           // this is an offer and platform can support video
                           GSM_DEBUG(DEB_L_C_F_PREFIX"\n\n\n\nUpdate video Offered Called %d\n", 
                                    DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), remote_direction);
                           lsm_update_video_offered(dcb_p->line, dcb_p->call_id, remote_direction);
                        }
                    }
                    break;
                }
            }

            /*
             * Negotiate address type and take only address type
             * that can be accepted.
             */
            if (gsmsdp_negotiate_addr_type(dcb_p, media) == FALSE) {
                unsupported_line = TRUE;
                break;
            }

            /*
             * Negotiate RTP/SRTP. The result is the media transport
             * which could be RTP/SRTP or fail.
             */
            transport = gsmsdp_negotiate_media_transport(dcb_p, sdp_p,
                                                         offer, media,
                                                         &crypto_inst);
            if (transport == SDP_TRANSPORT_INVALID) {
                /* unable to negotiate transport */
                unsupported_line = TRUE;
                GSM_DEBUG(DEB_L_C_F_PREFIX"transport mismatch at %d\n", 
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                break;
            }

            /*
             * Negotiate to a single codec
             */
            if (gsmsdp_negotiate_codec(dcb_p, sdp_p, media, offer, initial_offer) == 
                RTP_NONE) {
                /* unable to negotiate codec */
                unsupported_line = TRUE;
                /* Failed codec negotiation */
                cause = CC_CAUSE_PAYLOAD_MISMATCH;
                GSM_DEBUG(DEB_L_C_F_PREFIX"codec mismatch at %d\n", 
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                break;
            }

            /*
             * Both media transport (RTP/SRTP) and codec are
             * now negotiated to common ones, update transport
             * parameters to be used for SRTP, if there is any.
             */
            gsmsdp_update_negotiated_transport(dcb_p, sdp_p, media,
                                               crypto_inst, transport);
            GSM_DEBUG(DEB_F_PREFIX"local transport after updating negotiated: %d\n",DEB_F_PREFIX_ARGS(GSM, fname), sdp_get_media_transport(dcb_p->sdp->src_sdp, 1));
            /*
             * Add to or update media line to the local SDP as needed.
             */
            if (gsmsdp_is_multicast_address(media->dest_addr)) {
                /*
                 * Multicast, if the address is multicast
                 * then change the local sdp and do the necessary
                 * call to set up reception of multicast packets
                 */
                GSM_DEBUG(DEB_L_C_F_PREFIX"Got multicast offer\n", 
                         DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
                media->is_multicast = TRUE;
                media->multicast_port = port;
                update_local_ret_value = 
                     gsmsdp_update_local_sdp_for_multicast(dcb_p, port,
                                                           media, offer,
                                                           new_media);
            } else {
                update_local_ret_value = gsmsdp_update_local_sdp(dcb_p,
                                                                 offer,
                                                                 new_media,
                                                                 media);
            }
            GSM_DEBUG(DEB_F_PREFIX"local transport after updateing local SDP: %d\n",DEB_F_PREFIX_ARGS(GSM, fname), sdp_get_media_transport(dcb_p->sdp->src_sdp, 1));

            /* 
             * Successful codec negotiated cache direction for  ui video update
             */
            if (media_type == SDP_MEDIA_VIDEO ) {
                video_avail = media->direction;
            }

            if (update_local_ret_value == TRUE) {
                media->previous_sdp.dest_port = media->dest_port;
                media->dest_port = port;
                if (media_type == SDP_MEDIA_AUDIO) {
                    /* at least found one workable audio media line */
                    media_found = TRUE;
                }
            } else {
                /*
                 * Rejecting multicast because direction is not RECVONLY
                 */
                unsupported_line = TRUE;
                update_local_ret_value = TRUE;
            }
            break;
            
        default:
            /* Not a support media type stream */
            unsupported_line = TRUE;
            break;     
        }

        if (unsupported_line) { 
            /* add this line to unsupported line */
            gsmsdp_add_unsupported_stream_to_local_sdp(sdp_p, i);
            gsmsdp_set_mid_attr(sdp_p->src_sdp, i);
            /* Remove the media if one to be removed */
            if (media != NULL) {
                /* remove this media off the list */
                gsmsdp_remove_media(dcb_p, media);
            }
        }
        if (gsmsdp_validate_mid(sdp_p, i) == FALSE) {
             /* Failed mid validation */
            cause = CC_CAUSE_NO_MEDIA;
            GSM_DEBUG(DEB_L_C_F_PREFIX"failed mid validation at %d\n", 
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
        }            
    }

    /*
     * Must have at least one media found and at least one audio
     * line.
     */
    if (!media_found) { 
        if (cause != CC_CAUSE_PAYLOAD_MISMATCH) {
            cause = CC_CAUSE_NO_MEDIA;
        }
    } else {
        if (cause == CC_CAUSE_PAYLOAD_MISMATCH) {
            /*
             * some media lines have codec mismatch but there are some
             * that works, do not return error.
             */
            cause = CC_CAUSE_OK; 
        }

        /*
         * If we are processing an offer sdp, need to set the
         * start time and stop time based on the remote SDP
         */
        gsmsdp_update_local_time_stamp(dcb_p, offer, initial_offer);

        /*
         * workable media line was found. Need to make sure we don't 
         * advertise more than workable media lines. Loop through 
         * remaining media lines in local SDP and set port to zero.
         */
        num_local_m_lines = sdp_get_num_media_lines(sdp_p->src_sdp);
        if (num_local_m_lines > num_m_lines) {
            for (i = num_m_lines + 1; i <= num_local_m_lines; i++) {
                (void) sdp_set_media_portnum(sdp_p->src_sdp, i, 0);
            }
        }
    }
    /*
     * We have negotiated the line, clear flag that we have set
     * that we are waiting for an answer SDP in ack.
     */ 
    dcb_p->remote_sdp_in_ack = FALSE;

    /* 
     * check to see if UI needs to be updated for video
     */
    GSM_DEBUG(DEB_L_C_F_PREFIX"Update video Avail Called %d\n", 
               DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),video_avail);

    // update direction but preserve the cast attrib
    dcb_p->cur_video_avail &= CC_ATTRIB_CAST;
    dcb_p->cur_video_avail |= (uint8_t)video_avail;

    lsm_update_video_avail(dcb_p->line, dcb_p->call_id, dcb_p->cur_video_avail);

    return cause;
}

/*
 * gsmsdp_init_local_sdp
 *
 * Description:
 *
 * This function initializes the local sdp for generation of an offer sdp or
 * an answer sdp. The following sdp values are initialized.
 *
 * v= line
 * o= line <username><session id><version><network type><address type><address>
 * s= line
 * t= line <start time><stop time>
 *
 * Parameters:
 *
 * sdp_pp     - Pointer to the local sdp
 *
 * returns    cc_causes_t
 *            CC_CAUSE_OK - indicates success
 *            CC_CAUSE_ERROR - indicates failure
 */
static cc_causes_t
gsmsdp_init_local_sdp (cc_sdp_t **sdp_pp)
{
    char            addr_str[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t   ipaddr;
    unsigned long   session_id = 0;
    char            session_version_str[GSMSDP_VERSION_STR_LEN];
    void           *local_sdp_p = NULL;
    cc_sdp_t       *sdp_p = NULL;
    int             nat_enable = 0;
    char           *p_addr_str;
    cpr_ip_mode_e   ip_mode;
    int roapproxy;

    if (!sdp_pp) {
        return CC_CAUSE_ERROR;
    }

    ip_mode = platform_get_ip_address_mode();
    /*
     * Get device address. We will need this later.
     */
    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        if ((ip_mode == CPR_IP_MODE_DUAL) || (ip_mode == CPR_IP_MODE_IPV6)) {
            sip_config_get_net_ipv6_device_ipaddr(&ipaddr);
        } else if (ip_mode == CPR_IP_MODE_IPV4) {
            sip_config_get_net_device_ipaddr(&ipaddr);
        }
    } else {
        sip_config_get_nat_ipaddr(&ipaddr);
    }

    //<em>
    roapproxy = 0;
	config_get_value(CFGID_ROAPPROXY, &roapproxy, sizeof(roapproxy));
	if (roapproxy == TRUE) {
		strcpy(addr_str, gROAPSDP.offerAddress);
	} else {
		ipaddr2dotted(addr_str, &ipaddr);
	}


    p_addr_str = strtok(addr_str, "[ ]");

    /*
     * Create the local sdp struct
     */
    if (*sdp_pp == NULL) {
        sipsdp_src_dest_create(CCSIP_SRC_SDP_BIT, sdp_pp);
    } else {
        sdp_p = *sdp_pp;
        if (sdp_p->src_sdp != NULL) {
            sipsdp_src_dest_free(CCSIP_SRC_SDP_BIT, sdp_pp);
        }
        sipsdp_src_dest_create(CCSIP_SRC_SDP_BIT, sdp_pp);
    }
    sdp_p = *sdp_pp;

    if ( sdp_p == NULL )
       return CC_CAUSE_ERROR;

    local_sdp_p = sdp_p->src_sdp;

    /*
     * v= line
     */
    (void) sdp_set_version(local_sdp_p, SIPSDP_VERSION);

    /*
     * o= line <username><session id><version><network type>
     * <address type><address>
     */
    (void) sdp_set_owner_username(local_sdp_p, SIPSDP_ORIGIN_USERNAME);

    session_id = abs(cpr_rand() % 28457);
    snprintf(session_version_str, sizeof(session_version_str), "%d",
             (int) session_id);
    (void) sdp_set_owner_sessionid(local_sdp_p, session_version_str);

    snprintf(session_version_str, sizeof(session_version_str), "%d", 0);
    (void) sdp_set_owner_version(local_sdp_p, session_version_str);

    (void) sdp_set_owner_network_type(local_sdp_p, SDP_NT_INTERNET);

    if ((ip_mode == CPR_IP_MODE_DUAL) || (ip_mode == CPR_IP_MODE_IPV6)) {
        (void) sdp_set_owner_address_type(local_sdp_p, SDP_AT_IP6);
    } else if (ip_mode == CPR_IP_MODE_IPV4) {
       (void) sdp_set_owner_address_type(local_sdp_p, SDP_AT_IP4);
    }
    (void) sdp_set_owner_address(local_sdp_p, p_addr_str);

    /*
     * s= line
     */
    (void) sdp_set_session_name(local_sdp_p, SIPSDP_SESSION_NAME);

    /*
     * t= line <start time><stop time>
     * We init these to zero. If we are building an answer sdp, these will
     * be reset from the offer sdp.
     */
    (void) sdp_set_time_start(local_sdp_p, "0");
    (void) sdp_set_time_stop(local_sdp_p, "0");

    return CC_CAUSE_OK;
}

/**
 * The function sets the capabilities from media capability to the
 * media structure.
 *
 * @param[in]media     - pointer to the fsmdef_media_t to be set with
 *                       capabilites from the media_cap.
 * @param[in]media_cap - media capability to be used with this new media
 *                       line.
 *
 * @return           None.
 *
 * @pre              (media not_eq NULL)
 * @pre              (media_cap not_eq NULL)
 */
static void
gsmsdp_set_media_capability (fsmdef_media_t *media,
                             const cc_media_cap_t *media_cap)
{
    /* set default direction */
    media->direction = media_cap->support_direction;
    media->support_direction = media_cap->support_direction;
    if (media_cap->support_security) {
        /* support security */
        FSM_SET_FLAGS(media->flags, FSM_MEDIA_F_SUPPORT_SECURITY);
    }
}

/**
 * The function adds a media line into the local SDP.
 *
 * @param[in]dcb_p     - pointer to the fsmdef_dcb_t
 * @param[in]media_cap - media capability to be used with this new media
 *                       line. 
 * @param[in]cap_index - media capability entry index to associate with
 *                       the media line.
 * @param[in]level     - media line order in the SDP so called level.  
 * @param[in]addr_type - cpr_ip_type for address of the media line to add.
 *
 * @return           Pointer to the fsmdef_media_t if successfully
 *                   add a new line otherwise return NULL.
 *
 * @pre              (dcb_p not_eq NULL)
 * @pre              (media_cap not_eq NULL)
 */
static fsmdef_media_t * 
gsmsdp_add_media_line (fsmdef_dcb_t *dcb_p, const cc_media_cap_t *media_cap,
                       uint8_t cap_index, uint16_t level,
                       cpr_ip_type addr_type)
{
    static const char fname[] = "gsmsdp_add_media_line";
    cc_action_data_t  data;
    fsmdef_media_t   *media = NULL;
    int roapproxy;

    switch (media_cap->type) {
    case SDP_MEDIA_AUDIO:
    case SDP_MEDIA_VIDEO:
        media = gsmsdp_get_new_media(dcb_p, media_cap->type, level);
        if (media == NULL) {
            /* should not happen */
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media entry available\n", 
                        dcb_p->line, dcb_p->call_id, fname);
            return (NULL);
        }

        /* set capabilities */
        gsmsdp_set_media_capability(media, media_cap);

        /* associate this media line to the capability entry */
        media->cap_index = cap_index; /* keep the media cap entry index */ 

        /* override the direction for special feature */
        gsmsdp_feature_overide_direction(dcb_p, media);
        if (media->support_direction == SDP_DIRECTION_INACTIVE) {
            GSM_DEBUG(DEB_L_C_F_PREFIX"feature overrides direction to inactive"
                      " no media added\n", 
					  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            gsmsdp_remove_media(dcb_p, media);
            return (NULL);
        }

        /*
         * Get the local RTP port. The src port will be set in the dcb
         * within the call to cc_call_action(CC_ACTION_OPEN_RCV)
         */
        data.open_rcv.is_multicast = FALSE;
        data.open_rcv.listen_ip = ip_addr_invalid;
        data.open_rcv.port = 0;
        data.open_rcv.keep = FALSE;
        /*
         * Indicate type of media (audio/video etc) becase some for
         * supporting video over vieo, the port is obtained from other
         * entity.
         */
        data.open_rcv.media_type = media->type;
        data.open_rcv.media_refid = media->refid;
        if (cc_call_action(dcb_p->call_id, dcb_p->line,
                           CC_ACTION_OPEN_RCV,
                           &data) != CC_RC_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"allocate rx port failed\n", 
                        dcb_p->line, dcb_p->call_id, fname);
            gsmsdp_remove_media(dcb_p, media);
            return (NULL);
        }

        /* allocate port successful, save the port */

        // <em>
        roapproxy = 0;
    	config_get_value(CFGID_ROAPPROXY, &roapproxy, sizeof(roapproxy));
    	if (roapproxy == TRUE) {
    		if (SDP_MEDIA_AUDIO == media->type)
    			media->src_port = gROAPSDP.audioPort;
    		else
    			media->src_port = gROAPSDP.videoPort;
    	} else {
    		media->src_port = data.open_rcv.port;
    	}

        /*
         * Setup the local soruce address.
         */
        if (addr_type == CPR_IP_ADDR_IPV6) {
            gsmsdp_get_local_source_v6_address(media);
        } else if (addr_type == CPR_IP_ADDR_IPV4) {
            gsmsdp_get_local_source_v4_address(media);
        } else {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"invalid IP address mode\n", 
                        dcb_p->line, dcb_p->call_id, fname);
            gsmsdp_remove_media(dcb_p, media);
            return (NULL);
        }
    
        /*
         * Initialize the media transport for RTP or SRTP (or do not thing
         * and leave to the gsmsdp_update_local_sdp_media to set default)
         */
        gsmsdp_init_sdp_media_transport(dcb_p, dcb_p->sdp->src_sdp, media); 
       

            gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, TRUE, media,
                                          media->transport);


        gsmsdp_set_local_sdp_direction(dcb_p, media, media->direction);

        /*
         * Since we are initiating an initial offer and opening a
         * receive port, store initial media settings.
         */
        media->previous_sdp.avt_payload_type = media->avt_payload_type;
        media->previous_sdp.direction = media->direction;
        media->previous_sdp.packetization_period = media->packetization_period;
        media->previous_sdp.payload_type = media->payload;
        media->previous_sdp.local_payload_type = media->payload;
        break;

    default:
        /* Unsupported media type, not added */
        GSM_DEBUG(DEB_L_C_F_PREFIX"media type %d is not supported\n", 
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), media_cap->type);   
        break;
    }
    return (media);
}

/*
 * gsmsdp_create_local_sdp
 *
 * Description:
 *
 * Parameters:
 *
 * dcb_p - Pointer to the DCB whose local SDP is to be updated.
 *
 * returns    cc_causes_t
 *            CC_CAUSE_OK - indicates success
 *            CC_CAUSE_ERROR - indicates failure
 */
cc_causes_t
gsmsdp_create_local_sdp (fsmdef_dcb_t *dcb_p)
{
    static const char fname[] = "gsmsdp_create_local_sdp";
    uint16_t        level;
    const cc_media_cap_table_t *media_cap_tbl; 
    const cc_media_cap_t       *media_cap;
    cpr_ip_mode_e   ip_mode;    
    uint8_t         cap_index;
    fsmdef_media_t  *media;
    boolean         has_audio;

    if ( CC_CAUSE_OK != gsmsdp_init_local_sdp(&(dcb_p->sdp)) )
      return CC_CAUSE_ERROR;

    dcb_p->src_sdp_version = 0;

    media_cap_tbl = dcb_p->media_cap_tbl;

    if (media_cap_tbl == NULL) {
        /* should not happen */
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capbility available\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return (CC_CAUSE_ERROR);
    }

    media_cap = &media_cap_tbl->cap[0];
    level = 0;
    for (cap_index = 0; cap_index < CC_MAX_MEDIA_CAP; cap_index++) {
        /*
         * Add each enabled media line to the SDP
         */
        if (media_cap->enabled) {
            level = level + 1;  /* next level */ 
            ip_mode = platform_get_ip_address_mode();
            if (ip_mode >= CPR_IP_MODE_IPV6) {
                if (gsmsdp_add_media_line(dcb_p, media_cap, cap_index,
                                          level, CPR_IP_ADDR_IPV6) 
                    == NULL) {
                    /* fail to add a media line, go back one level */
                    level = level - 1;
                }
                
                if (ip_mode == CPR_IP_MODE_DUAL) {
                    level = level + 1;  /* next level */
                    if (gsmsdp_add_media_line(dcb_p, media_cap, cap_index, 
                                              level, CPR_IP_ADDR_IPV4) ==
                        NULL) {
                        /* fail to add a media line, go back one level */
                        level = level - 1;
                    } 
                }
            } else {
                if (gsmsdp_add_media_line(dcb_p, media_cap, cap_index, level,
                                          CPR_IP_ADDR_IPV4) == NULL) {
                    /* fail to add a media line, go back one level */
                    level = level - 1;
                }
            }
        }
        /* next capability */
        media_cap++;
    }

    if (level == 0) {
        /*
         * Did not find media line for the SDP and we do not 
         * support SDP without any media line.
         */
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media line for SDP\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return (CC_CAUSE_ERROR); 
    }

    /*
     * Ensure that there is at least one audio line.
     */
    has_audio = FALSE;
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->type == SDP_MEDIA_AUDIO) { 
            has_audio = TRUE; /* found one audio line, done */
            break;
        }
    }
    if (!has_audio) {
        /* No audio, do not allow */
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no audio media line for SDP\n", 
                    dcb_p->line, dcb_p->call_id, fname);
        return (CC_CAUSE_ERROR);
    }
    
    return CC_CAUSE_OK;
}

/**
 * The function creates a SDP that contains phone's current
 * capability for an option.
 *
 * @param[in/out]sdp_pp     - pointer to a pointer to cc_sdp_t to return
 *                            the created SDP.
 * @return                  none.
 * @pre              (sdp_pp not_eq NULL)
 */
void
gsmsdp_create_options_sdp (cc_sdp_t ** sdp_pp)
{
    cc_sdp_t *sdp_p;


    if (gsmsdp_init_local_sdp(sdp_pp) == CC_CAUSE_ERROR) {
        return;
    }

    sdp_p = *sdp_pp;

    /*
     * Insert media line at level 1.
     */
    if (sdp_insert_media_line(sdp_p->src_sdp, 1) != SDP_SUCCESS) {
        // Error
        return;
    }

    (void) sdp_set_media_type(sdp_p->src_sdp, 1, SDP_MEDIA_AUDIO);
    (void) sdp_set_media_portnum(sdp_p->src_sdp, 1, 0);
    gsmsdp_set_media_transport_for_option(sdp_p->src_sdp, 1);

    /*
     * Add all supported media formats to the local sdp.
     */
    gsmsdp_add_default_audio_formats_to_local_sdp(NULL, sdp_p, NULL);

    /* Add Video m line if video caps are enabled */
    if ( g_media_table.cap[CC_VIDEO_1].enabled == TRUE ) {
        if (sdp_insert_media_line(sdp_p->src_sdp, 2) != SDP_SUCCESS) {
            // Error
            return;
        }

        (void) sdp_set_media_type(sdp_p->src_sdp, 2, SDP_MEDIA_VIDEO);
        (void) sdp_set_media_portnum(sdp_p->src_sdp, 2, 0);
        gsmsdp_set_media_transport_for_option(sdp_p->src_sdp, 2);

        gsmsdp_add_default_video_formats_to_local_sdp(NULL, sdp_p, NULL);
    }
}

/**
 * The function checks and removes media capability for the media 
 * lines that is to be removed.
 *
 * @param[in]dcb_p   - Pointer to DCB
 *
 * @return           TRUE  - if there is a media line removed. 
 *                   FALSE - if there is no media line to remove.
 *
 * @pre              (dcb_p not_eq NULL)
 */
static boolean
gsmsdp_check_remove_local_sdp_media (fsmdef_dcb_t *dcb_p)
{
    static const char fname[] = "gsmsdp_check_remove_local_sdp_media";
    fsmdef_media_t             *media, *media_to_remove;
    const cc_media_cap_t       *media_cap;
    boolean                    removed = FALSE;
 
    media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
    while (media) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(media->cap_index,dcb_p);
        if (media_cap != NULL) {
            /* found the corresponding capability of the media line */
            if (!media_cap->enabled) {
                GSM_DEBUG(DEB_L_C_F_PREFIX"remove media at level %d\n", 
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), media->level);
                /* set the media line to unused */
                gsmsdp_add_unsupported_stream_to_local_sdp(dcb_p->sdp,
                                                           media->level);
                /*
                 * remember the media to remove and get the next media to
                 * work on before removing this media off the linked list.
                 */
                media_to_remove = media;
                media = GSMSDP_NEXT_MEDIA_ENTRY(media);

                /* remove the media from the list */
                gsmsdp_remove_media(dcb_p, media_to_remove);
                removed = TRUE;
                continue;
            }
        }
        media = GSMSDP_NEXT_MEDIA_ENTRY(media);
    }
    return (removed);
}

/**
 * The function checks and adds media capability for the media
 * lines that is to be added.
 * 
 * @param[in]dcb_p   - Pointer to DCB
 * @param[in]hold    - TRUE indicates the newly media line
 *                     should have direction that indicates hold.
 *
 * @return           TRUE  - if there is a media line added.
 *                   FALSE - if there is no media line to added.
 *
 * @pre              (dcb_p not_eq NULL)
 */
static boolean
gsmsdp_check_add_local_sdp_media (fsmdef_dcb_t *dcb_p, boolean hold)
{
    static const char fname[] = "gsmsdp_check_add_local_sdp_media";
    fsmdef_media_t             *media;
    const cc_media_cap_t       *media_cap;
    uint8_t                    cap_index;
    uint16_t                   num_m_lines, level_to_use;
    void                       *src_sdp;
    boolean                    need_mix = FALSE;
    boolean                    added = FALSE;
    cpr_ip_mode_e              ip_mode;
    cpr_ip_type                ip_addr_type[2]; /* for 2 IP address types */
    uint16_t                   i, num_ip_addrs;

    if (fsmcnf_get_ccb_by_call_id(dcb_p->call_id) != NULL) {
        /* 
         * This call is part of a local conference. The mixing
         * support will be needed for additional media line. 
         * If platform does not have capability to support mixing
         * of a particular media type for the local conference, either
         * leg in the conference will not see addition media line
         * added.
         */ 
        need_mix = TRUE;
    }

    /*
     * Find new media entries to be added.
     */
    src_sdp = dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL;
    for (cap_index = 0; cap_index < CC_MAX_MEDIA_CAP; cap_index++) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(cap_index, dcb_p);
        if (media_cap == NULL) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capbility available\n", 
                        dcb_p->line, dcb_p->call_id, fname);
            continue;
        }
        if (!media_cap->enabled) {
            /* this entry is disabled, skip it */
            continue;
        }
        media = gsmsdp_find_media_by_cap_index(dcb_p, cap_index);
        if (media != NULL) {
            /* this media entry exists, skip it */
            continue;
        }

        /* 
         * This is a new entry the capability table to be added.
         */
        if (CC_IS_AUDIO(cap_index) && need_mix) {
            if (!gsmsdp_platform_addition_mix(dcb_p, media_cap->type)) {
                /* platform can not support additional mixing of this type */
                GSM_DEBUG(DEB_L_C_F_PREFIX"no support addition mixing for %d "
                          "media type\n", 
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), 
						  media_cap->type);
                continue;
            }
        }

        /*
         * It depends on current address mode, if the mode is dual mode then
         * we need to add 2 media lines.
         */
        ip_mode = platform_get_ip_address_mode();
        switch (ip_mode) {
        case CPR_IP_MODE_DUAL:
            /* add both addresses, IPV6 line is first as it is prefered */
            num_ip_addrs = 2;
            ip_addr_type[0] = CPR_IP_ADDR_IPV6;
            ip_addr_type[1] = CPR_IP_ADDR_IPV4;
            break;
        case CPR_IP_MODE_IPV6:
            /* add IPV6 address only */
            num_ip_addrs = 1;
            ip_addr_type[0] = CPR_IP_ADDR_IPV6;
            break;
        default:
            /* add IPV4 address only */
            num_ip_addrs = 1;
            ip_addr_type[0] = CPR_IP_ADDR_IPV4;
            break;
        } 
        /* add media line or lines */
        for (i = 0; i < num_ip_addrs; i++) {
            /*
             * This is a new stream to add, find an unused media line
             * in the SDP. Find the unused media line that has the same
             * media type as the one to be added. The RFC-3264 allows reuse
             * any unused slot in the SDP body but it was recomended to use
             * the same type to increase the chance of interoperability by
             * using the unuse slot that has the same media type.
             */
            level_to_use = gsmsdp_find_unused_media_line_with_type(src_sdp,
                               media_cap->type);
            if (level_to_use == 0) {
                /* no empty slot is found, add a new line to the SDP */
                num_m_lines  = sdp_get_num_media_lines(src_sdp);
                level_to_use = num_m_lines + 1;
            }
            GSM_DEBUG(DEB_L_C_F_PREFIX"add media at level %d\n", 
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), level_to_use);

            /* add a new media */
            media = gsmsdp_add_media_line(dcb_p, media_cap, cap_index,
                                          level_to_use, ip_addr_type[i]);
            if (media != NULL) {
                /* successfully add a new media line */
                if (hold) {
                    /* this new media needs to be sent out with hold */
                    gsmsdp_set_local_hold_sdp(dcb_p, media);
                }
                added = TRUE;
            } else {
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"Unable to add a new media\n",
                            dcb_p->line, dcb_p->call_id, fname);
            }
        }
    }
    return (added);
}

/**
 * The function checks support direction changes and updates the support 
 * direction of media lines.
 *
 * @param[in]dcb_p         - Pointer to DCB
 * @param[in]no_sdp_update - TRUE indicates do not update SDP.
 * 
 * @return           TRUE  - if there is a media line support direction
 *                           changes.
 *                   FALSE - if there is no media line that has
 *                           support direction change.
 *
 * @pre              (dcb_p not_eq NULL)
 */
static boolean
gsmsdp_check_direction_change_local_sdp_media (fsmdef_dcb_t *dcb_p,
                                               boolean no_sdp_update)
{
    static const char fname[] = "gsmsdp_check_direction_change_local_sdp_media";
    fsmdef_media_t             *media;
    const cc_media_cap_t       *media_cap; 
    boolean                    direction_change = FALSE;
    sdp_direction_e            save_supported_direction;

    media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
    while (media) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(media->cap_index, dcb_p);
        if (media_cap != NULL) {
            if (media->support_direction !=
                media_cap->support_direction) {
                /*
                 * There is a possibility that supported direction has
                 * been overrided due to some feature. Check to see
                 * the supported direction remains the same after override
                 * take place. If it is different then there is a direction
                 * change.
                 */ 
                save_supported_direction = media->support_direction; 
                media->support_direction = media_cap->support_direction;
                gsmsdp_feature_overide_direction(dcb_p, media);  
                if (media->support_direction == save_supported_direction) { 
                    /* nothing change after override */
                } else {
                    /* there is no override, this is a change */
                    direction_change = TRUE;
                }
                if (direction_change) {
                    /* Support direction changed */
                    GSM_DEBUG(DEB_L_C_F_PREFIX"change support direction at level %d"
                              " from %d to %d\n", 
							  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), 
							  media->level, media->support_direction,
                              media_cap->support_direction);
                    if (no_sdp_update) {
                        /*
                         * The caller does not want to update SDP.
                         */
                        media->direction = media_cap->support_direction;
                    } else {
                        /* 
                         * Need to update direction in the SDP.
                         * The direction in the media structure will
                         * be set by the gsmsdp_set_local_sdp_direction.
                         */
                        gsmsdp_set_local_sdp_direction(dcb_p, media,
                                               media->support_direction);
                    }
                }
            }
        }
        media = GSMSDP_NEXT_MEDIA_ENTRY(media);
    }
    return (direction_change);
}

/**
 * The function resets media specified takes the hold state into account
 *
 * @param[in]dcb_p   - Pointer to DCB
 * @param[in]media   - Media to be updated
 * @param[in]hold    - Set media line will be set to hold.
 */
static void gsmsdp_reset_media(fsmdef_dcb_t *dcb_p, fsmdef_media_t *media, boolean hold){
    gsmsdp_reset_local_sdp_media(dcb_p, media, hold);
    if (hold) {
        gsmsdp_set_local_hold_sdp(dcb_p, media);
    } else {
        gsmsdp_set_local_resume_sdp(dcb_p, media);
    }
}

/**
 * 
 * This functions checks if the media IP Address has changed
 * 
 * Convert the configMedia IP String to Ip address format, 
 * check if it is valid and then compare to current media source
 * address, if the address is valid and the media source and
 * config media ip differ, then
 * 
 *  1) Stop the media to close the socket
 *  2) Set flag to true, to initiate re-invite
 *  3) Update the media src addr
 *  4) Update the c= line to reflect the new IP address source
 *
 * @param[in]dcb_p   - Pointer to DCB
 * @return         TRUE  - if IP changed
 *                 FALSE - if no no IP changed
 *
 * @pre              (dcb_p not_eq NULL)
 */
boolean 
gsmsdp_media_ip_changed (fsmdef_dcb_t *dcb_p)
{
    static const char     fname[] = "gsmsdp_media_ip_changed";
    boolean               ip_changed = FALSE;
    cpr_ip_addr_t         addr ;                
    char                  curr_media_ip[MAX_IPADDR_STR_LEN];
    char                  addr_str[MAX_IPADDR_STR_LEN];
    fsmdef_media_t        *media;

    /*
     * Check if media IP has changed
     */
    init_empty_str(curr_media_ip);
    config_get_value(CFGID_MEDIA_IP_ADDR, curr_media_ip, 
                        MAX_IPADDR_STR_LEN);
    if (is_empty_str(curr_media_ip) == FALSE) { 
        str2ip(curr_media_ip, &addr);
        util_ntohl(&addr, &addr);
        GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
            if ((util_check_if_ip_valid(&media->src_addr) == TRUE) &&
                (util_check_if_ip_valid(&addr) == TRUE) &&
                (util_compare_ip(&media->src_addr, &addr) == FALSE)) { 
                ipaddr2dotted(curr_media_ip, &media->src_addr); // for logging

                (void)cc_call_action(dcb_p->call_id, dcb_p->line,
                          CC_ACTION_STOP_MEDIA,
                          NULL);
                ip_changed = TRUE;
                media->src_addr = addr;
                if (dcb_p->sdp != NULL) {
                    gsmsdp_set_connection_address(dcb_p->sdp->src_sdp,
                            media->level,
                            &media->src_addr);
                } 
                ipaddr2dotted(addr_str, &media->src_addr);  // for logging
                GSM_ERR_MSG("%s MEDIA IP_CHANGED: after Update IP %s"\
                            " before %s" ,fname, addr_str, curr_media_ip );
            }
        }
    }

    return (ip_changed);
}

/**
 * this function will check whether the media ip addres in local sdp is same as to
 * media IP provided by application. If IP differs, then re-INVITE request is posted.
 */
boolean is_gsmsdp_media_ip_updated_to_latest( fsmdef_dcb_t * dcb ) {
    cpr_ip_addr_t         media_ip_in_host_order ;
    char                  curr_media_ip[MAX_IPADDR_STR_LEN];
    fsmdef_media_t        *media;

    init_empty_str(curr_media_ip);
    config_get_value(CFGID_MEDIA_IP_ADDR, curr_media_ip, MAX_IPADDR_STR_LEN);
    if (is_empty_str(curr_media_ip) == FALSE) {
        str2ip(curr_media_ip, &media_ip_in_host_order);
        util_ntohl(&media_ip_in_host_order, &media_ip_in_host_order);

        GSMSDP_FOR_ALL_MEDIA(media, dcb) {
            if (util_check_if_ip_valid(&media->src_addr) == TRUE) {
                if (util_compare_ip(&media->src_addr, &media_ip_in_host_order) == FALSE) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}


/**
 *
 * The function checks for media capability changes and updates the
 * local SDP for the changes. The function also provides a couple of options
 * 1)  reset the unchange media lines to initialize the codec list,
 * crypto etc. and 2) an option to update media directions for all
 * media lines for hold. 3)
 *
 * @param[in]dcb_p   - Pointer to DCB
 * @param[in]reset   - Reset the unchanged media lines to include all
 *                     codecs etc. again. 
 * @param[in]hold    - Set media line will be set to hold.
 *
 * @return         TRUE  - if media changes occur.
 *                 FALSE - if no media change occur.
 *
 * @pre              (dcb_p not_eq NULL)
 */
boolean
gsmsdp_update_local_sdp_media_capability (fsmdef_dcb_t *dcb_p, boolean reset,
                                          boolean hold)
{
    static const char     fname[] = "gsmsdp_update_local_sdp_media_capability";
    const cc_media_cap_table_t *media_cap_tbl;
    fsmdef_media_t             *media;
    boolean                    change_found = FALSE;
    boolean                    check_for_change = FALSE;

    change_found = gsmsdp_media_ip_changed(dcb_p);

    /*
     * check to see if media capability table has changed, by checking
     * the ID.
     */
    if ((g_media_table.id != dcb_p->media_cap_tbl->id) || reset) {

            /* 
             * capabilty table ID different or we are doing a reset for
             * the full offer again, need to check for various changes.
         * Update capabilities to match platform caps
         */
        media_cap_tbl = gsmsdp_get_media_capability(dcb_p);
            check_for_change = TRUE;
        }

    /*
     * Find any capability changes. The changes allowed are:
     * 1) media entry is disabled (to be removed).
     * 2) supported direction change.
     * 3) new media entry is enabled (to be added) keep it the last one.
     *
     * Find a media to be removed first so that if there is another
     * media line to add then there might be an unused a media line in the
     * SDP to use.
     */
    if (check_for_change && gsmsdp_check_remove_local_sdp_media(dcb_p)) {
        /* there were some media lines removed */
        change_found = TRUE;
    }

    /*
     * Find media lines that may have direction changes 
     */
    if ( check_for_change &&
         gsmsdp_check_direction_change_local_sdp_media(dcb_p, reset)) {
        /* there were media lines that directions changes */
        change_found = TRUE;
    }

    /*
     * Reset all the existing media lines to have full codec etc.
     * again if the caller requested.
     */
    if (reset) { 
        GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
            gsmsdp_reset_media(dcb_p, media, hold);
        }
    }

    /*
     * Find new media entries to be added.
     */
    if ( check_for_change && gsmsdp_check_add_local_sdp_media(dcb_p, hold)) { 
        change_found = TRUE;
    }

    if (change_found) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"media capability change found \n", 
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
    }

    
    /* report back any changes made */
    return (change_found);
}

/*
 * gsmsdp_reset_local_sdp_media
 *
 * Description:
 *
 * This function initializes the media portion of the local sdp. The following
 * lines are initialized.
 *
 * m= line <media><port><transport><fmt list>
 * a= session level line <media direction>
 *
 * Parameters:
 *
 * dcb_p - Pointer to DCB whose local SDP media is to be initialized
 * media - Pointer to fsmdef_media_t for the media entry of the SDP.
 *         If the value is NULL, it indicates all medie entries i the
 *         dcb is reset.
 * hold  - Boolean indicating if SDP is being initialized for sending hold
 *
 */
void
gsmsdp_reset_local_sdp_media (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media,
                              boolean hold)
{
    fsmdef_media_t *start_media, *end_media;

    if (media == NULL) {
        /* NULL value of the given media indicates for all media */
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
        end_media   = NULL; /* NULL means till the end of the list */
    } else {
        /* given media, uses the provided media */
        start_media = media;
        end_media   = media;
    }

    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }

        /*
         * Reset media transport in preparation for hold or resume.
         * It is possible that transport media may
         * change from the current media transport (for SRTP re-offer 
         * to a different end point).
         */
        gsmsdp_reset_sdp_media_transport(dcb_p, dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL,
                                             media, hold);

        gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, TRUE, media,
                                          media->transport);


        /*
         * a= line
         * If hold is being signaled, do not alter the media direction. This
         * will be taken care of by the state machine handler function.
         */
        if (!hold) {
            /*
             * We are not locally held, set direction to the supported
             * direction.
             */
            gsmsdp_set_local_sdp_direction(dcb_p, media,
                                           media->support_direction);
        }
    }
}

/*
 * gsmsdp_set_local_hold_sdp
 *
 * Description:
 *
 * Manipulates the local SDP of the specified DCB to indicate hold
 * to the far end.
 *
 * Parameters:
 *
 * dcb_p - Pointer to the DCB whose SDP is to be manipulated.
 * media - Pointer to the fsmdef_media_t for the current media entry.
 *         If the value is NULL, it indicates all medie entries i the
 *         dcb is reset.
 */
void
gsmsdp_set_local_hold_sdp (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    int             old_style_hold = 0;
    fsmdef_media_t *start_media, *end_media;

    if (media == NULL) {
        /* NULL value of the given media indicates for all media */
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
        end_media   = NULL; /* NULL means till the end of the list */
    } else {
        /* given media, uses the provided media */
        start_media = media;
        end_media   = media;
    }

    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }
        /*
         * Check if configuration indicates that the old style hold should be
         * signaled per RFC 2543. That is, c=0.0.0.0. Although
         * 2543 does not speak to the SDP direction attribute, we set
         * the direction to INACTIVE to be consistent with the connection
         * address setting.
         */
        config_get_value(CFGID_2543_HOLD, &old_style_hold,
                         sizeof(old_style_hold));
        if (old_style_hold) {
            gsmsdp_set_2543_hold_sdp(dcb_p, media->level);
            gsmsdp_set_local_sdp_direction(dcb_p, media,
                                           SDP_DIRECTION_INACTIVE);
        } else {
            /*
             * RFC3264 states that hold is signaled by setting the media
             * direction attribute to SENDONLY if in SENDRECV mode. 
             * INACTIVE if RECVONLY mode (mutual hold).
             */
            if (media->direction == SDP_DIRECTION_SENDRECV ||
                media->direction == SDP_DIRECTION_SENDONLY) {
                gsmsdp_set_local_sdp_direction(dcb_p, media, 
                                               SDP_DIRECTION_SENDONLY);
            } else {
                gsmsdp_set_local_sdp_direction(dcb_p, media, 
                                               SDP_DIRECTION_INACTIVE);
            }
        }
    }
}

/*
 * gsmsdp_set_local_resume_sdp
 *
 * Description:
 *
 * Manipulates the local SDP of the specified DCB to indicate hold
 * to the far end.
 *
 * Parameters:
 *
 * dcb_p - Pointer to the DCB whose SDP is to be manipulated.
 * media - Pointer to the fsmdef_media_t for the current media entry.
 *
 */
void
gsmsdp_set_local_resume_sdp (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    fsmdef_media_t *start_media, *end_media;

    if (media == NULL) {
        /* NULL value of the given media indicates for all media */
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
        end_media   = NULL; /* NULL means till the end of the list */
    } else {
        /* given media, uses the provided media */
        start_media = media;
        end_media   = media;
    }

    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }
        /*
         * We are not locally held, set direction to the supported 
         * direction.
         */
        gsmsdp_set_local_sdp_direction(dcb_p, media, media->support_direction);
    }
}

/*
 * gsmsdp_encode_sdp
 *
 * Description:
 * The function encodes SDP from the internal SDP representation
 * to the SDP body to be sent out.
 *
 * Parameters:
 * sdp_p    - pointer to the internal SDP info. block.
 * msg_body - pointer to the msg body info. block.
 *
 * Returns:
 * cc_causes_t to indicate failure or success.
 */
cc_causes_t
gsmsdp_encode_sdp (cc_sdp_t *sdp_p, cc_msgbody_info_t *msg_body)
{
    char           *sdp_body;
    cc_msgbody_t   *part;
    uint32_t        body_length;

    if (msg_body == NULL) {
        return CC_CAUSE_ERROR;
    }

    /* Support single SDP encoding for now */
    sdp_body = sipsdp_write_to_buf(sdp_p, &body_length);

    if (sdp_body == NULL) {
        return CC_CAUSE_ERROR;
    } else if (body_length == 0) {
        cpr_free(sdp_body);
        return CC_CAUSE_ERROR;
    }

    /* Clear off the bodies info */
    cc_initialize_msg_body_parts_info(msg_body);

    /* Set up for one SDP entry */
    msg_body->num_parts = 1;
    msg_body->content_type = cc_content_type_SDP;
    part = &msg_body->parts[0];
    part->body = sdp_body;
    part->body_length = body_length;
    part->content_type = cc_content_type_SDP;
    part->content_disposition.required_handling = FALSE;
    part->content_disposition.disposition = cc_disposition_session;
    part->content_id = NULL;
    return CC_CAUSE_OK;
}

/*
 * gsmsdp_encode_sdp_and_update_version
 *
 * Description:
 * The function encodes SDP from the internal SDP representation
 * to the SDP body to be sent out.  It also post-increments the owner
 * version number to prepare for the next SDP to be sent out.
 *
 * Parameters:
 * sdp_p    - pointer to DCB whose local SDP is to be encoded.
 * msg_body - pointer to the msg body info. block.
 *
 * Returns:
 * cc_causes_t to indicate failure or success.
 */
cc_causes_t
gsmsdp_encode_sdp_and_update_version (fsmdef_dcb_t *dcb_p, cc_msgbody_info_t *msg_body)
{
    char version_str[GSMSDP_VERSION_STR_LEN];

    snprintf(version_str, sizeof(version_str), "%d", dcb_p->src_sdp_version);

    if ( dcb_p->sdp == NULL || dcb_p->sdp->src_sdp == NULL )
    {
    	if ( CC_CAUSE_OK != gsmsdp_init_local_sdp(&(dcb_p->sdp)) )
    	{
    		return CC_CAUSE_ERROR;
    	}
    }
    (void) sdp_set_owner_version(dcb_p->sdp->src_sdp, version_str);

    if (gsmsdp_encode_sdp(dcb_p->sdp, msg_body) != CC_CAUSE_OK) {
        return CC_CAUSE_ERROR;
    }

    dcb_p->src_sdp_version++;
    return CC_CAUSE_OK;
}

/*
 * gsmsdp_get_sdp
 *
 * Description:
 * The function searches the SDP from the the all of msg. body parts.
 * All body parts having the content type SDP will be stored at the
 * given destination arrays. The number of SDP body are returned to
 * indicate the number of SDP bodies found. The function searches
 * the msg body in backward to order to form SDP in the destination array
 * to be from the highest to the lowest preferences.
 *
 * Parameters:
 * msg_body   - pointer to the incoming message body or cc_msgbody_info_t.
 * part_array - pointer to pointer of cc_msgbody_t to store the
 *              sorted order of the incoming SDP.
 * max_part   - the maximum number of SDP can be written to the
 *              part_array or the part_array size.
 * Returns:
 * The number of SDP parts found.
 */
static uint32_t
gsmsdp_get_sdp_body (cc_msgbody_info_t *msg_body,
                     cc_msgbody_t **part_array,
                     uint32_t max_parts)
{
    uint32_t      i, count;
    cc_msgbody_t *part;

    if ((msg_body == NULL) || (msg_body->num_parts == 0)) {
        /* No msg. body or no body parts in the msg. */
        return (0);
    }
    /*
     * Extract backward. The SDP are sent from the lowest
     * preference to the highest preference. The highest
     * preference will be extracted first into the given array
     * so that when we negotiate, the SDP we will attempt to
     * negotiate from the remote's highest to the lowest
     * preferences.
     */
    count = 0;
    part = &msg_body->parts[msg_body->num_parts - 1];
    for (i = 0; (i < msg_body->num_parts) && (i < max_parts); i++) {
        if (part->content_type == cc_content_type_SDP) {
            /* Found an SDP, keep the pointer to the part */
            *part_array = part; /* save pointer to SDP entry */
            part_array++;       /* next entry                */
            count++;
        }
        /* next one backward */
        part--;
    }
    /* return the number of SDP bodies found */
    return (count);
}

/*
 * gsmsdp_realloc_dest_sdp
 *
 * Description:
 * The function re-allocates the internal SDP info. block for the
 * remote or destination SDP. If there the SDP info. or the
 * SDP block does not exist, the new one is allocated. If SDP block
 * exists, the current one is released and is replaced by a new one.
 *
 * Parameters:
 * dcb_p - pointer to fsmdef_dcb_t.
 *
 * Returns:
 * cc_causes_t to indicate failure or success.
 */
static cc_causes_t
gsmsdp_realloc_dest_sdp (fsmdef_dcb_t *dcb_p)
{
    /* There are SDPs to process, prepare for parsing the SDP */
    if (dcb_p->sdp == NULL) {
        /* Create internal SDP information block with dest sdp block */
        sipsdp_src_dest_create(CCSIP_DEST_SDP_BIT, &dcb_p->sdp);
    } else {
        /*
         * SDP info. block exists, remove the previously received
         * remote or destination SDP and create a new one for
         * the new SDP.
         */
        if (dcb_p->sdp->dest_sdp) {
            sipsdp_src_dest_free(CCSIP_DEST_SDP_BIT, &dcb_p->sdp);
        }
        sipsdp_src_dest_create(CCSIP_DEST_SDP_BIT, &dcb_p->sdp);
    }

    /* No SDP info block and parsed control block are available */
    if ((dcb_p->sdp == NULL) || (dcb_p->sdp->dest_sdp == NULL)) {
        /* Unable to create internal SDP structure to parse SDP. */
        return CC_CAUSE_ERROR;
    }
    return CC_CAUSE_OK;
}

/*
 * gsmsdp_negotiate_answer_sdp
 *
 * Description:
 *
 * Interface function used to negotiate an ANSWER SDP.
 *
 * Parameters:
 *
 * fcb_p - Pointer to the FCB containing the DCB whose local SDP is being negotiated.
 * msg_body - Pointer to the cc_msgbody_info_t that contain the remote
 *
 */
cc_causes_t
gsmsdp_negotiate_answer_sdp (fsm_fcb_t *fcb_p, cc_msgbody_info_t *msg_body)
{
    static const char fname[] = "gsmsdp_negotiate_answer_sdp";
    fsmdef_dcb_t *dcb_p = fcb_p->dcb;
    cc_msgbody_t *sdp_bodies[CC_MAX_BODY_PARTS];
    uint32_t      i, num_sdp_bodies;
    cc_causes_t   status;
    char         *sdp_body;

    /* Get just the SDP bodies */
    num_sdp_bodies = gsmsdp_get_sdp_body(msg_body, &sdp_bodies[0],
                                         CC_MAX_BODY_PARTS);
    GSM_DEBUG(DEB_F_PREFIX"\n",DEB_F_PREFIX_ARGS(GSM, fname));
    if (num_sdp_bodies == 0) {
        /*
         * Clear the call - we don't have any remote SDP info!
         */
        return CC_CAUSE_ERROR;
    }

    /* There are SDPs to process, prepare for parsing the SDP */
    if (gsmsdp_realloc_dest_sdp(dcb_p) != CC_CAUSE_OK) {
        /* Unable to create internal SDP structure to parse SDP. */
        return CC_CAUSE_ERROR;
    }

    /*
     * Parse the SDP into internal structure,
     * now just parse one
     */
    status = CC_CAUSE_ERROR;
    for (i = 0; (i < num_sdp_bodies); i++) {
        if ((sdp_bodies[i]->body != NULL) && (sdp_bodies[i]->body_length > 0)) {
            /* Found a body */
            sdp_body = sdp_bodies[i]->body;
            if (sdp_parse(dcb_p->sdp->dest_sdp, &sdp_body,
                          (uint16_t)sdp_bodies[i]->body_length)
                    == SDP_SUCCESS) {
                status = CC_CAUSE_OK;
                break;
            }
        }
    }
    if (status != CC_CAUSE_OK) {
        /* Error parsing SDP */
        return status;
    }
 
    gsmsdp_set_remote_sdp(dcb_p, dcb_p->sdp);

    status = gsmsdp_negotiate_media_lines(fcb_p, dcb_p->sdp, FALSE, FALSE);
    GSM_DEBUG(DEB_F_PREFIX"returns with %d\n",DEB_F_PREFIX_ARGS(GSM, fname), status);
    return (status);
}

/*
 * gsmsdp_negotiate_offer_sdp
 *
 * Description:
 *
 * Interface function used to negotiate an OFFER SDP.
 *
 * Parameters:
 *
 * fcb_p - Pointer to the FCB containing the DCB whose local SDP is being negotiated.
 * msg_body - Pointer to remote SDP body infostructure.
 * init - Boolean indicating if the local SDP should be initialized as if this is the
 *        first local SDP of this session.
 *
 */
cc_causes_t
gsmsdp_negotiate_offer_sdp (fsm_fcb_t *fcb_p,
                           cc_msgbody_info_t *msg_body, boolean init)
{
    static const char fname[] = "gsmsdp_negotiate_offer_sdp";
    fsmdef_dcb_t *dcb_p = fcb_p->dcb;
    cc_causes_t   status;
    cc_msgbody_t *sdp_bodies[CC_MAX_BODY_PARTS];
    uint32_t      i, num_sdp_bodies;
    char         *sdp_body;

    /* Get just the SDP bodies */
    num_sdp_bodies = gsmsdp_get_sdp_body(msg_body, &sdp_bodies[0],
                                         CC_MAX_BODY_PARTS);
    GSM_DEBUG(DEB_L_C_F_PREFIX"Init is %d\n", 
		DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), init);
    if (num_sdp_bodies == 0) {
        /*
         * No remote SDP. So we will offer in our response and receive far end
         * answer in the ack. Only need to create local sdp if this is first offer
         * of a session. Otherwise, we will send what we have.
         */
        if (init) {
            if ( CC_CAUSE_OK != gsmsdp_create_local_sdp(dcb_p)) { 
                return CC_CAUSE_ERROR;
            }
        } else {
            /* 
             * Reset all media entries that we have to offer all capabilities
             */
           (void)gsmsdp_update_local_sdp_media_capability(dcb_p, TRUE, FALSE);
        }
        dcb_p->remote_sdp_in_ack = TRUE;
        return CC_CAUSE_OK;
    }

    /* There are SDPs to process, prepare for parsing the SDP */
    if (gsmsdp_realloc_dest_sdp(dcb_p) != CC_CAUSE_OK) {
        /* Unable to create internal SDP structure to parse SDP. */
        return CC_CAUSE_ERROR;
    }

    /*
     * Parse the SDP into internal structure,
     * now just parse one
     */
    status = CC_CAUSE_ERROR;
    for (i = 0; (i < num_sdp_bodies); i++) {
        if ((sdp_bodies[i]->body != NULL) && (sdp_bodies[i]->body_length > 0)) {
            /* Found a body */
            sdp_body = sdp_bodies[i]->body;
            if (sdp_parse(dcb_p->sdp->dest_sdp, &sdp_body,
                          (uint16_t)sdp_bodies[i]->body_length)
                    == SDP_SUCCESS) {
                status = CC_CAUSE_OK;
                break;
            }
        }
    }
    if (status != CC_CAUSE_OK) {
        /* Error parsing SDP */
        return status;
    }

    if (init) {
        (void)gsmsdp_init_local_sdp(&(dcb_p->sdp));
        /* Note that there should not a previous version here as well */
    } 

    gsmsdp_set_remote_sdp(dcb_p, dcb_p->sdp);

    /*
     * If a new error code has been added to sdp processing please make sure
     * the sip side is aware of it
     */
    status = gsmsdp_negotiate_media_lines(fcb_p, dcb_p->sdp, init, TRUE);
    return (status);
}

/*
 * gsmsdp_free
 *
 * Description:
 * The function frees SDP resources that were allocated during the
 * course of the call.
 *
 * Parameters:
 * dcb_p    - pointer to fsmdef_dcb_t.
 */
void
gsmsdp_free (fsmdef_dcb_t *dcb_p)
{
    if ((dcb_p != NULL) && (dcb_p->sdp != NULL)) {
        sipsdp_free(&dcb_p->sdp);
        dcb_p->sdp = NULL;
    }
}

/*
 * gsmsdp_sdp_differs_from_previous_sdp
 *
 * Description:
 *
 * Interface function used to compare newly received SDP to previously
 * received SDP. Returns TRUE if attributes of interest are the same.
 * Otherwise, FALSE is returned.
 *
 * Parameters:
 *
 * rcv_only - If TRUE, check for receive port perspective.
 * media    - Pointer to the fsmdef_media_t for the current media entry.
 */
boolean
gsmsdp_sdp_differs_from_previous_sdp (boolean rcv_only, fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_sdp_differs_from_previous_sdp";
    char    prev_addr_str[MAX_IPADDR_STR_LEN];
    char    dest_addr_str[MAX_IPADDR_STR_LEN];


    /*
     * Consider attributes of interest common to all active directions.
     */
    if ((media->previous_sdp.avt_payload_type != media->avt_payload_type) ||
        (media->previous_sdp.payload_type != media->payload)) {
        GSM_DEBUG(DEB_F_PREFIX"previous payload: %d new payload: %d\n",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.payload_type, media->payload);
        GSM_DEBUG(DEB_F_PREFIX"previous avt payload: %d new avt payload: %d\n",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.avt_payload_type,
                  media->avt_payload_type);
        return TRUE;
    }

    if ( (media->previous_sdp.local_payload_type != media->local_dynamic_payload_type_value) ) {
        GSM_DEBUG(DEB_F_PREFIX"previous dynamic payload: %d new dynamic payload: %d\n",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.local_payload_type, 
                               media->local_dynamic_payload_type_value);
        return TRUE;
    }

    /*
     * Consider attributes of interest for transmit directions.
     * If previous dest port is 0 then this is the first time
     * we received sdp for comparison. We treat this situation
     * as if addr and port did not change.
     */
    if ( (media->previous_sdp.dest_port != 0) && (rcv_only == FALSE)) {
        if ((util_compare_ip(&(media->previous_sdp.dest_addr),
                            &(media->dest_addr)) == FALSE) ||
            (media->previous_sdp.dest_port != media->dest_port)) {
        prev_addr_str[0] = '\0'; /* ensure valid string if convesion fails */
        dest_addr_str[0] = '\0';
        ipaddr2dotted(prev_addr_str, &media->previous_sdp.dest_addr);
        ipaddr2dotted(dest_addr_str, &media->dest_addr);
        GSM_DEBUG(DEB_F_PREFIX"previous address: %s new address: %s\n",
                  DEB_F_PREFIX_ARGS(GSM, fname), prev_addr_str, dest_addr_str);
        GSM_DEBUG(DEB_F_PREFIX"previous port: %d new port: %d\n",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.dest_port, media->dest_port);
            return TRUE;
        } else if ( media->tias_bw != media->previous_sdp.tias_bw) {
            GSM_DEBUG(DEB_F_PREFIX"previous bw: %d new bw: %d\n",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.tias_bw, media->tias_bw);
            return TRUE;
        } else if ( media->profile_level != media->previous_sdp.profile_level) {
            GSM_DEBUG(DEB_F_PREFIX"previous prof_level: %X new prof_level: %X\n",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.profile_level, media->profile_level);
            return TRUE;
        }
    }


    /* Check crypto parameters if we are doing SRTP */
    if (gsmsdp_crypto_params_change(rcv_only, media)) {
        return TRUE;
    }
    return FALSE;
}

