#include <stdlib.h>
#include "../px.h"

#define _CMD_NOOP 0
#define _CMD_EXIT 1
#define _CMD_TOGGLE_BLOCK 2

typedef struct {
	ui8 last_key;
	ui32 no_key;
} _draw_t;

static _draw_t *_drawp = 0;

void _do_cmd(ui8);
ui8 _get_cmd(ui8);
void _redraw(void);
void _in_loop(void);

void _do_cmd(ui8 cmd) {
	if (cmd == _CMD_EXIT) exit(0);
	if (cmd == _CMD_TOGGLE_BLOCK) px_block = !px_block;
}

ui8 _get_cmd(ui8 key) {
	if (key == 3) {
		return _CMD_EXIT;
	} else if (key == 'b') {
		return _CMD_TOGGLE_BLOCK;
	}
	return _CMD_NOOP;
}

void _redraw() {
	px_cx = 2;
	px_cy = 2;
	px_printf("last_key = %d ", _drawp->last_key & 0xFF);
	px_cy++;
	px_printf("no_key = %d ", _drawp->no_key);
	px_stats_print();
	px_flush();
}

void _loop() {
	_draw_t draw;
	ui8 key;
	ui8 cmd;
	draw.last_key = 0;
	draw.no_key = 0;
	_drawp = &draw;
	key = 0;
	cmd = 0;
	_redraw();
	while (1) {
		key = px_get_key();
		if (key != 0) {
			cmd = _get_cmd(key);
			_do_cmd(cmd);	
			_redraw();
			draw.last_key = key;
		} else {
			draw.no_key++;
		}
	}
}

int main(int argc, char **argv) {
	px_on_resize = _redraw;
	px_init();
	atexit(px_cleanup);
	_loop();
	return 0;
}
