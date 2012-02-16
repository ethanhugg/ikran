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
 *  Kai Chen <kaichen2@cisco.com>
 *  Yi Wang <yiw2@cisco.com>
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
 
#include "Common.h"


cc_sdp_direction_t _sVideoPref = CC_SDP_DIRECTION_INACTIVE;


#ifndef WIN32
int csf_getch(void)
{
    char buf = 0;
    struct termios old;
   
    memset(&old, 0, sizeof(termios));

    if (tcgetattr(0, &old) < 0)
    {
        CSFLogDebugS("Common", "tcgetattr() failed.\n");
        return 0;
    }

    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSANOW, &old) < 0)
    {
        CSFLogDebugS("Common", "tcsetattr ICANON.\n");
        return 0;
    }

    if (read(0, &buf, 1) < 0)
    {
        CSFLogDebugS("Common", "read()");
    }

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;

    if (tcsetattr(0, TCSADRAIN, &old) < 0)
    {
        perror ("tcsetattr ~ICANON");
    }
    return (buf);
}

int csf_kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

#else
int csf_getch(void)
{
    return _getch();
}

int csf_kbhit(void)
{
    return _kbhit();
}
#endif

UserOperationRequestData::~UserOperationRequestData()
{
    if (m_pData == NULL)
    {
        return;
    }

    switch (request)
    {
    case eOriginatePhoneCall:
        delete m_pPhoneNumberToCall;
        break;
    case eOriginateP2PPhoneCall:
        delete m_pPhoneNumberToCall;
        break;
    case eSendDTMFDigitOnFirstCallWithDTMFCaps:
        delete m_pDTMFDigit;
        break;
    default:
    	break;
    }
}


void getPasswordFromConsole (const char * pPromptText, string & password)
{
    password.clear();

    if (pPromptText == NULL)
    {
        return;
    }

    cout << pPromptText << flush;

    int ch = csf_getch();

#ifdef WIN32
const int ENTER_KEY = 13;
const int BS_KEY = 8;
#else
const int ENTER_KEY = '\n';
const int BS_KEY = 127;
#endif

    while(ch != ENTER_KEY)
    {
        if (ch == BS_KEY) // backspace
        {
            if (password.length() > 0)
            {
                cout <<"\b \b" << flush;  // print a space over the '*' that was there
                password.erase(password.length() - 1);
            }
        }
        else
        {
            password += (char)ch;
            cout << '*' << flush;
        }
        ch = csf_getch();
    }
}

cc_sdp_direction_t getActiveVideoPref ()
{
    return _sVideoPref;
}

void cycleToNextVideoPref()
{
    int nextVideoPrefAsInt = (((int) _sVideoPref) + 1);
    _sVideoPref = (cc_sdp_direction_t) nextVideoPrefAsInt;

    if (_sVideoPref == CC_SDP_MAX_QOS_DIRECTIONS)
    {
        assert(((int) CC_SDP_DIRECTION_INACTIVE) == 0);
        _sVideoPref = CC_SDP_DIRECTION_INACTIVE;
    }
}

const char * getUserFriendlyNameForVideoPref (cc_sdp_direction_t videoPref)
{
    const char * pMediaTypeStr = (videoPref == CC_SDP_DIRECTION_INACTIVE) ? "audio" :
                                 (videoPref == CC_SDP_DIRECTION_SENDONLY) ? "video out" :
                                 (videoPref == CC_SDP_DIRECTION_RECVONLY) ? "video in" :
                                 (videoPref == CC_SDP_DIRECTION_SENDRECV) ? "video in/out" : "unknown";

    return pMediaTypeStr;
}



