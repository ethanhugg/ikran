import os
import sys

# Take in build flags
# runscons is not defaulted to anything valid to force called to pass in arguments
runscons    = ARGUMENTS.get('runscons', 'xxx')
debug       = ARGUMENTS.get('debug', 1)
x64         = ARGUMENTS.get('x64', 'no')
noaddon     = ARGUMENTS.get('noaddon', 'no')

if runscons == 'xxx':
  print 'Do not call scons directly. Use runSconsBuild.py'
  exit(1)

suffixName = ""
componentName = 'enhanced-callcontrol'

# Environment variables
if 'MOZSRCPATH' in os.environ:
  mozsrcpath = os.environ['MOZSRCPATH'].rstrip('/').rstrip('\\')
  print 'Using MOZSRCPATH ' + mozsrcpath
else:
  print 'Environment variable MOZSRCPATH must be set to your mozilla-central'
  exit(1)

if 'WEBRTCPATH' in os.environ:
  webrtcpath = os.environ['WEBRTCPATH'].rstrip('/').rstrip('\\')
  print 'Using WEBRTCPATH ' + webrtcpath
else:
  print 'Environment variable WEBRTCPATH must be set to webrtc trunk path'
  exit(1)
 
if sys.platform =='win32':
  if 'MS_VC_PATH' in os.environ: 
    ms_vc_path=os.environ['MS_VC_PATH']
  else:
    print 'Environment variable MS_VC_PATH must point to your Visual Studio install'
    exit(1)
    
  if 'MS_WINDOWS_SDK_PATH' in os.environ:
    ms_windows_sdk_path=os.environ['MS_WINDOWS_SDK_PATH']
  else:
    print 'Environment variable MS_WINDOWS_SDK_PATH must point to your Windows Platform SDK install'
    exit(1)
    
  cl_path = ms_vc_path + '/VC/bin'
  rc_path = ms_windows_sdk_path + '/bin'
  common_path = ms_vc_path + '/Common7/IDE'
  vs_path = cl_path + ';' + common_path + ';' + rc_path
  ms_vc_include_path = ms_vc_path + '/VC/ATLMFC/INCLUDE' + ';' + ms_vc_path + '/VC/INCLUDE' ';' + ms_windows_sdk_path + '/Include'
elif sys.platform=='darwin':
  gcc_include_path = ' '
elif sys.platform=='linux2':
  gcc_include_path = ' '  

build_env = Environment()

build_env["CPPDEFINES"] = [
  'LOG4CXX_STATIC', 
  '_NO_LOG4CXX', 
  'USE_SSLEAY', 
  'LIBXML_STATIC', 
  'FORCE_PR_LOG',
]

if sys.platform =='win32':
  build_env["ENV"] = {'PATH' : vs_path, 'INCLUDE' : ms_vc_include_path}              
  build_env["MSVS_VERSION"] = '9.0'
  build_env["CPPDEFINES"] += [
    'SIP_OS_WINDOWS', 
    'WIN32',
    'GIPS_VER=3480',
    'NOMINMAX',
    'UNICODE',
    '_UNICODE',
    '_DEBUG',
  ]

  build_env["CPPFLAGS"] = [
    '/EHsc', 
    '/wd4996', # suppress warning for strncpy and the like
  ]
  
  build_env["LINKFLAGS"] = '/MACHINE:X86 '
elif sys.platform=='darwin':
  build_env["CPPFLAGS"] = [
    '-Werror',
    '-Wunused-function',
    '-fexceptions',
    '-fno-common',
  ]
  
  if x64 == 'yes':
    build_env["CPPFLAGS"] += [
      '-arch',
      'x86_64'  
    ]
  else:
    build_env["CPPFLAGS"] += [
      '-arch',
      'i386'  
    ]    
    
  build_env["CPPDEFINES"] += [
    'SIP_OS_OSX', 
    'OSX', 
    'GIPS_VER=3410', 
    'FORCE_PR_LOG',
    '_FORTIFY_SOURCE=2'
  ]
  
  build_env["AR"] = 'libtool'
  
  build_env["ARFLAGS"] = [
    '-static',
    '-o'
  ]
  
  build_env["LINKFLAGS"] = [
    '-isysroot',
    '/Developer/SDKs/MacOSX10.6.sdk',
    '-mmacosx-version-min=10.5',
    '-L/usr/X11R6/lib',
    '-framework',
    'Cocoa',
    '-framework',
    'AGL',
    '-framework',
    'OpenGL',
    '-framework',
    'Carbon',
    '-framework',
    'QtKit',
  ]

  if x64 == 'yes':
    build_env["LINKFLAGS"] += [
      '-arch',
      'x86_64'  
    ]
  else:
    build_env["LINKFLAGS"] += [
      '-arch',
      'i386'  
    ]    
    
  
