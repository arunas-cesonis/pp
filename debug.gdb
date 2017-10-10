shell CFLAGS='-std=c99 -DDEBUG_NO_EXEC' ./debug_build.sh
shell rm -rf ~/.cache/pp
shell rm -rf debug.log
file ./pp
set args -2
# break pp_str.c:73
# break pp_main.c:289
# rbreak exec_prep
# break pp_main.c:295
# break pp_main.c:367
# break pp_tree.c:125
# break main
cd /
run
