#!/bin/bash
# This script extracts all of the .o files from all of the libraries we want to link
# and then renames them, to avoid name clashes between .o files in different libraries.
# Then all the .o files from all of the libraries are put back into libcallControlComplete.a
# This whole process was done for us by libtool on the mac, but linux did not support the
# same behaviour so we had to write this script
rm -f lib$1.a
cd tmp_linux_libs
for i in *.a
do
	dirname=$i"-dir"
	mkdir $dirname
	cd $dirname
	ar x ../$i
	for ofile in *.o
	do
		mv $ofile $i"-"$ofile
	done
	cd ..
	echo ar rc ../lib$1.a $dirname/*.o
	ar rc ../lib$1.a $dirname/*.o
done
	

