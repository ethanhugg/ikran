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

#include "cpr_types.h"
#include "cpr_locks.h"
#include "cpr_win_locks.h" /* Should really have this included from cpr_locks.h */
#include "cpr_debug.h"
#include "cpr_memory.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include <windows.h>

/* Defined in cpr_init.c */
extern CRITICAL_SECTION criticalSection;

/**
 * cprDisableSwap
 *
 * Windows forces you to create a Critical region semaphore and have threads
 * attempt to get ownership of that semaphore. The calling thread
 * will block until ownership of the semaphore is granted.
 *
 * Parameters: None
 *
 * Return Value: Nothing
 */
void
cprDisableSwap (void)
{
    EnterCriticalSection(&criticalSection);
}


/**
 * cprEnableSwap
 *
 * Releases ownership of the critical section semaphore.
 *
 * Parameters: None
 *
 * Return Value: Nothing
 */
void
cprEnableSwap (void)
{
    LeaveCriticalSection(&criticalSection);
}


/**
 * cprCreateMutex
 *
 * Creates a mutual exclusion block
 *
 * Parameters: name  - name of the mutex
 *
 * Return Value: Mutex handle or NULL if creation failed.
 */
cprMutex_t
cprCreateMutex (const char *name)
{
    cpr_mutex_t *cprMutexPtr;
    static char fname[] = "cprCreateMutex";
	WCHAR* wname;

    /*
     * Malloc memory for a new mutex. CPR has its' own
     * set of mutexes so malloc one for the generic
     * CPR view and one for the CNU specific version.
     */
    cprMutexPtr = (cpr_mutex_t *) cpr_malloc(sizeof(cpr_mutex_t));
    if (cprMutexPtr != NULL) {
        /* Assign name if one was passed in */
        if (name != NULL) {
            cprMutexPtr->name = name;
        }

		wname = cpr_malloc((strlen(name) + 1) * sizeof(WCHAR));
		mbstowcs(wname, name, strlen(name));
        cprMutexPtr->u.handlePtr = CreateMutex(NULL, FALSE, wname);
		cpr_free(wname);

        if (cprMutexPtr->u.handlePtr == NULL) {
            CPR_ERROR("%s - Mutex init failure: %d\n", fname, GetLastError());
            cpr_free(cprMutexPtr);
            cprMutexPtr = NULL;
        }
    }
    return cprMutexPtr;

}


/*
 * cprDestroyMutex
 *
 * Destroys the mutex passed in.
 *
 * Parameters: mutex  - mutex to destroy
 *
 * Return Value: Success or failure indication
 */
cprRC_t
cprDestroyMutex (cprMutex_t mutex)
{
    cpr_mutex_t *cprMutexPtr;
    const static char fname[] = "cprDestroyMutex";

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        CloseHandle(cprMutexPtr->u.handlePtr);
        cpr_free(cprMutexPtr);
        return (CPR_SUCCESS);
        /* Bad application! */
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
}


/**
 * cprGetMutex
 *
 * Acquire ownership of a mutex
 *
 * Parameters: mutex - Which mutex to acquire
 *
 * Return Value: Success or failure indication
 */
cprRC_t
cprGetMutex (cprMutex_t mutex)
{
    int32_t rc;
    static const char fname[] = "cprGetMutex";
    cpr_mutex_t *cprMutexPtr;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        /*
         * Block until mutex is available so this function will
         * always return success since if it returns we have
         * gotten the mutex.
         */
        rc = WaitForSingleObject((HANDLE) cprMutexPtr->u.handlePtr, INFINITE);
        if (rc != WAIT_OBJECT_0) {
            CPR_ERROR("%s - Error acquiring mutex: %d\n", fname, rc);
            return (CPR_FAILURE);
        }

        return (CPR_SUCCESS);

        /* Bad application! */
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
}


/**
 * cprReleaseMutex
 *
 * Release ownership of a mutex
 *
 * Parameters: mutex - Which mutex to release
 *
 * Return Value: Success or failure indication
 */
