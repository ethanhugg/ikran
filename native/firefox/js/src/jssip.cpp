/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

/*
 * JS sip implementation.
 */
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jssip.h"
#include "jscntxt.h"
#include "jsinfer.h"
#include "jsversion.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"

#include "vm/GlobalObject.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

#include "sipcc_controller.h"

using namespace js;
using namespace js::types;

char* sessionCallback;
JSContext *mycx;
JSObject *myobj;


JSBool invokeCallback() {
    JSBool ok;
    jsval rval;

    ok = JS_CallFunctionName(mycx, JS_GetGlobalObject(mycx), "foo", 0, NULL, &rval);
    return JS_TRUE;
}

/*  for function callback see these
https://developer.mozilla.org/en/SpiderMonkey/JSAPI_Reference/JS_CallFunctionName
https://developer.mozilla.org/en/JSAPI_User_Guide
*/

void CallControl::OnIncomingCall(std::string callingPartyName, std::string callingPartyNumber) {
}

void CallControl::OnRegisterStateChange(std::string registrationState) {
}

void CallControl::OnCallTerminated() {
    //invokeCallback();
}

void CallControl::OnCallConnected(char* sdp) {
    invokeCallback();
}

void CallControl::OnCallHeld() {
}

void CallControl::OnCallResume() {
}

CallControl *cControl;

Class js::SipClass = {
    "Sip",
    JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_HAS_CACHED_PROTO(JSProto_Sip),    
    JS_PropertyStub,         /* addProperty */
    JS_PropertyStub,         /* delProperty */
    JS_PropertyStub,         /* getProperty */
    JS_StrictPropertyStub,   /* setProperty */
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static JSBool
sip_register(JSContext *cx, uintN argc, Value *vp) {
    mycx = cx;
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *fmt = ToString(cx, args[0]);
    JSString *fmt1 = ToString(cx, args[1]);
    JSString *fmt2 = ToString(cx, args[2]);
    JSString *fmt3 = ToString(cx, args[3]);
    SipccController::GetInstance()->AddSipccControllerObserver(cControl);
    SipccController::GetInstance()->Register(JS_EncodeString(cx, fmt), JS_EncodeString(cx, fmt1), JS_EncodeString(cx, fmt2), JS_EncodeString(cx, fmt3));
    return true;
}

static JSBool
sip_placeCall(JSContext *cx, uintN argc, Value *vp) {
    mycx = cx;
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *fmt = ToString(cx, args[0]);
    SipccController::GetInstance()->PlaceCall(JS_EncodeString(cx, fmt), (char *) "", 0, 0);
    return true;
}

static JSBool
sip_setVideoWindow(JSContext *cx, uintN argc, Value *vp) {
    CallArgs args = CallArgsFromVp(argc, vp);
  
    if (JSVAL_IS_OBJECT(*vp)) { 
        //JSObject *obj = JSVAL_TO_OBJECT(*vp);
        //SipccController::GetInstance()->SetExternalRenderer((void*)obj);
    }
    return true;
}

static JSBool
sip_endCall(JSContext *cx, uintN argc, Value *vp) {
    SipccController::GetInstance()->EndCall();
    return true;
}

static JSBool
sip_setSessionCallback(JSContext *cx, uintN argc, Value *vp) {
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *fmt = ToString(cx, args[0]);
    strcpy(sessionCallback, JS_EncodeString(cx, fmt));
    return true;
}

static JSFunctionSpec sip_methods[] = {
    JS_FN("register", sip_register,  4, 0),
    JS_FN("placeCall", sip_placeCall, 1, 0),
    JS_FN("setVideoWindow", sip_setVideoWindow, 1, 0),
    JS_FN("endCall", sip_endCall, 0, 0),
    JS_FN("setSessionCallback", sip_setSessionCallback, 1, 0),
    JS_FS_END
};

static JSBool
js_Sip(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &SipClass);
    if (!obj)
        return false;
    vp->setObject(*obj);
    return true;
}

JSObject *
js_InitSipClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = &obj->asGlobal();

    JSObject *sipProto = global->createBlankPrototype(cx, &SipClass);
    if (!sipProto)
        return NULL;

    JSFunction *ctor = global->createConstructor(cx, js_Sip, &SipClass,
                                                 CLASS_ATOM(cx, Sip), 1);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, sipProto))
        return NULL;

    if (!JS_DefineFunctions(cx, sipProto, sip_methods))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_Sip, ctor, sipProto))
        return NULL;

    cControl = new CallControl;
    mycx = cx;
    myobj = obj;

    return sipProto;
}

