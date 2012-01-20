Steps to build firefox with ikran files

1.  Updates to latest FF and build, i run a build and make sure build succeeds.
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
8.  Build ikran and copy this file ikran/libsessioncontrolbrowser.dylib to  <FF root>/obj-ff-dbg/modules/sip/lib 
8.  It should build now.


To Do
1.  Script inserting new text into js/src/Makefile and <FF root>/config/autoconf.mk.in
2.  Probably also want to script injecting new data into these files js/src/jsproto.tbl jsapi.cpp jsobj.h


