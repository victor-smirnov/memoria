#!/bin/sh

current_dir=`pwd`


for folder in `find . -type d` ; do
cd $folder

echo `pwd`

    for file in  *.[ch]pp ; do 
	if [ -a $file ]; then
	    #echo $file
	    expand -t 4 $file >$file.exp
	    mv $file.exp $file
	fi
    done

cd $current_dir

done