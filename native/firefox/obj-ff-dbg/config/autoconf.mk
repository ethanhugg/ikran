#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is this file as it was released upon August 6, 1998.
#
# The Initial Developer of the Original Code is
# Christopher Seawood.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Benjamin Smedberg <benjamin@smedbergs.us>
#
# Alternatively, the contents of this file may be used under the terms of
# either of the GNU General Public License Version 2 or later (the "GPL"),
# or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

# A netscape style .mk file for autoconf builds

INCLUDED_AUTOCONF_MK = 1
USE_AUTOCONF 	= 1
MOZILLA_CLIENT	= 1
target          = i386-apple-darwin9.2.0
ac_configure_args =  --enable-application=browser --enable-debug --disable-tests --disable-optimize --target=i386-apple-darwin9.2.0 --enable-macos-target=10.5 --disable-crashreporter
MOZILLA_VERSION = 12.0a1
FIREFOX_VERSION	= 12.0a1

MOZ_BUILD_APP = browser
MOZ_APP_NAME	= firefox
MOZ_APP_DISPLAYNAME = Nightly
MOZ_APP_BASENAME = Firefox
MOZ_APP_VENDOR = Mozilla
MOZ_APP_PROFILE = 
MOZ_APP_ID = {ec8030f7-c20a-464f-9b0e-13a3a9e97384}
MOZ_PROFILE_MIGRATOR = 1
MOZ_EXTENSION_MANAGER = 1
MOZ_APP_UA_NAME = 
MOZ_APP_VERSION = 12.0a1
MOZ_UA_BUILDID = 
MOZ_MACBUNDLE_NAME = NightlyDebug.app
MOZ_APP_STATIC_INI = 1

MOZ_PKG_SPECIAL = 

prefix		= /usr/local
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
includedir	= ${prefix}/include/$(MOZ_APP_NAME)-$(MOZ_APP_VERSION)
libdir		= ${exec_prefix}/lib
datadir		= ${prefix}/share
mandir		= ${prefix}/man
idldir		= $(datadir)/idl/$(MOZ_APP_NAME)-$(MOZ_APP_VERSION)

installdir	= $(libdir)/$(MOZ_APP_NAME)-$(MOZ_APP_VERSION)
sdkdir		= $(libdir)/$(MOZ_APP_NAME)-devel-$(MOZ_APP_VERSION)

DIST		= $(DEPTH)/dist
LIBXUL_SDK      = 

MOZ_FS_LAYOUT = bundle

L10NBASEDIR     = 

LIBXUL_DIST	= /Users/Enda/code/mozilla/mozilla_try/mozilla-central/obj-ff-dbg/dist
SYSTEM_LIBXUL   = 

XULRUNNER_STUB_NAME = xulrunner

MOZ_CHROME_FILE_FORMAT	= symlink
MOZ_OMNIJAR		= 1
OMNIJAR_NAME		= omni.ja

MOZ_WIDGET_TOOLKIT	= cocoa
MOZ_GFX_OPTIMIZE_MOBILE = 

MOZ_X11			= 

MOZ_PANGO = 1

MOZ_JS_LIBS		   = $(call EXPAND_LIBNAME_PATH,js_static,$(LIBXUL_DIST)/lib)

MOZ_DEBUG	= 1
MOZ_DEBUG_SYMBOLS = 1
MOZ_DEBUG_ENABLE_DEFS		= -DDEBUG -D_DEBUG -DTRACING
MOZ_DEBUG_DISABLE_DEFS	= -DNDEBUG -DTRIMMED
MOZ_DEBUG_FLAGS	= -g
MOZ_DEBUG_LDFLAGS= -framework ExceptionHandling
MOZ_EXTENSIONS  = 
MOZ_JSDEBUGGER  = 1
MOZ_IPDL_TESTS 	= 
MOZ_LEAKY	= 
MOZ_MEMORY      = 
MOZ_PROFILING   = 
MOZ_ENABLE_PROFILER_SPS = 
MOZ_JPROF       = 
MOZ_SHARK       = 
MOZ_CALLGRIND   = 
MOZ_VTUNE       = 
MOZ_ETW         = 
MOZ_TRACE_JSCALLS = @MOZ_TRACE_JSCALLS@
DEHYDRA_PATH    = 

