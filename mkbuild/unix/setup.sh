#!/bin/sh

BASE_DIR=$(dirname $0)
BUILD_DIR=$BASE_DIR/../../../memoria-build/unix

mkdir -p $BUILD_DIR

cp -i $BASE_DIR/build.sh		$BUILD_DIR
cp -i $BASE_DIR/clean.sh		$BUILD_DIR
cp -i $BASE_DIR/mkbuild.sh		$BUILD_DIR
