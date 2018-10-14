#!/usr/bin/env bash

set -eu

cd build && ninja && LD_LIBRARY_PATH=. $@