MOZ_LINKER = 
MOZ_OLD_LINKER = 
NS_TRACE_MALLOC = 
USE_ELF_DYNSTR_GC = 
USE_ELF_HACK = 
STDCXX_COMPAT = 
MOZ_LIBSTDCXX_TARGET_VERSION=
MOZ_LIBSTDCXX_HOST_VERSION=
INCREMENTAL_LINKER = 
MACOSX_DEPLOYMENT_TARGET = 10.5
ENABLE_TESTS	= 
IBMBIDI = 1
MOZ_UNIVERSALCHARDET = 1
ACCESSIBILITY = 
MOZ_BRANDING_DIRECTORY = browser/branding/nightly
XPCOM_USE_LEA = @XPCOM_USE_LEA@
MOZ_INSTALLER	= 1
MOZ_MAINTENANCE_SERVICE	= 
MOZ_UPDATER	= 1
MOZ_UPDATE_CHANNEL	= default
MOZ_UPDATE_PACKAGING	= 
MOZ_DISABLE_PARENTAL_CONTROLS = 
NS_ENABLE_TSF = 
MOZ_SPELLCHECK = 1
MOZ_ANDROID_HISTORY = 
MOZ_JAVA_COMPOSITOR = 
MOZ_PROFILELOCKING = 1
MOZ_FEEDS = 1
MOZ_TOOLKIT_SEARCH = 1
MOZ_PLACES = 1
MOZ_SAFE_BROWSING = 1
MOZ_URL_CLASSIFIER = 1
MOZ_ZIPWRITER = 1
MOZ_OGG = 1
MOZ_RAW = 
MOZ_SYDNEYAUDIO = 1
MOZ_WAVE = 1
MOZ_MEDIA = 1
MOZ_VORBIS = 1
MOZ_TREMOR = 
MOZ_NO_THEORA_ASM = 
MOZ_WEBM = 1
MOZ_VP8_ERROR_CONCEALMENT = 
MOZ_VP8_ENCODER = 
VPX_AS = yasm
VPX_ASFLAGS = -f macho32 -rnasm -pnasm -DPIC
VPX_DASH_C_FLAG = 
VPX_AS_CONVERSION = 
VPX_ASM_SUFFIX = asm
VPX_X86_ASM = 1
VPX_ARM_ASM = 
VPX_NEED_OBJ_INT_EXTRACT = 
LIBJPEG_TURBO_AS = yasm
LIBJPEG_TURBO_ASFLAGS = -f macho32 -rnasm -pnasm -DPIC -DMACHO
LIBJPEG_TURBO_X86_ASM = 1
LIBJPEG_TURBO_X64_ASM = 
NS_PRINTING = 1
MOZ_PDF_PRINTING = 
MOZ_CRASHREPORTER = 
MOZ_HELP_VIEWER = 
MOC = 
RCC = 
MOZ_NSS_PATCH = 
MOZ_WEBGL = 1
MOZ_ANGLE = 
MOZ_DIRECTX_SDK_PATH = 
MOZ_DIRECTX_SDK_CPU_SUFFIX = x86
MOZ_D3DX9_VERSION = 
MOZ_D3DX9_CAB = 
MOZ_D3DCOMPILER_CAB = 
MOZ_D3DX9_DLL = 
MOZ_D3DCOMPILER_DLL = 


JAVA="/usr/bin/java"
JAVAC="/usr/bin/javac"
JAR="/usr/bin/jar"

TAR=gnutar

MAKENSISU=

RM = rm -f

# The MOZ_UI_LOCALE var is used to build a particular locale. Do *not*
# use the var to change any binary files. Do *not* use this var unless you
# write rules for the "clean-locale" and "locale" targets.
MOZ_UI_LOCALE = en-US

MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS = 
MOZ_COMPONENT_NSPR_LIBS=-L$(LIBXUL_DIST)/bin $(NSPR_LIBS)

MOZ_FIX_LINK_PATHS=-Wl,-executable_path,$(LIBXUL_DIST)/bin

XPCOM_FROZEN_LDOPTS=-L$(LIBXUL_DIST)/bin -lxpcom -lmozalloc
XPCOM_LIBS=$(XPCOM_FROZEN_LDOPTS) $(LIBXUL_DIST)/bin/XUL
LIBXUL_LIBS=$(XPCOM_FROZEN_LDOPTS) $(LIBXUL_DIST)/bin/XUL

ENABLE_STRIP	= 
PKG_SKIP_STRIP	= 

MOZ_POST_DSO_LIB_COMMAND = 
MOZ_POST_PROGRAM_COMMAND = 

MOZ_BUILD_ROOT             = /Users/Enda/code/mozilla/mozilla_try/mozilla-central/obj-ff-dbg

MOZ_XUL                    = 1

