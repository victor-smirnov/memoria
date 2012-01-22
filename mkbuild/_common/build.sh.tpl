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
    
    BASE_DIR=$(dirname $0)

    echo "Build started: `date`" > $BASE_DIR/$BUILD_TIME_FILE

    time -f "%E" 2>&1 make --no-print-directory -C $BASE_DIR 2>$BASE_DIR/$BUILD_LOG_FILE
    duration=`tail -n 1 $BASE_DIR/$BUILD_LOG_FILE`

    head -n -1 $BASE_DIR/$BUILD_LOG_FILE > "$BASE_DIR/$BUILD_LOG_FILE"_
    rm $BASE_DIR/$BUILD_LOG_FILE
    mv "$BASE_DIR/$BUILD_LOG_FILE"_ $BASE_DIR/$BUILD_LOG_FILE

    echo "Build finished: `date`" >> $BASE_DIR/$BUILD_TIME_FILE
    echo "Duration: $duration" >> $BASE_DIR/$BUILD_TIME_FILE

    echo "Build duration: $duration"
    echo "Build log file: $BUILD_LOG_FILE"
else
    make --no-print-directory -C $BASE_DIR
fi
