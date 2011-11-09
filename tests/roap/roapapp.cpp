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

#include <sys/timeb.h>
#include <stdarg.h>

#include "csf_common.h"
#include "CSFLogStream.h"
#include "debug-psipcc-types.h"
#include "base/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/lock.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

#include "incomingroap.h"
#include "outgoingroap.h"
#include "incomingroapthread.h"
#include "outgoingroapthread.h"

static const char* logTag = "RoapProxy";



int main(int argc, char**argv)
{
  IncomingRoapThread incomingThread;
  OutgoingRoapThread outgoingThread;
  
  CSFLogDebugS(logTag, "ROAP Proxy Start");

  incomingThread.Start();
  outgoingThread.Start();
  
  printf("RoapProxy Running\nPress 'q' to quit\n\n");
  
  while (true)
  {
    char ch = getchar();
    
    if (ch == 'q' || ch == 'Q')
    {
      incomingThread.shutdown();
      outgoingThread.shutdown();
      break;
    }
  }
  
  incomingThread.Join();
  outgoingThread.Join();
  
  CSFLogDebugS(logTag, "ROAP Proxy End");
}
