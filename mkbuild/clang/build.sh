#!/bin/sh

BUILD_TIME_FILE="build_time.txt"
BUILD_LOG_FILE="build_log.txt"

echo "Build started: `date`" > $BUILD_TIME_FILE

time -f "%E" 2>&1 make 2>$BUILD_LOG_FILE
duration=`tail -n 1 $BUILD_LOG_FILE`

head -n -1 $BUILD_LOG_FILE > "$BUILD_LOG_FILE"_
rm $BUILD_LOG_FILE
mv "$BUILD_LOG_FILE"_ $BUILD_LOG_FILE

echo "Build finished: `date`" >> $BUILD_TIME_FILE
echo "Duration: $duration" >> $BUILD_TIME_FILE

echo "Build duration: $duration"
