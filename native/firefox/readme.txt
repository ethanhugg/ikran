# Build instructions
#  Two options 1) update to FF nightly revision 86237 and copy these files as an overlay
#              2) manually edit files described below then build 

# Following 4 lines is output from ht parent command showing the mozilla revision
emannion@emannion-ubuntu11:~/code/mozilla/mozilla_latest/mozilla-central$ hg parent
changeset:   86237:b077059c575a
tag:         tip
user:        Brian R. Bondy <netzen@gmail.com>
date:        Mon Feb 06 15:44:52 2012 -0500


# Steps to build firefox with ikran files

1.  Update to latest FF source and run a build, make sure the build succeeds.
2.  Copy the modules/sip folder to here  <FF root>/modules/
3.  Copy the modules/sip folder to <FF root>/modules/
4.  Copy these two files  js/src/jssip.*   to   <FF root>/js/src/jssip.*
5.  Make changes described below to <FF root>js/src/Makefile.in or copy file from overlay
6.  Make changes described below to <FF root>/config/autoconf.mk or copy from overlay 
7.  Make changes described below to <FF Root>js/src/jsproto.tbl, <FF root>/js/src/jsapi.cpp, <FF root>/js/src/jsobj.h or copy
8.  In this file ipc/chromium/src/base/basictypes.h   comment line 12 //#error You_must_include_basictypes.h_before_prtypes.h!
9.  [Linux only] Copy ./config/system-headers to <FF root>/config
10. [Linux only] Copy ./js/src/config/system-headers to <FF root>/js/src/config 
11. Build ikran and copy this file ikran/libsessioncontrolbrowser.dylib to  <FF root>/modules/sip/lib 
12. Now build Firefox, it should build.


## To Do
1.  Script inserting new text into js/src/Makefile and <FF root>/configure.in
2.  Probably also want to script injecting new data into these files js/src/jsproto.tbl jsapi.cpp jsobj.h


### Manual changes to files


##file: ./js/src/Makefile.in
------
jssip.cpp \

LOCAL_INCLUDES += \
    -I. \
    -I../../../modules/sip/include \
    -I../../../modules/sip/browser_logging \
    -I../../../ipc/chromium/src \
    -I../../../xpcom/base \
    -I../../xpcom \
    -I../../../modules/sip/sipcc/include/ \
    -I../../../modules/sip/webrtc \
    $(NULL)

ifneq ($(OS_ARCH),ANDROID)
# TEST_DIRS += jsapi-tests
endif

-------

##file: jsapi.cpp
------
+#include "jssip.h"
+{js_InitSipClass,                   EAGER_ATOM_AND_CLASP(Sip)},
------

##file jsobj.h
------
+extern Class SipClass;
------

##file: jsproto.tbl
------
+JS_PROTO(Sip,                   39,     js_InitSipClass)
------


##file: obj-i686-pc-linux-gnu/config/autoconf.mk
-----
mac (add onto NSPR_LIBS)
-L/usr/X11/lib -lX11 -lXext

linux (add onto NSPR_LIBS)
-L/usr/lib/i386-linux-gnu -lX11 -lXext

NSPR_CFLAGS += -I../modules/sip/include \
-I../../modules/sip/include \
-I../../../modules/sip/include \
-I../../../../modules/sip/include \
-I../../../../../modules/sip/include \
-I../../../../../../modules/sip/include

NSPR_LIBS += -L../modules/sip/lib \
-L../../modules/sip/lib \
-L../../../modules/sip/lib \
-L../../../../modules/sip/lib \
-L../../../../../modules/sip/lib \
-L../../../../../../modules/sip/lib \
-lsessioncontrolbrowser
-----

##file: ./config/system-headers
##      ./js/src/config/system-headers

linux only
--------
+sipcc_controller.h
--------

## file .mozconfig

ac_add_options --enable-debug --disable-optimize
mk_add_options MOZ_MAKE_FLAGS="JS_DISABLE_SHELL=1"
ac_add_options --disable-tests

