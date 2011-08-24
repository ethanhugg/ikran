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

#ifndef CSFGIPSMEDIAPROVIDER_H_
#define CSFGIPSMEDIAPROVIDER_H_

#ifndef _USE_CPVE

#include <CSFMediaProvider.h>
#include "voe_base.h"
#include <string>

class webrtc::VoiceEngine;

namespace CSF
{
	enum	// Event ids
	{
		eVideoModeChanged,
		eKeyFrameRequest,
		eMediaLost,
		eMediaRestored
	};

// Master key is 128 bits, master salt 112 bits.
// See GIPS_VoiceEngine_API_Guide_3.4.8.pdf page 183
#define GIPS_MASTER_KEY_LENGTH      16
#define GIPS_MASTER_SALT_LENGTH     14
#define GIPS_KEY_LENGTH             (GIPS_MASTER_KEY_LENGTH + GIPS_MASTER_SALT_LENGTH)
#define GIPS_CIPHER_LENGTH          GIPS_KEY_LENGTH

	class GipsAudioProvider;
	class GipsVideoProvider;

	class GipsMediaProvider : public MediaProvider
	{
		friend class MediaProvider;
        friend class GipsVideoProvider;
        friend class GipsAudioProvider;

	protected:
		GipsMediaProvider( );
		~GipsMediaProvider();

		int init();
		virtual void shutdown();

		AudioControl* getAudioControl();
		VideoControl* getVideoControl();
		AudioTermination* getAudioTermination();
		VideoTermination* getVideoTermination();
		void addMediaProviderObserver( MediaProviderObserver* observer );

        bool getKey(
            const unsigned char* masterKey, 
            int masterKeyLen, 
            const unsigned char* masterSalt, 
            int masterSaltLen,
            unsigned char* key,
            unsigned int keyLen
            );

	private:
		GipsAudioProvider* pAudio;
		GipsVideoProvider* pVideo;

        webrtc::VoiceEngine * getGipsVoiceEngine ();
	};

} // namespace

#endif
#endif /* CSFGIPSMEDIAPROVIDER_H_ */
