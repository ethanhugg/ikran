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

#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <nspr.h>

//#include <gtk/gtk.h>
//#include "libyuv.h" 

#include "vie_render.h"
#include "video_render_defines.h"
#include "common_types.h"
#include "sipcc_controller.h"

#ifdef XP_MACOSX
#include "CoreFoundation/CoreFoundation.h"
#endif

using namespace js;
using namespace js::types;

char* sessionCallback;
JSContext *mycx;
JSObject *myobj;
PRThread *search_tid;
Display* display;
Window* window;
Window* rootwindow;
int screen;

int RGB32toI420(int width, int height, const char *src, char *dst);
int I420toRGB32(int width, int height, const char *src, char *dst);

#ifdef XP_MACOSX
static void EndMyLoop();
#else
void CreateWindow() {
    display = XOpenDisplay(NULL);
    screen = DefaultScreen(display);
    rootwindow = new Window();
    *rootwindow = RootWindow(display, screen);
    window = new Window();
    *window = XCreateSimpleWindow(display, *rootwindow, 0, 0, 640, 500, 1, 0, 0);
    XMapWindow(display, *window);
    XFlush(display);
}
#endif

class VideoRenderer: public webrtc::ExternalRenderer {
public:
    VideoRenderer(int w, int h);
    ~VideoRenderer();
    Window* GetWindow() {return _window;};

protected:
    int FrameSizeChange(unsigned int width, unsigned int height, unsigned int numberOfStreams);
    int DeliverFrame(unsigned char* buffer, int bufferSize, unsigned timestamp);
    void CreateWindow(void *info);

private:
    Display* _display;
    Window* _window;
    Window* _rootwindow;
    int screen;
    unsigned int image_width, image_height, image_byte_size;
#ifdef XP_MACOSX
    char* image_data;
    Pixmap pixmap;
    GC gc;
    Visual *visual;
    int depth;
    XImage* image;
    XShmSegmentInfo _shminfo;
    unsigned char* _buffer;
    PRLock *render_lock;
#endif
};

VideoRenderer::VideoRenderer(int w, int h) : _display(NULL), _window(0L), _rootwindow(0L), image_width(w), image_height(h)
#ifdef XP_MACOSX
          ,_shminfo(), image(NULL), _buffer(NULL) 
#endif
{
    CreateWindow(NULL);
}

VideoRenderer::~VideoRenderer() {
  XDestroyWindow(_display, *_window);
#ifdef XP_MACOSX
  XShmDetach(_display, &_shminfo);
  XDestroyImage(image);
  shmdt(_shminfo.shmaddr);
  EndMyLoop();
#endif
}

void VideoRenderer::CreateWindow( void *info ) {
    _display = XOpenDisplay(NULL);
    screen = DefaultScreen(_display);
    _rootwindow = new Window();
    *_rootwindow = RootWindow(_display, screen);
    _window = new Window();
    *_window = XCreateSimpleWindow(_display, *_rootwindow, 0, 0, 640, 500, 1, 0, 0);

    //gtk_container_set_border_width (GTK_CONTAINER (_window), 10);

    XMapWindow(_display, *_window);
    XFlush(_display);
#ifdef XP_MACOSX
    depth = DefaultDepth (_display, screen);
    pixmap = XCreatePixmap (_display, *_rootwindow, image_width, image_height, depth);
    gc = XCreateGC (_display, pixmap, 0, 0);
    visual = DefaultVisual(_display, screen);
    image = XShmCreateImage(_display, CopyFromParent, 24, ZPixmap, NULL, &_shminfo, image_width, image_height);
    if(image == NULL) return;
    _shminfo.shmid = shmget(IPC_PRIVATE, (image->bytes_per_line * image->height), IPC_CREAT | 0777);
    if(_shminfo.shmid < 0) return;
    _shminfo.shmaddr = image->data = (char*) shmat(_shminfo.shmid, 0, 0);
    _buffer = (unsigned char*) image->data;
    _shminfo.readOnly = False;

    if (!XShmAttach(_display, &_shminfo)) {
        return;
    }
    //render_lock = PR_NewLock();
#endif
}

int VideoRenderer::FrameSizeChange(unsigned int width, unsigned int height, unsigned int numberOfStreams) {
    return -1;
}

int VideoRenderer::DeliverFrame(unsigned char* buffer, int bufferSize, unsigned int timestamp) {
#ifdef XP_MACOSX
    //PR_Lock(render_lock);
	
    unsigned char *pBuf = buffer;
    //webrtc::ConvertFromI420(pBuf, image_width, webrtc::kARGB, 0, image_width, image_height, _buffer);
    I420toRGB32(image_width, image_height, (const char *)pBuf, (char *)_buffer);

    XShmPutImage(_display, *_window, gc, image, 0, 0, 0, 0 , image_width, image_height, True);
    XSync(_display, false);

    //PR_DestroyLock(render_lock);
#endif
    return 0;
}

VideoRenderer *renderSource = NULL;

static void RunWindowThread ( void *info ) {
    CreateWindow();
    PR_Sleep(PR_MillisecondsToInterval(500));
}