NECKO_PROTOCOLS = about data file ftp http res viewsource websocket wyciwyg device
NECKO_COOKIES = 1
NECKO_WIFI = 1
MOZ_AUTH_EXTENSION = 1

MOZ_NATIVE_HUNSPELL = 
MOZ_HUNSPELL_LIBS = 
MOZ_HUNSPELL_CFLAGS = 

MOZ_NATIVE_LIBEVENT = 
MOZ_LIBEVENT_LIBS = 
MOZ_LIBEVENT_INCLUDES = 

MOZ_NATIVE_LIBVPX = 
MOZ_LIBVPX_LIBS = 
MOZ_LIBVPX_INCLUDES = 

MOZ_NATIVE_ZLIB	= 
MOZ_NATIVE_BZ2	= 
MOZ_NATIVE_JPEG	= 
MOZ_NATIVE_PNG	= 
MOZ_TREE_CAIRO = 1
MOZ_TREE_PIXMAN = 1

MOZ_UPDATE_XTERM = 
MOZ_PERMISSIONS = 1
MOZ_XTF = 1
MOZ_SVG_DLISTS = @MOZ_SVG_DLISTS@
MOZ_CAIRO_CFLAGS = -I$(LIBXUL_DIST)/include/cairo

MOZ_PREF_EXTENSIONS = 1

MOZ_CAIRO_LIBS = $(call EXPAND_LIBNAME_PATH,mozcairo,$(DEPTH)/gfx/cairo/cairo/src)  $(call EXPAND_LIBNAME_PATH,mozlibpixman,$(DEPTH)/gfx/cairo/libpixman/src)

MOZ_ENABLE_GNOMEUI = 
MOZ_GNOMEUI_CFLAGS = 
MOZ_GNOMEUI_LIBS = 

MOZ_ENABLE_STARTUP_NOTIFICATION = 
MOZ_STARTUP_NOTIFICATION_CFLAGS = 
MOZ_STARTUP_NOTIFICATION_LIBS = 

MOZ_ENABLE_GNOMEVFS = 
MOZ_GNOMEVFS_CFLAGS = 
MOZ_GNOMEVFS_LIBS = 

MOZ_ENABLE_GCONF = 
MOZ_GCONF_CFLAGS = 
MOZ_GCONF_LIBS = 

MOZ_ENABLE_GNOME_COMPONENT = 

MOZ_ENABLE_GIO = 
MOZ_GIO_CFLAGS = 
MOZ_GIO_LIBS = 

MOZ_NATIVE_NSPR = 
MOZ_NATIVE_NSS = 

MOZ_B2G_RIL = 

BUILD_CTYPES = 1

COMPILE_ENVIRONMENT = 1
CROSS_COMPILE   = 1

WCHAR_CFLAGS	= -fshort-wchar

OS_CPPFLAGS	= 
OS_CFLAGS	= $(OS_CPPFLAGS) -Wall -W -Wno-unused -Wpointer-arith -Wdeclaration-after-statement -Wcast-align -W -fno-strict-aliasing -fno-common -pthread -DNO_X11 -pipe
OS_CXXFLAGS	= $(OS_CPPFLAGS) -fno-rtti -Wall -Wpointer-arith -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wcast-align -Wno-invalid-offsetof -Wno-variadic-macros -Werror=return-type -fno-exceptions -fno-strict-aliasing -fno-common -fshort-wchar -pthread -DNO_X11 -pipe
OS_LDFLAGS	=   -framework Cocoa -lobjc  -L/usr/X11/lib -lX11

OS_COMPILE_CFLAGS = $(OS_CPPFLAGS) -include $(DEPTH)/mozilla-config.h -DMOZILLA_CLIENT $(filter-out %/.pp,-MD -MF $(MDDEPDIR)/$(basename $(@F)).pp)
OS_COMPILE_CXXFLAGS = $(OS_CPPFLAGS) -DMOZILLA_CLIENT -include $(DEPTH)/mozilla-config.h $(filter-out %/.pp,-MD -MF $(MDDEPDIR)/$(basename $(@F)).pp)

