#!/bin/sh

build_time_file="build_time.txt"
build_log_file="build_log.txt"

echo "Build started: `date`" > $build_time_file

time -f "%E" 2>&1 make 2>$build_log_file
duration=`tail -n 1 $build_log_file`

head -n -1 $build_log_file > "$build_log_file"_
rm $build_log_file
mv "$build_log_file"_ $build_log_file

echo "Build finished: `date`" >> $build_time_file
echo "Duration: $duration" >> $build_time_file

echo "Build duration: $duration"
