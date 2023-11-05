#!/bin/bash

set -e
docker run \
    --mount type=bind,source=$(pwd)/build-files,target=/build \
    -it memoria-build:latest /bin/bash