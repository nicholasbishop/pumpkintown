#!/usr/bin/env bash

set -eu

ninja -C build && LD_LIBRARY_PATH=build/gl $@
