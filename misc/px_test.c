#include <stdlib.h>
#include "../px.h"

#define CMD_NOOP 0
#define CMD_QUIT 1

static ui32 count_redraw = 0;
static ui32 count_key = 0;

void redraw(void);
void loop(void);

void redraw() {
	ui16 y;
	count_redraw++;
	px_cy = px_out_h - 1; px_cx = 1;
	px_rect(1, px_out_w - 2, '-');
	px_cy = 0; px_cx = 1;
	px_rect(1, px_out_w - 2, '-');
	px_cy = 1; px_cx = px_out_w -1;
	px_rect(px_out_h - 2, 1, '|');
	px_cy = 1; px_cx = 0;
	px_rect(px_out_h - 2, 1, '|');
	px_cy = 5;
	px_cx = 5;
	px_rect(16, 38, '.');
	y = 7;
	px_cx = 7;
#define TD(X) px_cx = 7; px_cy = y++; px_printf(# X" = %d", X);
	TD(px_out_h);
	TD(px_out_w);
	TD(count_redraw);
	TD(count_key);
#ifdef PX_STATS
	TD(px_stats.out_bytes_last);
	TD(px_stats.out_bytes_max);
	TD(px_stats.out_bytes_total);
	TD(px_stats.in_bytes_last);
	TD(px_stats.in_bytes_max);
	TD(px_stats.in_bytes_total);
#endif /* PX_STATS */
	px_flush();
}

void loop() {
	ui8 key;
	while (1) {
		redraw();
		key = px_get_key();
		count_key++;
		switch (key) {
		case 3:
			exit(0);
			break;
		case 12:
			px_clear();
			break;
		default:
			break;
		}
	}
}

int main(int argc, char **argv) { 
	px_on_resize = &redraw;
	px_init();
	atexit(px_cleanup);
	loop();
	return 0;
}