OS_INCLUDES	= $(NSPR_CFLAGS) $(NSS_CFLAGS) $(JPEG_CFLAGS) $(PNG_CFLAGS) $(ZLIB_CFLAGS)
OS_LIBS		= 
ACDEFINES	=  -DCROSS_COMPILE=1 -DX_DISPLAY_MISSING=1 -DMOZILLA_VERSION=\"12.0a1\" -DMOZILLA_VERSION_U=12.0a1 -DXP_MACOSX=1 -DXP_DARWIN=1 -DD_INO=d_ino -DSTDC_HEADERS=1 -DHAVE_SSIZE_T=1 -DHAVE_ST_BLKSIZE=1 -DHAVE_SIGINFO_T=1 -DHAVE_UINT=1 -DHAVE_VISIBILITY_HIDDEN_ATTRIBUTE=1 -DHAVE_VISIBILITY_ATTRIBUTE=1 -DHAVE_DIRENT_H=1 -DHAVE_GETOPT_H=1 -DHAVE_MEMORY_H=1 -DHAVE_UNISTD_H=1 -DHAVE_NL_TYPES_H=1 -DHAVE_X11_XKBLIB_H=1 -DHAVE_SYS_STATVFS_H=1 -DHAVE_MMINTRIN_H=1 -DHAVE_SYS_CDEFS_H=1 -DHAVE_DLADDR=1 -DNO_X11=1 -DHAVE_RANDOM=1 -DHAVE_STRERROR=1 -DHAVE_LCHOWN=1 -DHAVE_FCHMOD=1 -DHAVE_SNPRINTF=1 -DHAVE_MEMMOVE=1 -DHAVE_RINT=1 -DHAVE_STAT64=1 -DHAVE_LSTAT64=1 -DHAVE_SETBUF=1 -DHAVE_ISATTY=1 -DHAVE_FLOCKFILE=1 -DHAVE_LOCALTIME_R=1 -DHAVE_STRTOK_R=1 -DHAVE_LANGINFO_CODESET=1 -DVA_COPY=va_copy -DHAVE_VA_COPY=1 -DMALLOC_H=\<malloc/malloc.h\> -DHAVE_I18N_LC_MESSAGES=1 -DHAVE_LOCALECONV=1 -DNS_ALWAYS_INLINE=__attribute__\(\(always_inline\)\) -DNS_ATTR_MALLOC=__attribute__\(\(malloc\)\) -DNS_WARN_UNUSED_RESULT=__attribute__\(\(warn_unused_result\)\) -DMOZ_PHOENIX=1 -DMOZ_BUILD_APP=browser -DMOZ_WIDGET_COCOA=1 -DMOZ_INSTRUMENT_EVENT_LOOP=1 -DMOZ_DISTRIBUTION_ID=\"org.mozilla\" -DIBMBIDI=1 -DNS_PRINTING=1 -DNS_PRINT_PREVIEW=1 -DMOZ_OGG=1 -DATTRIBUTE_ALIGNED_MAX=64 -DMOZ_WEBM=1 -DVPX_X86_ASM=1 -DMOZ_WAVE=1 -DMOZ_SYDNEYAUDIO=1 -DMOZ_MEDIA=1 -DMOZ_VORBIS=1 -DMOZ_XTF=1 -DENABLE_SYSTEM_EXTENSION_DIRS=1 -DMOZ_CRASHREPORTER_ENABLE_PERCENT=100 -DLIBJPEG_TURBO_X86_ASM=1 -DMOZ_UPDATER=1 -DMOZ_UPDATE_CHANNEL=default -DMOZ_FEEDS=1 -DMOZ_SAFE_BROWSING=1 -DMOZ_URL_CLASSIFIER=1 -DMOZ_DEBUG_SYMBOLS=1 -DMOZ_LOGGING=1 -DMOZ_DUMP_PAINTING=1 -DJSGC_INCREMENTAL=1 -DHAVE___CXA_DEMANGLE=1 -DMOZ_DEMANGLE_SYMBOLS=1 -DHAVE__UNWIND_BACKTRACE=1 -DJS_DEFAULT_JITREPORT_GRANULARITY=3 -DMOZ_OMNIJAR=1 -DMOZ_USER_DIR=\"Mozilla\" -DMOZ_STATIC_JS=1 -DHAVE_STDINT_H=1 -DHAVE_INTTYPES_H=1 -DMOZ_TREE_CAIRO=1 -DHAVE_UINT64_T=1 -DMOZ_TREE_PIXMAN=1 -DMOZ_GRAPHITE=1 -DMOZ_ENABLE_SKIA=1 -DMOZ_XUL=1 -DMOZ_PROFILELOCKING=1 -DBUILD_CTYPES=1 -DMOZ_PLACES=1 -DMOZ_SERVICES_SYNC=1 -DMOZ_APP_UA_NAME=\"\" -DMOZ_APP_UA_VERSION=\"12.0a1\" -DMOZ_UA_FIREFOX_VERSION=\"12.0a1\" -DFIREFOX_VERSION=12.0a1 -DMOZ_UA_BUILDID=\"\" -DMOZ_DLL_SUFFIX=\".dylib\" -DXP_UNIX=1 -DUNIX_ASYNC_DNS=1 -DMOZ_REFLOW_PERF=1 -DMOZ_REFLOW_PERF_DSP=1 

