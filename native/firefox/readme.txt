## Steps to build firefox with ikran files

1.  Update to latest FF and rin a build, make sure build succeeds.
2.  Copy the modules/sip folder to here  <FF root>/modules/
3.  Copy the obj-ff-dbg/modules/sip folder to <FF root>/obj-ff-dbg/modules/
4.  Copy these two files  js/src/jssip.*   to   <FF root>/js/src/jssip*
5.  Copy js/src/Makefile.in   to  <FF root>/js/src/Makefile.in
6.  Update file   <FF root>/obj-ff-dbg/config/autoconf.mk 
    Add these lines

    NSPR_CFLAGS += -I../obj-ff-dbg/modules/sip/include \
    -I../../obj-ff-dbg/modules/sip/include \
    -I../../../obj-ff-dbg/modules/sip/include \
    -I../../../../obj-ff-dbg/modules/sip/include \
    -I../../../../../obj-ff-dbg/modules/sip/include \
    -I../../../../../../obj-ff-dbg/modules/sip/include

    NSPR_LIBS += -L../obj-ff-dbg/modules/sip/lib \
    -L../../obj-ff-dbg/modules/sip/lib \
    -L../../../obj-ff-dbg/modules/sip/lib \
    -L../../../../obj-ff-dbg/modules/sip/lib \
    -L../../../../../obj-ff-dbg/modules/sip/lib \
    -L../../../../../../obj-ff-dbg/modules/sip/lib \
    -lsessioncontrolbrowser

7.  Copy these files js/src/jsproto.tbl jsapi.cpp jsobj.h to here <FF root>/js/src/
8.  in this file ipc/chromium/src/base/basictypes.h   comment line 12 //#error You_must_include_basictypes.h_before_prtypes.h!
9.  Build ikran and copy this file ikran/libsessioncontrolbrowser.dylib to  <FF root>/obj-ff-dbg/modules/sip/lib 
10. Now build Firefox, it should build now.


## To Do
1.  Script inserting new text into js/src/Makefile and <FF root>/config/autoconf.mk.in
2.  Probably also want to script injecting new data into these files js/src/jsproto.tbl jsapi.cpp jsobj.h



## Text to add to files

file: jsapi.cpp

------
+#include "jssip.h"
+{js_InitSipClass,                   EAGER_ATOM_AND_CLASP(Sip)},
------

file jsobj.h

------
+extern Class SipClass;
------

file: jsproto.tbl

------
+JS_PROTO(Sip,                   37,     js_InitSipClass)
------


