map c :w:!sh debug_valgrind.sh cpu pp_ && cat *cpu.log
map m :w:!sh debug_valgrind.sh mem 
map g :w:!gdb -x debug.gdb
map d :w:!sh debug.sh
map v :w:so %:exec 'e' _pp()
map p :w:!make -C ./misc clean px_test2 MISC_DEBUG='-DDEBUG' && gdb -x ./misc/debug_px_test.gdb
set ai
set aw