cprRC_t
cprReleaseMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprReleaseMutex";
    cpr_mutex_t *cprMutexPtr;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        if (ReleaseMutex(cprMutexPtr->u.handlePtr) == 0) {
            CPR_ERROR("%s - Error releasing mutex: %d\n",
                      fname, GetLastError());
            return (CPR_FAILURE);
        }
        return (CPR_SUCCESS);

        /* Bad application! */
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
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

	cpr_signal_t* cprSignalPtr = NULL;
	cpr_condition_t* cprConditionPtr = NULL;

	/* Initialise memory for signal */
	cprSignalPtr = (cpr_signal_t *) cpr_malloc(sizeof(cpr_signal_t));
	if (NULL == cprSignalPtr) {
		CPR_ERROR("%s - Malloc for Signal %s failed.\n", fname, name);
		errno = ENOMEM;
		return NULL;
	}
	
	/* ...and now the condition struct */
	cprConditionPtr = (cpr_condition_t *) cpr_malloc(sizeof(cpr_condition_t));
	if ((NULL == cprSignalPtr) || (NULL == cprConditionPtr)) {
		CPR_ERROR("%s - Malloc for Condition in Signal %s failed.\n", fname, name);
		errno = ENOMEM;
		cpr_free(cprSignalPtr);
		return NULL;
	}

	/* Populate the condition*/
	cprConditionPtr->noWaiters = 0;
	cprConditionPtr->sema = CreateSemaphore(NULL, /* No security */
											 0, /* Initially 0 */
											 0x7fffffff, /* Max Count */
											 NULL); /* Unnamed */
	InitializeCriticalSection(&cprConditionPtr->noWaitersLock);

	/* Populate the signal */
	cprSignalPtr->name = name;
	cprSignalPtr->u.handlePtr = cprConditionPtr;
	cprSignalPtr->lockId = ++id;

	return cprSignalPtr;
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
	cpr_signal_t* cprSignalPtr = NULL;
	cpr_condition_t* cprConditionPtr = NULL;

	if (NULL == signal) {
		CPR_ERROR("%s - NULL pointer passed in.\n", fname);
		errno = EINVAL;
		return CPR_FAILURE;
	}

	cprSignalPtr = (cpr_signal_t*) signal;
	
	cprConditionPtr = (cpr_condition_t*) cprSignalPtr->u.handlePtr;

	/*
	 * Free the condition type
	 * This is overkill right now with the null settings, but
	 * start off as you mean to go on and all that!
	 */
	cpr_free(cprConditionPtr);
	cprConditionPtr = NULL;
	cprSignalPtr->u.handlePtr = NULL;

	/* Free the signal type */
	cprSignalPtr->lockId = 0;
	cpr_free(cprSignalPtr);
	cprSignalPtr = NULL;

	return CPR_SUCCESS;
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

	cpr_mutex_t* cprMutexPtr = NULL;
	cpr_signal_t* cprSignalPtr = NULL;
	cpr_condition_t* cprConditionPtr = NULL;
	cprRC_t	returnVal = CPR_FAILURE;
	unsigned long timeout = (sec * 1000) + msec;
	int noWaiters = -1;
	
	cprMutexPtr = (cpr_mutex_t*) mutex;
	cprSignalPtr = (cpr_signal_t*) signal;

	if ((NULL != cprMutexPtr) && (NULL != cprSignalPtr)) {
		cprConditionPtr = (cpr_condition_t*) cprSignalPtr->u.handlePtr;

		/* Avoid race conditions while incrementing the number of waiters */
		EnterCriticalSection (&cprConditionPtr->noWaitersLock);
		cprConditionPtr->noWaiters++;
		LeaveCriticalSection (&cprConditionPtr->noWaitersLock);

		/* 
		 * This call atomically releases the mutex and waits on the 
		 * semaphore until cprCondSignal is called by another thread
		 */

		SignalObjectAndWait((HANDLE) cprMutexPtr->u.handlePtr,
			cprConditionPtr->sema,
			(DWORD)(timeout),
			TRUE);

		/* Reacquire lock */
		EnterCriticalSection(&cprConditionPtr->noWaitersLock);
		cprConditionPtr->noWaiters--;
		LeaveCriticalSection(&cprConditionPtr->noWaitersLock);

//		if (0 == noWaiters) {
//			/*
//			 * This call atomically signals the waiters_done event
//			 * and waits until it can acquire the external mutex.
//			 * This is required to ensure fairness
//			 */
//			SignalObjectAndWait(cprConditionPtr->waitersDone,
//				(HANDLE) cprMutexPtr->u.handlePtr,
//				INFINITE,
//				FALSE);
//		} else {
			/*
			 * Always regain the external mutex since that's the guarantee we give to callers
			 */
			WaitForSingleObject((HANDLE) cprMutexPtr->u.handlePtr, INFINITE);
//		}
		returnVal = CPR_SUCCESS;
	}

	return returnVal;
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
    static const char fname[] = "cprCondSignal";
    cpr_signal_t*	 cprSignalPtr = NULL;
	cpr_condition_t* cprConditionPtr = NULL;
	int haveWaiters = 0;

	cprSignalPtr = (cpr_signal_t *)signal;
	if (NULL == cprSignalPtr) {
		CPR_ERROR("%s - NULL pointer passed in.\n", fname);
		errno = EINVAL;
		return CPR_FAILURE;
	}

    cprConditionPtr = cprSignalPtr->u.handlePtr;
	if (NULL == cprConditionPtr) {
		CPR_ERROR("%s - Error sending signal %s: %d\n",
			fname, cprSignalPtr->name, -1);
		errno = EINVAL;
		return CPR_FAILURE;
	}

	EnterCriticalSection(&cprConditionPtr->noWaitersLock);
	haveWaiters = (cprConditionPtr->noWaiters > 0);
	LeaveCriticalSection(&cprConditionPtr->noWaitersLock);

	if (haveWaiters)
	{
		ReleaseSemaphore(cprConditionPtr->sema,1, 0);
	}
    return CPR_SUCCESS;
}

