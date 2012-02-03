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

/*
 *     This module provides sigle linked list near functionality of the
 *     the singly_link_list.c module with limitations. The main 
 *     intention is for low overhead. The followings are the main
 *     differences.
 *
 *     1) It does not allocate storage containers for list node or
 *        list control structures by the facility. The caller
 *        provides the storage from any source as needed.  
 *
 *     2) No application call back for node search/find.
 *
 *     3) The use must include the node structure as the first
 *        field of the user's data structure to linked. This allows
 *        user to retrives the node and at the same time access to
 *        the data associated with the node.
 *
 *     4) Lite verion is higher risk because the caller also has
 *        the defition of the list and node structures. Use it
 *        at your own risk. 
 *
 *     5) There is no protection for mutual exclusive access by
 *        multiple threads or from interrupt context. The caller
 *        has to provide the protection if needed.
 */

#include "cpr_types.h"
#include "sll_lite.h"

/*
 *     LIST          NODE-1        NODE-2        NODE-3
 *   -----------    ----------    ----------    ----------
 *   | head    |--->| next_p |--->| next_p |--->| next_p |--->NULL
 *   | count   |    |        |    |        |  ->|        |
 *   |         |    ----------    ----------  | ----------
 *   | tail    |------------------------------|
 *   -----------
 */

/**
 * sll_lite_init initializes list control structure given by the 
 * caller.
 *
 * @param[in]list - pointer to the list control structure 
 *                  sll_lite_list_t 
 *
 * @return        - SLL_LITE_RET_SUCCESS for success
 *                - SLL_LITE_RET_INVALID_ARGS when arguments are 
 *                  invalid.
 */
sll_lite_return_e
sll_lite_init (sll_lite_list_t *list)
{
    if (list == NULL) { 
        return (SLL_LITE_RET_INVALID_ARGS);
    }
    list->count   = 0;
    list->head_p  = NULL; 
    list->tail_p  = NULL;
    return (SLL_LITE_RET_SUCCESS);
}

/**
 * sll_lite_link_head puts node to the head of the list.
 *
 * @param[in]list - pointer to the list control structure 
 *                  sll_lite_list_t. The list must be
 *                  initialized prior. 
 * @param[in]node - pointer to the list node structure.
 *
 * @return        - SLL_LITE_RET_SUCCESS for success
 *                - SLL_LITE_RET_INVALID_ARGS when arguments are 
 *                  invalid.
 */
sll_lite_return_e
sll_lite_link_head (sll_lite_list_t *list, sll_lite_node_t *node)
{
    if ((list == NULL) || (node == NULL)) {
        return (SLL_LITE_RET_INVALID_ARGS);
    }

    if (list->head_p == NULL) {
        /* list is empty, this node becomes head */
        list->head_p = node;
        list->tail_p = node;
        node->next_p = NULL; 
    } else {
        /* list is not empty, link to the head */
        node->next_p = list->head_p;
        list->head_p = node;
    }
    list->count++;

    return (SLL_LITE_RET_SUCCESS);
}

/**
 * sll_lite_link_tail puts node to the tail of the list.
 *
 * @param[in]list - pointer to the list control structure
 *                  sll_lite_list_t. The list must be
 *                  initialized prior.
 * @param[in]node - pointer to the list node structure.
 *
 * @return        - SLL_LITE_RET_SUCCESS for success
 *                - SLL_LITE_RET_INVALID_ARGS when arguments are
 *                  invalid.
 */
sll_lite_return_e
sll_lite_link_tail (sll_lite_list_t *list, sll_lite_node_t *node)
{
    if ((list == NULL) || (node == NULL)) {
        return (SLL_LITE_RET_INVALID_ARGS);
    }

    if (list->tail_p == NULL) {
        /* list is empty, this node becomes head */
        list->head_p = node;
        list->tail_p = node;
    } else {
        /* list is not empty, link to the tail */
        list->tail_p->next_p = node;
        list->tail_p = node;
    }
    node->next_p = NULL;
    list->count++;

    return (SLL_LITE_RET_SUCCESS);
}

/**
 * sll_lite_unlink_head removes head node from the head of the list and
 * returns it to the caller.
 *
 * @param[in]list - pointer to the list control structure
 *                  sll_lite_list_t. The list must be
 *                  initialized prior.
 *
 * @return        Pointer to the head node if one exists otherwise
 *                return NULL.
 */
sll_lite_node_t *
sll_lite_unlink_head (sll_lite_list_t *list)
{
    sll_lite_node_t *node_p = NULL;

    if (list == NULL) {
        return (NULL);
    }

    if (list->head_p !=  NULL) {
        node_p = list->head_p;
        list->head_p = list->head_p->next_p;
        if (list->tail_p == node_p) {
            /* this the only node on the list */
            list->tail_p = list->head_p;
        }
        list->count--;
        node_p->next_p = NULL;
    }
    return (node_p);
}

/**
 * sll_lite_unlink_tail removes tail node from the list and
 * returns it to the caller.
 *
 * @param[in]list - pointer to the list control structure
 *                  sll_lite_list_t. The list must be
 *                  initialized prior.
 *
 * @return        Pointer to the tail node if one exists otherwise
 *                return NULL.
 */
sll_lite_node_t *
sll_lite_unlink_tail (sll_lite_list_t *list)
{
    sll_lite_node_t *node_p;

    if ((list == NULL) || (list->tail_p == NULL)) {
        return (NULL);
    }

    /* take the node at the tail and then remove it from the list */
    node_p = list->tail_p;
    if (sll_lite_remove(list, node_p) != SLL_LITE_RET_SUCCESS) {
        /* some thing is wrong */
        return (NULL);
    }
    return (node_p);
}

/**
 * sll_lite_remove removes the given node from the list.
 *
 * @param[in]list - pointer to the list control structure
 *                  sll_lite_list_t. The list must be
 *                  initialized prior.
 * @param[in]node - pointer to the list node structure to be
 *                  removed.
 *
 * @return        - SLL_LITE_RET_SUCCESS for success
 *                - SLL_LITE_RET_INVALID_ARGS when arguments are 
 *                  invalid.
 *                - SLL_LITE_RET_NODE_NOT_FOUND when the node
 *                  to remove is not found.
 */
sll_lite_return_e
sll_lite_remove (sll_lite_list_t *list, sll_lite_node_t *node)
{
    sll_lite_node_t *this_p, *prev_p;

    if ((list == NULL) || (node == NULL)) {
        return (SLL_LITE_RET_INVALID_ARGS);
    }

    prev_p = NULL;
    this_p = list->head_p; /* starting from the head */

    /*
     * Find the node on the list and then remove it from
     * the list.
     */
    while (this_p != NULL) {
        if (this_p == node) {
            break;
        }
        prev_p = this_p;
        this_p = this_p->next_p;
    }

    if (this_p != NULL) {
       /* found the element */
       if (prev_p == NULL) {
           /* the node to remove is the head */
           list->head_p = list->head_p->next_p;
           if (list->tail_p == this_p) {
               /* this the only node on the list */
               list->tail_p = list->head_p;
           }
       } else {
           /* the element is in the middle of the chain */
           prev_p->next_p = this_p->next_p;
           if (list->tail_p == this_p) {
               /* the node to remove is the last node */
               list->tail_p = prev_p;
           }
       }
       list->count--;
       node->next_p = NULL;
       return (SLL_LITE_RET_SUCCESS);
    }
    return (SLL_LITE_RET_NODE_NOT_FOUND);
}
