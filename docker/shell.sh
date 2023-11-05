#!/bin/bash

set -e
parentdir="$(dirname $(pwd))"
docker run \
    --mount type=bind,source=${parentdir},target=/sources \
    -v memoria-build:/build \
    -v vcpkh-cache:/root/.cache/vcpkg \
    -v vcpkg-repo:/vcpkg \
    -it memoria-build:latest /bin/bash