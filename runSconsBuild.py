import sys, subprocess, os, shutil, commands
from subprocess import Popen,PIPE,STDOUT

build = 'debug'
gen_addon = 'no'


# Default the return code to good. If any unexpected warnings appear or the sonns build had errors then
# the return value will change to bad
returnCode = 0


# This function will be used to find a specific string with in a file. The lines do not have to 
# completely match - if the chckLine in file forms part of the line argument then the line will not  be
# printed otherwise it is printed. This means that a warning can be passed in and if no substring of that
# warninging exists in file then it will be printed (i.e. it is an unexpected warning).
def printIfNotFound(line, file):
  global returnCode
  searchFile = open(file)
  for checkLine in searchFile:
    # get rid of leading or trailing whitespace that might confuse the comparison
    checkLine = checkLine.strip()
    line = line.strip()
    # change all \ to / in paths since different parts of the build use different seperators
    # and we want "boost/strings" to match "boost\strings
    checkLine = checkLine.replace("\\", "/")
    line = line.replace("\\", "/")
    # make it all lower case, since directory/filenames can be expressed in either case
    checkLine = checkLine.lower()
    line = line.lower()
    if checkLine != "":
      if checkLine in line:
        return
  print line
  # A line was printed so there is at least one unexpected warning so indicate fail return value
  returnCode = 1
  return

# Need to check the warning file since a line with, for example, "d" would strip
# every waring ocntaining the letter 'd' so we make sure every line contains ": warn"
# to be sure it is an actual warning.
# On VC++ warnings will contain ": warning". On mac they may contain ranlib or libtool.
def checkValidWarningFile(file):
  global returnCode
  warnFile = open(file)
  for checkLine in warnFile:
    # get rid of leading or trailing whitespace that might confuse the comparison
    checkLine = checkLine.strip()    
    if ": warn" not in checkLine and "ranlib: file:" not in checkLine and "libtool: file:" not in checkLine and checkLine != "":
      if not checkLine.startswith('#'):
        print "ERROR: AllowedWarnings contains a line that is not a warning: "
        print checkLine
        returnCode = 1

# Find every warning in a file and then try to print it. The print will not happen if a
# substring of the warning exists in 
def warningFilter(sconsLog, filterFile):
  global returnCode
  sconsPassFound = None
  checkValidWarningFile(filterFile)
  cOut = open(sconsLog)
  for cLine in cOut.readlines():
    if ": warning" in cLine or "ranlib: file:" in cLine or "libtool: file:" in cLine:
      printIfNotFound(cLine, filterFile)
    # also want to print if the overall scons build has any errors
    if "scons: done building targets (errors occurred during build)." in cLine:
      print "Build also had errors. Build Failed"
      returnCode = 1
    if "scons: done building targets." in cLine:
      sconsPassFound = 1
  cOut.close()
  if sconsPassFound==None:
    print "Fail because scons never ran to completion "
    returnCode = 1

# Find every warning in a file and then try to print it. The print will not happen if a
# substring of the warning exists in 
def checkErrors(sconsLog):
  global returnCode
  sconsPassFound = None
  cOut = open(sconsLog)
  for cLine in cOut.readlines():
    # want to print if the overall scons build has any errors
    if "scons: done building targets (errors occurred during build)." in cLine:
      print "Build also had errors. Build Failed"
      returnCode = 1
    if "scons: done building targets." in cLine:
      sconsPassFound = 1
  cOut.close()
  if sconsPassFound==None:
    print "Fail because scons never ran to completion "
    returnCode = 1
#
# Start of Main
#
# remove an old log if there was one from a previous build
logFile = "sconsbuild.log"
if os.path.exists(logFile):
  os.remove(logFile)
  
sconsLoc = os.environ.get("SCONS_LOCATION")
if sys.platform == "win32":
  if sconsLoc is None:
    sconsLoc = "C:\Python27\Scripts"
  sconsProg = sconsLoc + "\scons.bat"
elif sys.platform == "darwin":
  if sconsLoc is None:
    sconsLoc = "/usr/local/bin"
  sconsProg = sconsLoc + "/scons"
elif sys.platform == "linux2":
  if sconsLoc is None:
    sconsLoc = "/usr/bin"
  sconsProg = sconsLoc + "/scons"

if sys.platform == "win32":
  zipLoc = os.environ.get("ZIP_LOCATION")
  print zipLoc
  if zipLoc is None:
    print "7-ZIP is either uninstalled or environment variable ZIP_LOCATION not set !"
    exit(1)

buildType = '-k'
isDebug = 1

buildArgs = ['runscons=yes']

for arg in sys.argv:
  print "runSconsBuild.py using arg: " + arg
  if (arg == 'release' ):
    buildArgs += ['debug=0']
    isDebug = 0
    
  if (arg == 'x64'):
    buildArgs += ['x64=yes']

  if (arg == 'noaddon'):
    buildArgs += ['noaddon=yes']
    gen_addon = 'no'
  else:
    gen_addon = 'yes'  

  if (arg == 'clean'):
    shutil.rmtree("out", True)
    buildType = '-c'
  

# Now run scons in a subprocess. We want to print the scons output to the console as well as record in a log
# file which can be searched for warnings afterwards. 
f = open(logFile,'w')
  
  
p = Popen([sconsProg, buildType] + buildArgs, stdout=PIPE, stderr=STDOUT)

while True:
  o = p.stdout.readline()
  if o == '' and p.poll() != None: break
  # send the build output to the console as well as a log file
  f.write(o)
  o = o.rstrip()
  if o != '':
    print o
  
f.close()

if (buildType == '-c'):
  sys.exit(returnCode)

# All compile is done now so stop recording log and then print the unfiltered warnings
# print ""
# print "Warning Summary: All these warnings are unexpected "
# print ""
# warningFilter(logFile, "AllowedWarnings.txt")
# print "End of Warning Summmary"

if(gen_addon == 'yes'):
  print 'building Addon for ' + sys.platform
  if sys.platform == 'darwin':
    xpt_dylib = "libsessioncontrol.dylib "
  elif sys.platform == 'linux2':
    xpt_dylib = "libsessioncontrol.so "
  elif sys.platform == 'win32':
    xpt_dylib = "libsessioncontrol.dll "
  xpi_xpt = "ICallControl.xpt "
  xpi_content = "content/ "
  xpi_installer = "install.rdf "
  xpi_manifest = "chrome.manifest "
  xpi_output = "ikran-0.2-dev.xpi "
  
  if sys.platform == 'win32':
    zipcmd = '"'+zipLoc + "/7z.exe "+'"'
    cmd = r"cd ikran & " + zipcmd + " a " + xpi_output + xpi_manifest + xpi_installer + xpi_content + xpt_dylib + xpi_xpt
    proc = subprocess.Popen(cmd ,shell=True)
  else:
    cmd = "cd ikran ; zip -9r "
    proc = subprocess.Popen(cmd + xpi_output + xpi_manifest + xpi_installer + xpi_content + xpt_dylib + xpi_xpt,shell=True)
  
  proc.wait()

sys.exit(returnCode)