WARNINGS_AS_ERRORS = 

MOZ_OPTIMIZE	= 
MOZ_FRAMEPTR_FLAGS = -fno-omit-frame-pointer
MOZ_OPTIMIZE_FLAGS = -O3
MOZ_PGO_OPTIMIZE_FLAGS = 
MOZ_OPTIMIZE_LDFLAGS = -Wl,-dead_strip
MOZ_OPTIMIZE_SIZE_TWEAK = 

MOZ_RTTI_FLAGS_ON = -frtti

PROFILE_GEN_CFLAGS = 
PROFILE_GEN_LDFLAGS = 
PROFILE_USE_CFLAGS = 
PROFILE_USE_LDFLAGS = 

XCFLAGS		= 
XLDFLAGS	= 
XLIBS		= 
XEXT_LIBS	= 
XCOMPOSITE_LIBS	= 
XSS_LIBS	= 

MOZ_THUMB2	= 
MOZ_EGL_XRENDER_COMPOSITE	= 

WIN_TOP_SRC	= 
AR		= ar
AR_FLAGS	= cr $@
AR_EXTRACT	= $(AR) x
AR_LIST		= $(AR) t
AR_DELETE	= $(AR) d
AS		= $(CC)
ASFLAGS		=  -fPIC
AS_DASH_C_FLAG	= -c
LD		= ld
RC		= 
RCFLAGS		= 
MC		= 
WINDRES		= :
IMPLIB		= 
FILTER		= 
BIN_FLAGS	= 
MIDL		= 
MIDL_FLAGS	= 
_MSC_VER	= 

DLL_PREFIX	= lib
LIB_PREFIX	= lib
# We do magic with OBJ_SUFFIX in config.mk, the following ensures we don't
# manually use it before config.mk inclusion
OBJ_SUFFIX	= $(error config/config.mk needs to be included before using OBJ_SUFFIX)
_OBJ_SUFFIX	= o
LIB_SUFFIX	= a
DLL_SUFFIX	= .dylib
BIN_SUFFIX	= 
ASM_SUFFIX	= s
IMPORT_LIB_SUFFIX = 
LIBS_DESC_SUFFIX = desc
USE_N32		= 
HAVE_64BIT_OS	= 

CC		    = gcc-4.2 -arch i386
CXX		    = g++-4.2 -arch i386
CPP       = gcc-4.2 -arch i386 -E

CC_VERSION	= gcc version 4.2.1 (Apple Inc. build 5646)
CXX_VERSION	= gcc version 4.2.1 (Apple Inc. build 5646)

GNU_AS		= 
GNU_LD		= 
GNU_CC		= 1
GNU_CXX		= 1
INTEL_CC	= 
INTEL_CXX	= 

STL_FLAGS		= 
WRAP_STL_INCLUDES	= 
MOZ_MSVC_STL_WRAP__Throw= 
MOZ_MSVC_STL_WRAP__RAISE= 

HOST_CC		= gcc-4.2
HOST_CXX	= g++-4.2
HOST_CFLAGS	=  -DXP_UNIX -DXP_MACOSX -DNO_X11
HOST_CXXFLAGS	= 
HOST_LDFLAGS	= 
HOST_OPTIMIZE_FLAGS = -O3
HOST_NSPR_MDCPUCFG = \"md/_darwin.cfg\"
HOST_AR		= ar
HOST_AR_FLAGS	= cr $@
HOST_LD		= 
HOST_RANLIB	= ranlib
HOST_BIN_SUFFIX	= 

HOST_OS_ARCH	= Darwin
host_cpu	= x86_64
host_vendor	= apple
host_os		= darwin10.8.0

TARGET_NSPR_MDCPUCFG = \"md/_darwin.cfg\"
TARGET_CPU	= i386
TARGET_VENDOR	= apple
TARGET_OS	= darwin9.2.0
TARGET_MD_ARCH	= unix
TARGET_XPCOM_ABI = x86-gcc3

