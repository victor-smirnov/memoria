#!/bin/bash

set -e

echo "Configuring basic system..."


dnf install -y git sudo mc clang gcc cmake boost-devel libicu-devel make


echo "Build environment configuration finished..."

# Basic build environment system is done