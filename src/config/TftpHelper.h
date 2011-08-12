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

#pragma once

#include <string>

namespace CSFUnified
{

	class TftpHelper
	{
		public:
			TftpHelper();
			~TftpHelper();

			/**
			 * retrieveFile
			 *
			 * @brief Retrieves a file from a TFTP server. This method does not save the file locally.
			 * It instead copies the contents to a specified buffer.
			 *
			 * @param[in] url - URL of the file to download in the form "tftp://host/directory/filename"
			 *
			 * @param[out] contents - string buffer to which the TFTP file contents will be copied to".
			 *
			 * @return true for successful download or false for failure.
			 */
			bool retrieveFile(const std::string& url, std::string& contents);

			/**
			 * retrieveFile
			 *
			 * @brief Retrieves a file from a TFTP server. This method saves the file locally.
			 *
			 * @param[in] url - URL of the file to download in the form "tftp://host/directory/filename"
			 *
			 * @param[in] filePath - path and filename to which the downloaded file will be saved.
			 *
			 * @return true for successful download or false for failure.
			 */
			bool retrieveFile(const std::string& url, const std::string& filePath);

			/**
			 * writeLocalFile
			 *
			 * @brief Saves a file locally.
			 *
			 * @param[in] filePath - name of the file to write to"
			 *
			 * @param[in] contents - the data to write to the file.
			 *
			 * @return true for successful write or false for failure.
			 */
			bool writeLocalFile(const std::string& filePath, const std::string& contents);

			/**
			 * readLocalFile
			 *
			 * @brief Reads a file locally.
			 *
			 * @param[in] filePath - name of the file to read from
			 *
			 * @param[out] contents - the data read from the file.
			 *
			 * @return true for successful read or false for failure.
			 */
			bool readLocalFile(const std::string& filePath, std::string& contents);
		};
}
