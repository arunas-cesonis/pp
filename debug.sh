#!/bin/sh
set -eu
cd $(dirname $0)
PP=$(pwd)
CFLAGS='-DDEBUG_NO_EXEC' ./debug_build.sh
rm -rf ~/.cache/pp
# cd /usr/dev/poker-ui-app
$PP/pp -2
