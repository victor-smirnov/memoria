#!/bin/bash

set -e

docker volume create vcpkg-cache
docker volume create vcpkg-repo
docker volume create memoria-build

docker run -v vcpkg-repo:/vcpkg -it memoria-build:latest /usr/bin/git clone https://github.com/microsoft/vcpkg.git /vcpkg