AUTOCONF	= /opt/local/bin/autoconf
GMAKE		= /usr/bin/make
PERL		= /opt/local/bin/perl5
PYTHON		= /opt/local/bin/python2.6
RANLIB		= ranlib
OBJCOPY		= 
UNZIP		= /usr/bin/unzip
ZIP		= /usr/bin/zip
XARGS		= /usr/bin/xargs
STRIP		= string -x -S -x -S
DOXYGEN		= :
PBBUILD_BIN	= /usr/bin/xcodebuild
SDP		= /usr/bin/sdp
NSINSTALL_BIN	= 
WGET		= 
RPMBUILD	= :

ifdef MOZ_NATIVE_JPEG
JPEG_CFLAGS	= 
JPEG_LIBS	= 
JPEG_REQUIRES	=
else
JPEG_CFLAGS	= 
JPEG_LIBS	= $(call EXPAND_LIBNAME_PATH,mozjpeg,$(DEPTH)/media/libjpeg)
JPEG_REQUIRES	= jpeg
endif

ifdef MOZ_NATIVE_ZLIB
ZLIB_CFLAGS	= 
ZLIB_LIBS	= 
ZLIB_REQUIRES	=
else
ZLIB_CFLAGS	= 
MOZ_ZLIB_LIBS = $(call EXPAND_LIBNAME_PATH,mozz,$(DEPTH)/modules/zlib/src)
ZLIB_REQUIRES	= zlib
endif

ifdef MOZ_NATIVE_BZ2
BZ2_CFLAGS	= 
BZ2_LIBS	= 
BZ2_REQUIRES	=
else
BZ2_CFLAGS	= 
BZ2_LIBS	= $(call EXPAND_LIBNAME_PATH,bz2,$(DEPTH)/modules/libbz2/src)
BZ2_REQUIRES	= libbz2
endif

ifdef MOZ_NATIVE_PNG
PNG_CFLAGS	= 
PNG_LIBS	= 
PNG_REQUIRES	=
else
PNG_CFLAGS	= 
PNG_LIBS	= $(call EXPAND_LIBNAME_PATH,mozpng,$(DEPTH)/media/libpng)
PNG_REQUIRES	= png
endif

QCMS_LIBS	= $(call EXPAND_LIBNAME_PATH,mozqcms,$(DEPTH)/gfx/qcms)

MOZ_HARFBUZZ_LIBS = $(DEPTH)/gfx/harfbuzz/src/$(LIB_PREFIX)mozharfbuzz.$(LIB_SUFFIX)
MOZ_GRAPHITE_LIBS = $(DEPTH)/gfx/graphite2/src/$(LIB_PREFIX)mozgraphite2.$(LIB_SUFFIX)
MOZ_GRAPHITE = 1
MOZ_OTS_LIBS = $(DEPTH)/gfx/ots/src/$(LIB_PREFIX)mozots.$(LIB_SUFFIX)
MOZ_SKIA_LIBS = $(DEPTH)/gfx/skia/$(LIB_PREFIX)skia.$(LIB_SUFFIX)
MOZ_ENABLE_SKIA = 1

MOZ_NATIVE_SQLITE = 
SQLITE_CFLAGS     = 
SQLITE_LIBS       = $(call EXPAND_LIBNAME_PATH,mozsqlite3,$(DIST)/lib)

NSPR_CONFIG	= 
NSPR_CFLAGS = -I/Users/Enda/code/mozilla/mozilla_try/mozilla-central/obj-ff-dbg/dist/include/nspr

NSPR_CFLAGS += -I/Users/Enda/code/mozilla/mozilla_test/mozilla-central/obj-ff-dbg/dist/include/nspr \
-I../obj-ff-dbg/modules/sip/include \
-I../../obj-ff-dbg/modules/sip/include \
-I../../../obj-ff-dbg/modules/sip/include \
-I../../../../obj-ff-dbg/modules/sip/include \
-I../../../../../obj-ff-dbg/modules/sip/include \
-I../../../../../../obj-ff-dbg/modules/sip/include 

NSPR_LIBS = -L/Users/Enda/code/mozilla/mozilla_try/mozilla-central/obj-ff-dbg/dist/lib -lplds4 -lplc4 -lnspr4

NSPR_LIBS += -L../obj-ff-dbg/modules/sip/lib \
-L../../obj-ff-dbg/modules/sip/lib \
-L../../../obj-ff-dbg/modules/sip/lib \
-L../../../../obj-ff-dbg/modules/sip/lib \
-L../../../../../obj-ff-dbg/modules/sip/lib \
-L../../../../../../obj-ff-dbg/modules/sip/lib \
-lsessioncontrolbrowser