#ifdef XP_MACOSX
static void _input(void *info) {
}

static void RunMyLoop ( void *info ) {
    if (renderSource == NULL)
        renderSource = new VideoRenderer(640, 480);

    PR_Sleep(PR_MillisecondsToInterval(500));

    CFRunLoopSourceRef source;
    CFRunLoopSourceContext source_context;

    bzero(&source_context, sizeof(source_context));
    source_context.perform = _input;
    source = CFRunLoopSourceCreate(NULL, 0, &source_context);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
    CFRunLoopRun();
}

static void EndMyLoop() {
    CFRunLoopStop(CFRunLoopGetCurrent());
}
#endif

JSBool invokeCallback() {
    JSBool ok;
    jsval rval;
    //JS_SetContextThread(mycx);
    //ok = JS_CallFunctionName(mycx, JS_GetGlobalObject(mycx), "foo", 0, NULL, &rval);
    return JS_TRUE;
}

void CallControl::OnIncomingCall(std::string callingPartyName, std::string callingPartyNumber) {
}

void CallControl::OnRegisterStateChange(std::string registrationState) {
}

void CallControl::OnCallTerminated() {
    invokeCallback();
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

#ifdef XP_MACOSX
    if ( (search_tid = PR_CreateThread( PR_USER_THREAD, RunMyLoop,
#else
    if ( (search_tid = PR_CreateThread( PR_USER_THREAD, RunWindowThread,
#endif
            NULL, PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_UNJOINABLE_THREAD,
            0 )) == NULL ) {
                perror( "PR_CreateThread search_thread" );
                exit( 1 );
    }

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

/*
#ifdef XP_MACOSX
    if ( (search_tid = PR_CreateThread( PR_USER_THREAD, RunMyLoop,
#else
    if ( (search_tid = PR_CreateThread( PR_USER_THREAD, RunWindowThread,
#endif
            NULL, PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_UNJOINABLE_THREAD,
            0 )) == NULL ) {
                perror( "PR_CreateThread search_thread" );
                exit( 1 );
    }

    PR_Sleep(PR_MillisecondsToInterval(1000));
*/

#ifdef XP_MACOSX
    SipccController::GetInstance()->SetExternalRenderer(renderSource);
    SipccController::GetInstance()->PlaceCall(JS_EncodeString(cx, fmt), (char *) "", 0, 0);
#else
    SipccController::GetInstance()->PlaceCallWithWindow((void*)*window, JS_EncodeString(cx, fmt), (char *) "", 0, 0);
#endif

    return true;
}

static JSBool
sip_setVideoWindow(JSContext *cx, uintN argc, Value *vp) {
    CallArgs args = CallArgsFromVp(argc, vp);
  
    if (JSVAL_IS_OBJECT(*vp)) { 
        JSObject *obj = JSVAL_TO_OBJECT(*vp);
        //SipccController::GetInstance()->SetExternalRenderer((void*)obj);
    }
    return true;
}

static JSBool
sip_answerCall(JSContext *cx, uintN argc, Value *vp) {
    SipccController::GetInstance()->AnswerCall();
    return true;
}

static JSBool
sip_endCall(JSContext *cx, uintN argc, Value *vp) {
    SipccController::GetInstance()->EndCall();
    return true;
}

static JSBool
sip_unRegister(JSContext *cx, uintN argc, Value *vp) {
    SipccController::GetInstance()->RemoveSipccControllerObserver();
    SipccController::GetInstance()->UnRegister();

#ifdef XP_MACOSX
    if (renderSource != NULL) {
        delete renderSource;
        renderSource = NULL;
    }
#endif
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
    JS_FN("answerCall", sip_answerCall, 0, 0),
    JS_FN("endCall", sip_endCall, 0, 0),
    JS_FN("unRegister", sip_unRegister, 0, 0),
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


enum {
	CLIP_SIZE = 811,
	CLIP_OFFSET = 277,
	YMUL = 298,
	RMUL = 409,
	BMUL = 516,
	G1MUL = -100,
	G2MUL = -208,
};

static int tables_initialized = 0;

static int yuv2rgb_y[256];
static int yuv2rgb_r[256];
static int yuv2rgb_b[256];
static int yuv2rgb_g1[256];
static int yuv2rgb_g2[256];

static unsigned long yuv2rgb_clip[CLIP_SIZE];
static unsigned long yuv2rgb_clip8[CLIP_SIZE];
static unsigned long yuv2rgb_clip16[CLIP_SIZE];

#define COMPOSE_RGB(yc, rc, gc, bc)		\
	( 0xff000000 |				\
	  yuv2rgb_clip16[(yc) + (rc)] |		\
	  yuv2rgb_clip8[(yc) + (gc)] |		\
	  yuv2rgb_clip[(yc) + (bc)] )


static void init_yuv2rgb_tables(void)
{
	int i;

	for (i = 0; i < 256; ++i) {
		yuv2rgb_y[i] = (YMUL * (i - 16) + 128) >> 8;
		yuv2rgb_r[i] = (RMUL * (i - 128)) >> 8;
		yuv2rgb_b[i] = (BMUL * (i - 128)) >> 8;
		yuv2rgb_g1[i] = (G1MUL * (i - 128)) >> 8;
		yuv2rgb_g2[i] = (G2MUL * (i - 128)) >> 8;
	}
	for (i = 0 ; i < CLIP_OFFSET; ++i) {
		yuv2rgb_clip[i] = 0;
		yuv2rgb_clip8[i] = 0;
		yuv2rgb_clip16[i] = 0;
	}
	for (; i < CLIP_OFFSET + 256; ++i) {
		yuv2rgb_clip[i] = i - CLIP_OFFSET;
		yuv2rgb_clip8[i] = (i - CLIP_OFFSET) << 8;
		yuv2rgb_clip16[i] = (i - CLIP_OFFSET) << 16;
	}
	for (; i < CLIP_SIZE; ++i) {
		yuv2rgb_clip[i] = 255;
		yuv2rgb_clip8[i] = 255 << 8;
		yuv2rgb_clip16[i] = 255 << 16;
	}

	tables_initialized = 1;
}

/*
 * Convert i420 to RGB32 (0xBBGGRRAA).
 * NOTE: size of dest must be >= width * height * 4
 *
 * This function uses precalculated tables that are initialized
 * on the first run.
 */
int
I420toRGB32(int width, int height, const char *src, char *dst)
{
	int i, j;
	unsigned int *dst_odd;
	unsigned int *dst_even;
	const unsigned char *u;
	const unsigned char *v;
	const unsigned char *y_odd;
	const unsigned char *y_even;

	if (!tables_initialized)
		init_yuv2rgb_tables();

	dst_even = (unsigned int *)dst;
	dst_odd = dst_even + width;

	y_even = (const unsigned char *)src;
	y_odd = y_even + width;
	u = y_even + width * height;
	v = u + ((width * height) >> 2);

	for (i = 0; i < height / 2; ++i) {
		for (j = 0; j < width / 2; ++j) {
			const int rc = yuv2rgb_r[*v];
			const int gc = yuv2rgb_g1[*v] + yuv2rgb_g2[*u];
			const int bc = yuv2rgb_b[*u];
			const int yc0_even = CLIP_OFFSET + yuv2rgb_y[*y_even++];
			const int yc1_even = CLIP_OFFSET + yuv2rgb_y[*y_even++];
			const int yc0_odd = CLIP_OFFSET + yuv2rgb_y[*y_odd++];
			const int yc1_odd = CLIP_OFFSET + yuv2rgb_y[*y_odd++];

			*dst_even++ = COMPOSE_RGB(yc0_even, bc, gc, rc);
			*dst_even++ = COMPOSE_RGB(yc1_even, bc, gc, rc);
			*dst_odd++ = COMPOSE_RGB(yc0_odd, bc, gc, rc);
			*dst_odd++ = COMPOSE_RGB(yc1_odd, bc, gc, rc);

			++u;
			++v;
		}

		y_even += width;
		y_odd += width;
		dst_even += width;
		dst_odd += width;
	}

	return 0;
}

/*
 * Convert RGB32 to i420. NOTE: size of dest must be >= width * height * 3 / 2
 * Based on formulas found at http://en.wikipedia.org/wiki/YUV  (libvidcap)
 */
int
RGB32toI420(int width, int height, const char *src, char *dst)
{
    int i, j;
    unsigned char *dst_y_even;
    unsigned char *dst_y_odd;
    unsigned char *dst_u;
    unsigned char *dst_v;
    const unsigned char *src_even;
    const unsigned char *src_odd;

    src_even = (const unsigned char *)src;
    src_odd = src_even + width * 4;

    dst_y_even = (unsigned char *)dst;
    dst_y_odd = dst_y_even + width;
    dst_u = dst_y_even + width * height;
    dst_v = dst_u + ((width * height) >> 2);

    for (i = 0; i < height / 2; ++i) {
        for (j = 0; j < width / 2; ++j) {
            short r, g, b;
            r = *src_even++;
            g = *src_even++;
            b = *src_even++;

            ++src_even;
            *dst_y_even++ = (unsigned char)
                ((( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16);
            *dst_u++ = (unsigned char)
                ((( r * -38 - g * 74 + b * 112 + 128 ) >> 8 ) + 128);
            *dst_v++ = (unsigned char)
                ((( r * 112 - g * 94 - b * 18 + 128 ) >> 8 ) + 128);

            r = *src_even++;
            g = *src_even++;
            b = *src_even++;
            ++src_even;
            *dst_y_even++ = (unsigned char)
                ((( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16);

            r = *src_odd++;
            g = *src_odd++;
            b = *src_odd++;
            ++src_odd;
            *dst_y_odd++ = (unsigned char)
                ((( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16);

            r = *src_odd++;
            g = *src_odd++;
            b = *src_odd++;
            ++src_odd;
            *dst_y_odd++ = (unsigned char)
                ((( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16);
        }

        dst_y_odd += width;
        dst_y_even += width;
        src_odd += width * 4;
        src_even += width * 4;
    }

    return 0;
}

