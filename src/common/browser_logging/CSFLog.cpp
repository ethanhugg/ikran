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

#include "CSFLog.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "base/logging.h"
#include "base/file_path.h"
#include "base/path_service.h"
#include "base/at_exit.h"
#include "base/command_line.h"


bool InitChromeLogging(int argc, char** argv)
{
    // Manages the destruction of singletons.
    base::AtExitManager exit_manager;

#ifdef NDEBUG
    logging::LoggingDestination destination = logging::LOG_ONLY_TO_FILE;
#else
    logging::LoggingDestination destination = logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG;
#endif

    CommandLine::Init(argc, argv);

	FilePath log_filename;
	PathService::Get(base::DIR_EXE, &log_filename);
	log_filename = log_filename.AppendASCII("sip.log");
	logging::InitLogging(
				log_filename.value().c_str(),
				destination,
				logging::LOCK_LOG_FILE,
				logging::DELETE_OLD_LOG_FILE,
				logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);

	return true;
}


void CSFLogV(CSFLogLevel priority, const char* sourceFile, int sourceLine, const char* tag , const char* format, va_list args)
{

	char buffer[1024];

#ifdef STDOUT_LOGGING
		printf("%s\n:",tag);
		vprintf(format, args);
#else
		vsnprintf(buffer, 1024, format, args);
		if (priority == CSF_LOG_CRITICAL)
			LOG(ERROR) << tag << " " << buffer;
		if (priority == CSF_LOG_ERROR)
			LOG(ERROR) << tag << " " << buffer;
		if (priority == CSF_LOG_WARNING)
			LOG(WARNING) << tag << " " << buffer;
		if (priority == CSF_LOG_INFO)
			LOG(INFO) << tag << " " << buffer;
		if (priority == CSF_LOG_NOTICE)
			LOG(ERROR) << tag << " " << buffer;
		if (priority == CSF_LOG_DEBUG)
			LOG(INFO) << tag << " " << buffer;
#endif

}

void CSFLog( CSFLogLevel priority, const char* sourceFile, int sourceLine, const char* tag , const char* format, ...)
{
	va_list ap;
    va_start(ap, format);
	
    CSFLogV(priority, sourceFile, sourceLine, tag, format, ap);
    va_end(ap);
}

