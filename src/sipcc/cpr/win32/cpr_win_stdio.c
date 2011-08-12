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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "cpr_types.h"
#include "cpr_win_stdio.h"

typedef enum
{
    CPR_LOGLEVEL_ERROR=0,
    CPR_LOGLEVEL_WARNING,
    CPR_LOGLEVEL_INFO,
    CPR_LOGLEVEL_DEBUG
} cpr_log_level_e;

#ifdef _CPR_USE_EXTERNAL_LOGGER_

static plog_msg_function_cpr _sRegisteredLogger=NULL;

void cprRegisterLogger (plog_msg_function_cpr pLoggerFunction)
{
    if (pLoggerFunction != NULL)
    {
        _sRegisteredLogger = pLoggerFunction;
    }
}
#endif

static void notifyExternalLogger (int logLevel, const char * pFormat, va_list args)
{
#ifdef _CPR_USE_EXTERNAL_LOGGER_
    if (_sRegisteredLogger != NULL)
    {
        (*_sRegisteredLogger)(logLevel, pFormat, args);
    }
#endif
}

#ifndef WIN32_7960
/**
 * @def LOG_MAX
 *
 * Constant represents the maximum allowed length for a message
 */
#define LOG_MAX 4096
//static FILE *logfile = fopen("sipcc.log", "w");

/**************************************************
 *
 *
 * These Functions handle the TNP Soft Phones
 *
 *
 **************************************************/

int32_t
buginf_msg (const char *str)
{
    OutputDebugStringA(str);

    return (0);
}

void
err_msg (const char *_format, ...)
{
#ifdef _CPR_USE_EXTERNAL_LOGGER_
    va_list ap;

    va_start(ap, _format);
    notifyExternalLogger((int) CPR_LOGLEVEL_ERROR, _format, ap);
    va_end(ap);
#else
    char fmt_buf[LOG_MAX+1];  // temporary workspace for all of the printfs
    va_list ap;

    va_start(ap, _format);
    vsprintf_s(fmt_buf, _countof(fmt_buf), _format, ap);
    va_end(ap);

    buginf_msg(fmt_buf);
#endif // ifdef _CPR_USE_EXTERNAL_LOGGER
}

void
notice_msg (const char *_format, ...)
{
#ifdef _CPR_USE_EXTERNAL_LOGGER_
    va_list ap;

    va_start(ap, _format);
    notifyExternalLogger((int) CPR_LOGLEVEL_INFO, _format, ap);
    va_end(ap);
#else
    char fmt_buf[LOG_MAX+1];  // temporary workspace for all of the printfs
    va_list ap;

    va_start(ap, _format);
    vsprintf_s(fmt_buf, _countof(fmt_buf), _format, ap);
    va_end(ap);

    buginf_msg(fmt_buf);
#endif // ifdef _CPR_USE_EXTERNAL_LOGGER
}

int32_t
buginf (const char *_format, ...)
{
#ifdef _CPR_USE_EXTERNAL_LOGGER_
    va_list ap;

    va_start(ap, _format);
    notifyExternalLogger((int) CPR_LOGLEVEL_DEBUG, _format, ap);
    va_end(ap);
#else
    char fmt_buf[LOG_MAX+1];  // temporary workspace for all of the printfs
    va_list ap;

    va_start(ap, _format);
    vsprintf_s(fmt_buf, _countof(fmt_buf), _format, ap);
    va_end(ap);

    buginf_msg(fmt_buf);
#endif
    return (0);
}


#else

/**************************************************
 *
 *
 * These Functions handle the 7960/40 Soft Phones
 *
 *
 **************************************************/
#define MAX_TIMER_RESOLUTION 0

/* Semaphore to serialize writing to the debug log file */
HANDLE LogSema;

#define DEBUGLOG "DebugLog.Txt"

extern int timestamp_debug;
void FormatDebugTime(char *timeStr, int length);

void
win32_stdio_init (void)
{
    /* Create the semaphore used to serialize writing debug log file */
    LogSema = CreateSemaphore(NULL, 1, 1, NULL);
}

void
Format7960DebugTime (char *timeStr, int length)
{
#if MAX_TIMER_RESOLUTION
    char tmpBuf[32];
    SYSTEMTIME myTime;
    DWORD dwTime = timeGetTime();

    GetSystemTime(&myTime);

    sprintf(tmpBuf, "[%2.2d:%2.2d:%2.2d.%2.3d %6u] ",
            myTime.wHour, myTime.wMinute, myTime.wSecond, myTime.wMilliseconds,
            dwTime);
    SafeStrCpy(timeStr, tmpBuf, length);
#else
    FormatDebugTime(timeStr, length);
#endif

}


int
put_timestamp (const char *_format, char *fmt_buf)
{
    char timeStr[32];
    int size = 0;

    /*
     * Put the Timestamp at the front of the message
     */
    if (timestamp_debug) {
        /*
         * Do not print a timestamp if we are just printing a blank line
         */
        if (strcmp(_format, "\n")) {
            Format7960DebugTime(timeStr, sizeof(timeStr));
            /*
             * Formatting (%s, %c) is not allowed in the timeStr (Unless you want
             * to adjust the variable arguments to make it work)
             */
            vsprintf(fmt_buf, (const char *) timeStr, 0);
            size = strlen(timeStr);
        }
    }
    return size;
}

int32_t
buginf_msg (const char *str)
{
    static FILE *f = NULL;

    if (f == NULL) {
        f = fopen(DEBUGLOG, "w");
    }

    if (f != NULL) {
        fwrite(str, 1, min(4096, strlen(str)), f);
        fflush(f);
    }

    return (0);
}

int32_t
err_msg (const char *_format, ...)
{
    static char fmt_buf[1024];  // temporary workspace for all of the printfs
    va_list ap;
    int size;

    WaitForSingleObject(LogSema, INFINITE);

    size = put_timestamp(_format, fmt_buf);
    va_start(ap, _format);
    vsprintf(fmt_buf + size, _format, ap);
    va_end(ap);

    buginf_msg((const char *) fmt_buf);

    ReleaseSemaphore(LogSema, 1, NULL);
    return (0);
}

int32_t
buginf (const char *_format, ...)
{
    static char fmt_buf[1024];  // temporary workspace for all of the printfs
    va_list ap;
    int size;

    WaitForSingleObject(LogSema, INFINITE);

    size = put_timestamp(_format, fmt_buf);
    va_start(ap, _format);
    vsprintf(fmt_buf + size, _format, ap);
    va_end(ap);

    buginf_msg((const char *) fmt_buf);

    ReleaseSemaphore(LogSema, 1, NULL);
    return (0);
}

#endif
