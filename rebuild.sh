#!/bin/bash

set -ex

rm -rf build

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$(find . -name "vcpkg.cmake")

cmake --build build
