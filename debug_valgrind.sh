#!/bin/sh
set -eu

_mem () {
	rm -rf ~/.cache/pp
	rm -rf debug.log
	cd ~/dev
	valgrind \
		--tool=memcheck \
		--leak-check=yes \
		--show-reachable=yes \
		--num-callers=20 \
		--gen-suppressions=all \
		--suppressions=$PP/ncurses.supp \
		--track-fds=yes \
		--log-file=$LOG \
		$BIN -2

}

_cpu () {
	local FILTER=cat
	PP=${PP:-.}
	if [ $# -eq 1 ]; then
		FILTER="grep $1"
	fi

	rm -rf ~/.cache/pp
	cd ~/dev/uClibc

	valgrind \
		--tool=callgrind \
		--callgrind-out-file=$PP/callgrind.out \
		$BIN -2
	exec 3>&1
	exec >$LOG
	echo INCLUSIVE
	callgrind_annotate --inclusive=yes $PP/callgrind.out | $FILTER || true
	echo EXCLUSIVE
	callgrind_annotate $PP/callgrind.out | $FILTER || true
	exec >&3
}

_main () {
	cd $(dirname $0)
	PP=$(pwd)
	# export CFLAGS_USER='-DDEBUG_AUTO=5 -DDEBUG_LINE=zellner.vim'
	# export CFLAGS_USER='-DDEBUG_LINE=zellner.vim'
	# export CFLAGS_USER='-DDEBUG_AUTO=CMD_EXEC'
	export CFLAGS_USER='-DDEBUG_AUTO=CMD_EXEC'
	sh ./debug_build.sh
	# make -e clean all
	LOG=${LOG:-$PP/valgrind_$CMD.log}
	BIN=${BIN:-$PP/pp}
	_$CMD $@
}

CMD=
if [ $# -gt 0 ]; then
	CMD=$1
	shift
	_main $@
fi

