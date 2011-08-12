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

#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

/**
  * @defgroup MutexIPCAPIs The Mutex/Semaphore IPC APIs
  * @ingroup IPC
  * @brief The module related to Mutex/Sempahore abstraction for the pSIPCC
  * @{
  */


/**
 * cprCreateMutex
 *
 * @brief Creates a mutual exclusion block
 *
 * The cprCreateMutex function is called to allow the OS to perform whatever
 * work is needed to create a mutex.
 *
 * @param[in] name  - name of the mutex. If present, CPR assigns this name to
 * the mutex to assist in debugging.
 *
 * @return Mutex handle or NULL if creation failed. If NULL, set errno
 */
cprMutex_t
cprCreateMutex (const char *name)
{
    static const char fname[] = "cprCreateMutex";
    static uint16_t id = 0;
    int32_t returnCode;
    cpr_mutex_t *cprMutexPtr;
    pthread_mutex_t *pthreadMutexPtr;

    /*
     * Malloc memory for a new mutex. CPR has its' own
     * set of mutexes so malloc one for the generic
     * CPR view and one for the CNU specific version.
     */
    cprMutexPtr = (cpr_mutex_t *) cpr_malloc(sizeof(cpr_mutex_t));
    pthreadMutexPtr = (pthread_mutex_t *) cpr_malloc(sizeof(pthread_mutex_t));
    if ((cprMutexPtr != NULL) && (pthreadMutexPtr != NULL)) {
        /* Assign name */
        cprMutexPtr->name = name;

        /*
         * Use default mutex attributes. TBD: if we do not
         * need cnuMutexAttributes global get rid of it
         */
        returnCode = pthread_mutex_init(pthreadMutexPtr, NULL);
        if (returnCode != 0) {
            CPR_ERROR("%s - Failure trying to init Mutex %s: %d\n",
                      fname, name, returnCode);
            cpr_free(pthreadMutexPtr);
            cpr_free(cprMutexPtr);
            return (cprMutex_t)NULL;
        }

        /*
         * TODO - It would be nice for CPR to keep a linked
         * list of active mutexes for debugging purposes
         * such as a show command or walking the list to ensure
         * that an application does not attempt to create
         * the same mutex twice.
         */
        cprMutexPtr->u.handlePtr = pthreadMutexPtr;
        cprMutexPtr->lockId = ++id;
        return (cprMutex_t)cprMutexPtr;
    }

    /*
     * Since the code malloced two pointers ensure both
     * are freed since one malloc call could have worked
     * and the other failed.
     */
    if (pthreadMutexPtr != NULL) {
        cpr_free(pthreadMutexPtr);
    } else if (cprMutexPtr != NULL) {
        cpr_free(cprMutexPtr);
    }

    /* Malloc failed */
    CPR_ERROR("%s - Malloc for mutex %s failed.\n", fname, name);
    errno = ENOMEM;
    return (cprMutex_t)NULL;
}


/**
 * cprDestroyMutex
 *
 * @brief Destroys the mutex passed in.
 *
 * The cprDestroyMutex function is called to destroy a mutex. It is the
 * application's responsibility to ensure that the mutex is unlocked when
 * destroyed. Unpredictiable behavior will occur if an application
 * destroys a locked mutex.
 *
 * @param[in] mutex - mutex to destroy
 *
 * @return CPR_SUCCESS or CPR_FAILURE. errno should be set for CPR_FAILURE.
 */
cprRC_t
cprDestroyMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprDestroyMutex";
    cpr_mutex_t *cprMutexPtr;
    int32_t rc;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        rc = pthread_mutex_destroy(cprMutexPtr->u.handlePtr);
        if (rc != 0) {
            CPR_ERROR("%s - Failure destroying Mutex %s: %d\n",
                      fname, cprMutexPtr->name, rc);
            return CPR_FAILURE;
        }
        cprMutexPtr->lockId = 0;
        cpr_free(cprMutexPtr->u.handlePtr);
        cpr_free(cprMutexPtr);
        return CPR_SUCCESS;
    }

    /* Bad application! */
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}


/**
 * cprGetMutex
 *
 * @brief Acquire ownership of a mutex
 *
 * This function locks the mutex referenced by the mutex parameter. If the mutex
 * is locked by another thread, the calling thread will block until the mutex is
 * released.
 *
 * @param[in] mutex - Which mutex to acquire
 *
 * @return CPR_SUCCESS or CPR_FAILURE
 */
cprRC_t
cprGetMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprGetMutex";
    cpr_mutex_t *cprMutexPtr;
    int32_t rc;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        rc = pthread_mutex_lock((pthread_mutex_t *) cprMutexPtr->u.handlePtr);
        if (rc != 0) {
            CPR_ERROR("%s - Error acquiring mutex %s: %d\n",
                      fname, cprMutexPtr->name, rc);
            return CPR_FAILURE;
        }
        return CPR_SUCCESS;
    }

    /* Bad application! */
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}


/**
 * cprReleaseMutex
 *
 * @brief Release ownership of a mutex
 *
 * This function unlocks the mutex referenced by the mutex parameter.
 * @param[in] mutex - Which mutex to release
 *
 * @return CPR_SUCCESS or CPR_FAILURE
 */
cprRC_t
cprReleaseMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprReleaseMutex";
    cpr_mutex_t *cprMutexPtr;
    int32_t rc;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        rc = pthread_mutex_unlock((pthread_mutex_t *) cprMutexPtr->u.handlePtr);
        if (rc != 0) {
            CPR_ERROR("%s - Error releasing mutex %s: %d\n",
                      fname, cprMutexPtr->name, rc);
            return CPR_FAILURE;
        }
        return CPR_SUCCESS;
    }

    /* Bad application! */
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}


