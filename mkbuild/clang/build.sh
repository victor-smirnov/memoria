#!/bin/sh

print_usage()
{
    SCRIPT_NAME="build.sh"
    echo "USAGE: $SCRIPT_NAME [--raw]"
}

if [ $# -eq 0 ]; then
    RAW=0
elif [ $# -eq 1 ]; then
    case $1 in
        --raw) RAW=1 ;;
        *)
            print_usage
            exit 1
            ;;
    esac
else
    print_usage
    exit 1
fi

if [ $RAW -ne 1 ]; then
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
    echo "Build log file: $BUILD_LOG_FILE"
else
    make
fi
