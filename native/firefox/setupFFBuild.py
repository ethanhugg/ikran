import os, sys, re

if sys.platform != 'darwin':
    print "Currently only works on mac"
    sys.exit(1)

if (len(sys.argv) != 2):
    print "please use path to root of mozilla source tree as parameter"

#print sys.argv[1]

modulesArea = os.path.abspath(sys.argv[1]  + "/modules")

print modulesArea
