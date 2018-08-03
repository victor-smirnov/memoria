#!/bin/bash

set -e

echo "Configuring basic system..."


dnf install -y sudo mc clang gcc cmake boost-devel libicu-devel


echo "Build environment configuration finished..."

# Basic build environment system is done