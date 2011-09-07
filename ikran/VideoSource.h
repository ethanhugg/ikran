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
 * The Original Code is Rainbow.
 *
 * The Initial Developer of the Original Code is Mozilla Labs.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Anant Narayanan <anant@kix.in>
 *   Brian Coleman <brianfcoleman@gmail.com>
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

#ifndef _VIDEOSOURCE_H
#define _VIDEOSOURCE_H
#include "Convert.h"

#include <prlog.h>
#include <nsAutoPtr.h>
#include <nsThreadUtils.h>

#include <nsIOutputStream.h>
#include <nsIDOMCanvasRenderingContext2D.h>

/* Rendering on Canvas happens on the main thread as this runnable.
 * This is not very performant, we should move to rendering inside a <video>
 * so that Gecko can use hardware acceleration.
 */
class CanvasRenderer : public nsRunnable {
public:
    CanvasRenderer(
        nsIDOMCanvasRenderingContext2D *pCtx, PRUint32 width, PRUint32 height,
        nsAutoArrayPtr<PRUint8> &pData, PRUint32 pDataSize)
        :   m_pCtx(pCtx), m_width(width), m_height(height),
            m_pData(pData), m_pDataSize(pDataSize) {}

    NS_IMETHOD Run() {
        return m_pCtx->PutImageData_explicit(
            0, 0, m_width, m_height, m_pData.get(), m_pDataSize,
            PR_TRUE, 0, 0, m_width, m_height
        );
    }

private:
    nsIDOMCanvasRenderingContext2D *m_pCtx;
    PRUint32 m_width;
    PRUint32 m_height;
    nsAutoArrayPtr<PRUint8> m_pData;
    PRUint32 m_pDataSize;

};

#endif