NSS_CONFIG	= 
NSS_CFLAGS	= -I$(LIBXUL_DIST)/include/nss
NSS_LIBS	= $(LIBS_DIR) -lcrmf -lsmime3 -lssl3 -lnss3 -lnssutil3
NSS_DEP_LIBS	=         $(LIBXUL_DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX)         $(LIBXUL_DIST)/lib/$(DLL_PREFIX)smime3$(DLL_SUFFIX)         $(LIBXUL_DIST)/lib/$(DLL_PREFIX)ssl3$(DLL_SUFFIX)         $(LIBXUL_DIST)/lib/$(DLL_PREFIX)nss3$(DLL_SUFFIX)         $(LIBXUL_DIST)/lib/$(DLL_PREFIX)nssutil3$(DLL_SUFFIX)
NSS_DISABLE_DBM = 

XPCOM_GLUE_LDOPTS = $(LIBXUL_DIST)/lib/$(LIB_PREFIX)xpcomglue_s.$(LIB_SUFFIX) $(XPCOM_FROZEN_LDOPTS)
XPCOM_STANDALONE_GLUE_LDOPTS = $(LIBXUL_DIST)/lib/$(LIB_PREFIX)xpcomglue.$(LIB_SUFFIX)

USE_DEPENDENT_LIBS = 1

# UNIX98 iconv support
LIBICONV = 

