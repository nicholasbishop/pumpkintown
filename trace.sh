#!/usr/bin/env bash

set -eux

ninja -C build && LD_LIBRARY_PATH=${PWD}/build/gl LD_PRELOAD=${PWD}/build/gl/libGL.so.1 $@