/**
 * cprCreateSignal
 *
 * Creates a conditional signal block
 *
 * Parameters: name  - name of the signal
 *
 * Return Value: signal handle or NULL if creation failed.
 */
cprSignal_t
cprCreateSignal (const char *name)
{
    static const char fname[] = "cprCreateSignal";
    static uint16_t id = 0;
    int32_t returnCode;
    cpr_signal_t *cprSignalPtr;
    pthread_cond_t *pthreadSignalPtr;

    /*
     * Malloc memory for a new signal. CPR has its' own
     * set of signales so malloc one for the generic
     * CPR view and one for the CNU specific version.
     */
    cprSignalPtr = (cpr_signal_t *) cpr_malloc(sizeof(cpr_signal_t));
    pthreadSignalPtr = (pthread_cond_t *) cpr_malloc(sizeof(pthread_cond_t));
    if ((cprSignalPtr != NULL) && (pthreadSignalPtr != NULL)) {
        /* Assign name */
        cprSignalPtr->name = name;

        /*
         * Use default signal attributes. TBD: if we do not
         * need cnuMutexAttributes global get rid of it
         */
        returnCode = pthread_cond_init(pthreadSignalPtr, NULL);
        if (returnCode != 0) {
            CPR_ERROR("%s - Failure trying to init Conitional signal %s: %d\n",
                      fname, name, returnCode);
            cpr_free(pthreadSignalPtr);
            cpr_free(cprSignalPtr);
            return (cprSignal_t)NULL;
        }

        cprSignalPtr->u.handlePtr = pthreadSignalPtr;
        cprSignalPtr->lockId = ++id;
        return (cprSignal_t)cprSignalPtr;
    }

    /*
     * Since the code malloced two pointers ensure both
     * are freed since one malloc call could have worked
     * and the other failed.
     */
    if (pthreadSignalPtr != NULL) {
        cpr_free(pthreadSignalPtr);
    } else if (cprSignalPtr != NULL) {
        cpr_free(cprSignalPtr);
    }

    /* Malloc failed */
    CPR_ERROR("%s - Malloc for Signal %s failed.\n", fname, name);
    errno = ENOMEM;
    return (cprSignal_t)NULL;
}


/*
 * cprDestroySignal
 *
 * Destroys the signal passed in.
 *
 * Parameters: signal - signal to destroy
 *
 * Return Value: CPR_SUCCESS or CPR_FAILURE
 */
cprRC_t
cprDestroySignal (cprSignal_t signal)
{
    static const char fname[] = "cprDestroySignal";
    cpr_signal_t *cprSignalPtr;
    int32_t rc;

    cprSignalPtr = (cpr_signal_t *) signal;
    if (cprSignalPtr != NULL) {
        rc = pthread_cond_destroy(cprSignalPtr->u.handlePtr);
        if (rc != 0) {
            CPR_ERROR("%s - Failure destroying Signal %s: %d\n",
                      fname, cprSignalPtr->name, rc);
            return CPR_FAILURE;
        }
        cprSignalPtr->lockId = 0;
        cpr_free(cprSignalPtr->u.handlePtr);
        cpr_free(cprSignalPtr);
        return CPR_SUCCESS;
    }

    /* Bad application! */
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}

/**
 * cprCondTimedWait
 *
 * Create a conditional wait
 *
 *
 * Return Value: CPR_SUCCESS or CPR_FAILURE
 */
cprRC_t
cprCondTimedWait (cprMutex_t mutex, cprSignal_t signal, 
		unsigned int sec, unsigned int msec)
{
    static const char fname[] = "cprCondTimedWait";
    cpr_mutex_t *cprMutexPtr;
    cpr_signal_t  *cprSignalPtr;
    int32_t rc;
    struct timespec ts;
    struct timeval tv;

    // Set up ts values
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + sec;
    unsigned long temp_nsec = (tv.tv_usec + (msec * 1000));
    if ( temp_nsec > 1000000)
    {
    	ts.tv_nsec = temp_nsec % 1000000;
    	ts.tv_sec++;
    } else {
    	ts.tv_nsec = temp_nsec * 1000;
    }

    cprMutexPtr = (cpr_mutex_t *) mutex;
    cprSignalPtr = (cpr_signal_t *)signal;
    if (cprMutexPtr != NULL && cprSignalPtr != NULL) {
        rc = pthread_cond_timedwait((pthread_cond_t *)cprSignalPtr->u.handlePtr,
                           (pthread_mutex_t *) cprMutexPtr->u.handlePtr,
                           &ts);
        if (rc != 0) {
            CPR_ERROR("%s - Error releasing mutex %s, signal %s: %d\n",
                      fname, cprMutexPtr->name, cprSignalPtr->name, rc);
            return CPR_FAILURE;
        }
        return CPR_SUCCESS;
    }

    /* Bad application! */
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}

/**
 * cprCondSignal
 *
 * Send a conditional 
 *
 *
 * Return Value: CPR_SUCCESS or CPR_FAILURE
 */
cprRC_t
cprCondSignal (cprSignal_t signal)
{
    static const char fname[] = "cprCondTimedWait";
    cpr_signal_t  *cprSignalPtr;
    int32_t rc;

    cprSignalPtr = (cpr_signal_t *)signal;
    if (cprSignalPtr != NULL) {
        rc = pthread_cond_signal((pthread_cond_t *)cprSignalPtr->u.handlePtr); 
        
        if (rc != 0) {
            CPR_ERROR("%s - Error sending signal %s: %d\n",
                      fname, cprSignalPtr->name, rc);
            return CPR_FAILURE;
        }
        return CPR_SUCCESS;
    }

    /* Bad application! */
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}