# MKSHLIB_FORCE_ALL is used to force the linker to include all object
# files present in an archive. MKSHLIB_UNFORCE_ALL reverts the linker
# to normal behavior. Makefile's that create shared libraries out of
# archives use these flags to force in all of the .o files in the
# archives into the shared library.
WRAP_LDFLAGS            = 
DSO_CFLAGS              = 
DSO_PIC_CFLAGS          = -fPIC
MKSHLIB                 = $(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@
MKCSHLIB                = $(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@
MKSHLIB_FORCE_ALL       = 
MKSHLIB_UNFORCE_ALL     = 
DSO_LDOPTS              = 
DLL_SUFFIX              = .dylib

NO_LD_ARCHIVE_FLAGS     = 1

GTK_CONFIG	= 
QT_CONFIG	= @QT_CONFIG@
TK_CFLAGS	= -DNO_X11
TK_LIBS		= -framework QuartzCore -framework Carbon -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework AddressBook -framework OpenGL

MOZ_TOOLKIT_REGISTRY_CFLAGS = \
	$(TK_CFLAGS)

CAIRO_FT_CFLAGS		= 

MOZ_TREE_FREETYPE		= 
MOZ_ENABLE_CAIRO_FT	= 
MOZ_ENABLE_GTK2		= 
MOZ_ENABLE_QT		= 
MOZ_ENABLE_XREMOTE	= 
MOZ_ENABLE_DWRITE_FONT	= 
MOZ_ENABLE_D2D_SURFACE	= 
MOZ_ENABLE_D3D9_LAYER	= 
MOZ_ENABLE_D3D10_LAYER  = 

MOZ_GTK2_CFLAGS		= 
MOZ_GTK2_LIBS		= 

MOZ_QT_CFLAGS		= 
MOZ_QT_LIBS		= 
MOZ_ENABLE_QTNETWORK    = 
MOZ_ENABLE_QMSYSTEM2    = 
MOZ_ENABLE_QTMOBILITY   = 
MOZ_ENABLE_CONTENTACTION   = 
MOZ_ENABLE_MEEGOTOUCHSHARE = 
MOZ_ENABLE_CONTENTMANAGER = 

MOZ_DBUS_CFLAGS         = 
MOZ_DBUS_LIBS           = 
MOZ_DBUS_GLIB_CFLAGS    = 
MOZ_DBUS_GLIB_LIBS      = 
MOZ_ENABLE_DBUS         = 

MOZ_GTHREAD_CFLAGS      = 
MOZ_GTHREAD_LIBS        = 

FT2_CFLAGS             = 
FT2_LIBS               = 

MOZ_PANGO_CFLAGS        = 
MOZ_PANGO_LIBS          = 

XT_LIBS			= 

MOZ_LIBPROXY_CFLAGS     = 
MOZ_LIBPROXY_LIBS       = 
MOZ_ENABLE_LIBPROXY     = 

MOZ_LIBNOTIFY_CFLAGS	= 
MOZ_LIBNOTIFY_LIBS	= 
MOZ_ENABLE_LIBNOTIFY	= 

MOZ_ALSA_LIBS           = 

GLIB_CFLAGS	= 
GLIB_LIBS	= 
GLIB_GMODULE_LIBS	= 

MOZ_NATIVE_MAKEDEPEND	= /usr/X11/bin/makedepend

export CL_INCLUDES_PREFIX = 

MOZ_AUTO_DEPS	= 1
COMPILER_DEPEND = 1
MDDEPDIR        := .deps
CC_WRAPPER = 
CXX_WRAPPER = 

MOZ_DEMANGLE_SYMBOLS = 1

OS_TARGET=Darwin
OS_ARCH=Darwin
OS_RELEASE=
OS_TEST=i386
CPU_ARCH=x86
INTEL_ARCHITECTURE=1

# For Solaris build
SOLARIS_SUNPRO_CC = 
SOLARIS_SUNPRO_CXX = 

# For AIX build
AIX_OBJMODEL = 

# For OS/2 build
MOZ_OS2_TOOLS = 
MOZ_OS2_HIGH_MEMORY = 1

MOZ_PSM=1

# Gssapi (krb5) libraries and headers for the Negotiate auth method
GSSAPI_INCLUDES = @GSSAPI_INCLUDES@
USE_GSSAPI	= @USE_GSSAPI@

MOZILLA_OFFICIAL = 

# Win32 options
MOZ_BROWSE_INFO	= 
MOZ_TOOLS_DIR	= 
MOZ_QUANTIFY	= 
MSMANIFEST_TOOL = 
WIN32_REDIST_DIR = 
MOZ_GLUE_LDFLAGS = $(call EXPAND_LIBNAME_PATH,mozglue,$(LIBXUL_DIST)/lib)
MOZ_GLUE_PROGRAM_LDFLAGS = 
WIN32_CRT_LIBS = 

# This is used to pass jemalloc flags to NSS
DLLFLAGS = 

# Codesighs tools option, enables win32 mapfiles.
MOZ_MAPINFO	= 

MOZ_PHOENIX	= 1
MOZ_XULRUNNER	= 

MOZ_DISTRIBUTION_ID = org.mozilla

MOZ_PLATFORM_MAEMO = 
MOZ_PLATFORM_MAEMO_CFLAGS	= 
MOZ_PLATFORM_MAEMO_LIBS 	= 
MOZ_MAEMO_LIBLOCATION 	= 

MOZ_ENABLE_LIBCONIC = 
LIBCONIC_CFLAGS     = 
LIBCONIC_LIBS       = 

MACOS_SDK_DIR	= 
NEXT_ROOT	= 
GCC_VERSION	= 4.2
UNIVERSAL_BINARY= 
MOZ_CAN_RUN_PROGRAMS = 
HAVE_DTRACE= 

VISIBILITY_FLAGS = -fvisibility=hidden
WRAP_SYSTEM_INCLUDES = 

HAVE_ARM_SIMD = 
HAVE_ARM_NEON = 
HAVE_GCC_ALIGN_ARG_POINTER = 1

MOZ_THEME_FASTSTRIPE = 

MOZ_SERVICES_SYNC = 1

MOZ_OFFICIAL_BRANDING = 

HAVE_CLOCK_MONOTONIC = 
REALTIME_LIBS = 

MOZ_APP_COMPONENT_LIBS = 
MOZ_APP_EXTRA_LIBS = 

ANDROID_NDK       = 
ANDROID_TOOLCHAIN = 
ANDROID_PLATFORM  = 
ANDROID_SDK       = 
ANDROID_PLATFORM_TOOLS = 
ANDROID_VERSION   = 

ANDROID_PACKAGE_NAME = 

JS_SHARED_LIBRARY = 

MOZ_INSTRUMENT_EVENT_LOOP = 1

# We only want to do the pymake sanity on Windows, other os's can cope
ifeq ($(HOST_OS_ARCH),WINNT)
# Ensure invariants between GNU Make and pymake
# Checked here since we want the sane error in a file that
# actually can be found regardless of path-style.
ifeq (_:,$(.PYMAKE)_$(findstring :,$(srcdir)))
$(error Windows-style srcdir being used with GNU make. Did you mean to run $(topsrcdir)/build/pymake/make.py instead? [see-also: https://developer.mozilla.org/en/Gmake_vs._Pymake])
endif
ifeq (1_a,$(.PYMAKE)_$(firstword a$(subst /, ,$(srcdir))))
$(error MSYS-style srcdir being used with Pymake. Did you mean to run GNU Make instead? [see-also: https://developer.mozilla.org/en/Gmake_vs._Pymake])
endif
endif # WINNT
