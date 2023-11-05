#!/bin/bash

docker run \
    -v vcpkh-cache:/root/.cache/vcpkg \
    -v vcpkg-repo:/vcpkg \
    -it memoria-build:latest /bin/bash -c cd /vcpkg && /usr/bin/git pull