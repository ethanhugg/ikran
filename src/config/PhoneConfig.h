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

#ifndef PHONE_CONFIG_H
#define PHONE_CONFIG_H

#include <string>
#include <vector>

namespace CSF
{
    class PhoneConfig
    {
    public:
        PhoneConfig();
        ~PhoneConfig();

        /**
         * parse
         *
         * @brief Creates phone configuration from XML string.
         *
         * @param[in] configFileContents - XML string typically downloaded from TFTP server.
         *
         * @return true for successful parsing and object creation or false for failure.
         */
        bool parse(const std::string& configFileContents);

        /**
         * isValidConfig
         *
         * @brief Indicates if the configuration contained in the object is valid and no errors were found parsing.
         * This is the same as returned by parse().
         *
         * @return true for if the configuration is valid, false otherwise.
         */
        bool isValidConfig();

        /**
         * isFullConfig
         *
         * @brief Indicates whether full config is present or if CAPF config is present.
         * If this returns false then only CAPF related methods will return valid data.
         *
         * @return true if full config is present or false if CAPF config is present.
         */
        bool isFullConfig();

        /**
         * getMediaStartPort
         *
         * @brief Get the start UDP port range which can used for RTP.
         *
         * @return Start of the range of ports.
         */
        int getMediaStartPort();

        /**
         * getMediaEndPort
         *
         * @brief Get the end UDP port range which can used for RTP.
         *
         * @return End of the range of ports.
         */
        int getMediaEndPort();

        /**
         * getCallManagerAddresses
         *
         * @brief Retrieves Call Manager IP addresses
         *
         * @return Call Manager IP addresses
         */
        std::vector< std::string > getCallManagerAddresses();

        /**
         * getLineNumbers
         *
         * @brief Retrieves line DNs
         *
         * @return line DNs
         */
        std::vector< std::string > getLineNumbers();

        /**
         * getCAPFMode
         *
         * @brief Retrieves the CAPF mode
         *
         * @return CAPF mode
         */
        int getCAPFMode();

        /**
         * hasEncryptedConfig
         *
         * @brief Indicates that the config is encrypted.
         *
         * @return true if config is encrypted and false otherwise.
         */
        bool hasEncryptedConfig();

        /**
         * getCertificateHash
         *
         * @brief Retrieves the certificate hash.
         *
         * @return string containing the certificate hash or an empty string if no CAPF enrollment has been done.
         */
        std::string getCertificateHash();

        /**
         * getDSCPAudio
         *
         * @brief Retrieves the DSCP value for audio media.
         *
         * @return int representing the DSCP value for audio media.
         */
        int getDSCPAudio();

        /**
         * getDSCPAudio
         *
         * @brief Retrieves the DSCP value for video media.
         *
         * @return int representing the DSCP value for video media.
         */
        int getDSCPVideo();

        /**
         * isVADEnabled
         *
         * @brief Indicates whether Voice Activity Detection (VAD) is
         * enabled.
         *
         * @return bool indicating whether Voice Activity Detection (VAD)
         * is enabled or not.
         */
        bool isVADEnabled();

    private:
        void init();
         
        bool validConfig;
        bool fullConfig;
        int mediaStartPort;
        int mediaEndPort;
        std::vector< std::string > callManagerAddresses;
        std::vector< std::string > lineNumbers;
        int CAPFMode;
        bool encryptedConfig;
        std::string certificateHash;
        int DSCPAudio;
        int DSCPVideo;
        bool VADEnabled;
    };
}

#endif // PHONE_CONFIG_H
