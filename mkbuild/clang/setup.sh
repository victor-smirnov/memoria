#!/bin/sh

BASE_DIR=$(dirname $0)
BUILD_DIR=$BASE_DIR/../../../memoria-build/clang
COMMON_DIR=$BASE_DIR/../_common

mkdir -p $BUILD_DIR

cp -i $COMMON_DIR/build.sh.tpl	$BUILD_DIR/build.sh
cp -i $COMMON_DIR/clean.sh.tpl	$BUILD_DIR/clean.sh
cp -i $BASE_DIR/mkbuild.sh		$BUILD_DIR
