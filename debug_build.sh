#!/bin/sh
set -eux
export CFLAGS="${CFLAGS:-}"' -std=c99 -g -DDEBUG -DDEBUG_OUT='$(pwd)'/debug.log'
./configure
make -e clean all