elif sys.platform=='linux2':
  build_env["CPPFLAGS"] = [
    '-Werror',
    '-Wall',
    '-fexceptions',
    '-fno-common',
    '-fPIC',
  ]
  if x64 == 'yes':
    build_env["CPPFLAGS"] += [
      '-m64',
    ]
  else:
    build_env["CPPFLAGS"] += [
      '-m32',
      '-march=i486'
    ] 
  build_env["CPPDEFINES"] += [
    '_GNU_SOURCE', 
    'SIP_OS_LINUX', 
    'LINUX', 
    'GIPS_VER=3510', 
    'FORCE_PR_LOG',
    'SECLIB_OPENSSL'
  ]
  build_env["LINKFLAGS"] = [
  ]
  if x64 == 'yes':
    build_env["LINKFLAGS"] += [
      '-m64'
    ]
  else:  
    build_env["LINKFLAGS"] += [
      '-m32',
      '-march=i486'
    ]
  
# currently we optimize for speed - perhaps we should change to reduce size - Options are -Os or even Oz for gcc and O1 for VC


if int(debug):
  if sys.platform =='win32':
    build_env["CPPFLAGS"] += [
      '/Od',
      '/RTC1',
      '/Fd"enhanced-callcontrol_slib_MDd.pdb"',
      '/Gm',
      '/ZI'
    ]
    
    build_env["CPPFLAGS"] += [
      '/MTd'
    ]
    suffixName = "_MTd"

  elif sys.platform=='darwin':
    build_env["CPPFLAGS"] += [
      '-O0',
      '-g',
      '-ggdb',
      '-isysroot',
      '/Developer/SDKs/MacOSX10.6.sdk',
      '-mmacosx-version-min=10.5'
  ]
  elif sys.platform=='linux2':
    build_env["CPPFLAGS"] += [ 
      '-O0',
      '-g',
      '-ggdb'
    ]

else:
  if sys.platform =='win32':
    # /Zi will add debug info, but only in the PDB file
    build_env["CPPFLAGS"] += [
      '/O2',
      '/Gy',
      '/Zi'
    ]
    build_env["CPPFLAGS"] += [
      '/MT'
    ]
    suffixName = "_MT"
  elif sys.platform=='darwin':
    build_env["CPPFLAGS"] += [
      '-fast',
      '-D',
      'NDEBUG',
      '-isysroot',
      '/Developer/SDKs/MacOSX10.6.sdk',
      '-mmacosx-version-min=10.5'
    ]
  elif sys.platform=='linux2':
    build_env["CPPFLAGS"] += [
      '-D',
      'NDEBUG'
    ]

componentName = 'enhanced-callcontrol_slib'
  
if int(debug):
  print "Building with debug"
print "x64 is set to " + x64
if noaddon == 'yes':
  print "Not building browser addon"
else:
  print "Building browser addon"
  
chromiumbaseincludepath = 'third_party'
chromiumbaselibpath = 'third_party'
#chromiumbaseincludepath = mozsrcpath + '/ipc/chromium/src'
#chromiumbaselibpath = mozsrcpath + '/ipc/chromium/src'

Export('build_env')
Export('debug')
Export('x64')
Export('mozsrcpath')
Export('webrtcpath')
Export('chromiumbaseincludepath')
Export('chromiumbaselibpath')
Export('suffixName')
Export('componentName')

SCRIPT_FILES = [ 

  'src/common/browser_logging/SConstruct',

  'src/SConstruct',
    
  'src/sipcc/SConstruct',
  
  'third_party/chromium_base/SConstruct',

  # Pull it into one big static lib for ease of consumption
  'Sconscriptlib'
]

# Unit tests and testapp builds are all static.
SCRIPT_FILES += [ 
  'tests/testapp_softphone/SConstruct',
]

if sys.platform != 'win32':
  SCRIPT_FILES += [
    'tests/roap/SConstruct'
  ]

if noaddon != 'yes':
  SCRIPT_FILES += [ 
    'ikran/SConstruct',
#    'ikran/SConstructBrowser',
    'ikran/SConstructBrowserStatic'
  ]

SConscript(SCRIPT_FILES)
