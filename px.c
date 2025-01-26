#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#define PX_EXTERN
#include "px.h"

#ifdef DEBUG
#include <stdio.h>
#define PX_LOG(...) fprintf(stderr, __VA_ARGS__);
#else /* DEBUG */
#define PX_LOG(...)
#endif /* DEBUG */

#ifdef PX_STATS
#define PX_DOT() .
#define _PX_P(X) X
#define PX_P(X) _PX_P(X)
#define _PX_CAT(X,Y) X ## Y
#define PX_CAT(X,Y) _PX_CAT(X,Y)
#define PX_STAT(T,N) PX_P(px_stats)PX_DOT()PX_CAT(T,N)
#define PX_CTR(T,C)\
	PX_STAT(T,_last) = C;\
	if (PX_STAT(T,_last) > PX_STAT(T,_max))\
		 PX_STAT(T,_max) = PX_STAT(T,_last);\
	PX_STAT(T,_total) += PX_STAT(T,_last);
#else /* PX_STATS */
#define PX_CTR(T,C) C;
#endif /* PX_STATS */

void px_sigwinch(int sig);
char *px_wint(char*, int, int);
void px_winsz(void);
void px_out_resize(ui16 h, ui16 w);
void px_read_in(void);

#define PP_STDIN 0
#define PP_STDOUT 1
#define PP_OUT_BUF_MULT 2
#define PP_CUT_BUF_SZ 8
#define TSAVESCR "\0337\033[?47h"
#define TRESTSCR "\033[?47l\0338"

static struct termios torig;
static struct winsize winsz;
static ui8p in_buf[256];
static ui8p *in_buf_end;
static ui8p *in_buf_p;
static char *out0 = 0;
static char *out = 0;
static char *out_end = 0;
static size_t out_sz = 0;
static char *out_buf = 0;
static bool out_clr = false;

void px_init(void) {
	struct termios traw;

	px_cy = 0;
	px_cx = 0;
	px_out_h = 0;
	px_out_w = 0;
	px_block = true;

	tcgetattr(PP_STDOUT, &torig);
	traw = torig;
	traw.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP |INLCR|IGNCR|ICRNL|IXON);
	traw.c_oflag &= ~OPOST;
	traw.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	traw.c_cflag &= ~(CSIZE|PARENB);
	traw.c_cflag |= CS8;
	tcsetattr(PP_STDOUT, TCSANOW, &traw);

	write(PP_STDOUT, TSAVESCR, sizeof(TSAVESCR) - 1);
	px_winsz();
	signal(SIGWINCH, &px_sigwinch);
}

void px_sigwinch(int sig) {
	signal(SIGWINCH, SIG_IGN);
	px_winsz();
	if (px_on_resize) {
		px_on_resize();
	}
	signal(SIGWINCH, &px_sigwinch);
}

char *px_wint(char *p, int val, int pad) {
	int tmp;
	char *q;
	char *r;
	char c;

	if (val < 0) {
		val = -val;
		*p++ = '-';
	}

	q = p;

	do {
		tmp = val;
		val /= 10;
		*p++ = '0' + tmp - val * 10;
		pad--;
	} while (val);

	while (pad-- > 0) *p++ = ' ';

	r = p;

	p--;
	while (q < p) {
		c = *q;
		*q = *p;
		*p = c;
		p--;
		q++;
	}

	return r;
}

void px_print(char *s) {
	char *p;
	char c;
	p = out + px_cy * px_out_w + px_cx;
	while (p < out + out_sz && (c = *s++)) {
		*p++ = c;
	}
}

#ifndef PX_NOPRINTF
void px_printf(char *fmt, ...) {
	char *p;
	char *save;
	char c;
	int dval;
	int exp;
	int pad;
	char *sval;
	va_list arg;
	va_start(arg, fmt);
	pad = 0;
	p = out + px_cy * px_out_w + px_cx;
	while ((c = *fmt++)) {
		if (c == '%') {
			c = *fmt++;
_expand_arg:
			switch (c) {
			case 'd':
				dval = va_arg(arg, int);
				p = px_wint(p, dval, pad);
				pad = 0;
				break;
			case 's':
				sval = va_arg(arg, char*);
				if (pad <= 0) {
_expand_sval:
					while ((c = *sval++)) *p++ = c;
				} else {
					pad -= strlen(sval);
					while (pad-- > 0) *p++ = ' ';
					goto _expand_sval;
				}
				break;
			default:
				if (c >= '0' && c <= '9') {
					exp = 1;
					save = fmt - 1;
					while ((c = *fmt++) && c >= '0' && c <= '9') {
						exp *= 10;
					}
					pad = 0;
					while (exp > 0) {
						pad = pad + (*save++ - '0') * exp;
						exp /= 10;
					}
					goto _expand_arg;
				}
				*p++ = '%';
				*p++ = c;
			}
		} else {
			*p++ = c;
		}
	}
	va_end(arg);
}
#endif /* PX_NOPRINTF */

#ifndef PX_NORECT
void px_rect(ui16 h, ui16 w, char c) {
	char *p;
	char *pnext;
	char *pend;
	p = out + px_cy * px_out_w + px_cx;
	pend = out + (px_cy + h) * px_out_w + px_cx;
	if (pend >= out_end) pend = out_end - 1;
	while (p < pend) {
		pnext = p + w; 
		if (pnext > pend) pnext = pend;
		while (p < pnext) {
			*p++ = c;
		}
		p += px_out_w - w;
	}
	return;
}
#endif /* PX_NORECT */

void px_flush(void) {
	char *p;
	char c0;
	char c;
	int y;
	int x;
	int offs;
	int sz;
	bool cut;
	char cut_buf[PP_CUT_BUF_SZ];
	char *cut_buf_end;
	char *p1;
#ifdef PX_STATS
	px_stats.out_cut_last = 0;
	px_stats.out_mv_last = 0;
#endif /* PX_STATS */

	p = out_buf;
	y = 0;
	x = 0;
	offs = 0;
	cut = true;

	cut_buf_end = cut_buf;

	if (out_clr) {
		*p++ = '\033';
		*p++ = '[';
		*p++ = '2';
		*p++ = 'J';
		out_clr = false;
	}
	*p++ = '\033';
	*p++ = '[';
	*p++ = '1';
	*p++ = ';';
	*p++ = '1';
	*p++ = 'H';
	sz = px_out_h * px_out_w;
	while(offs < sz) {
		c0 = *(out0 + offs);
		c = *(out + offs);

		if (c != c0) {
			if (cut) {
				if (cut_buf_end - cut_buf < PP_CUT_BUF_SZ) {
					p1 = cut_buf;
					while (p1 < cut_buf_end) {
						*p++ = *p1++;
					}
					cut_buf_end = cut_buf;
#ifdef PX_STATS
					px_stats.out_cut_last++;
#endif /* PX_STATS */
				} else {
					*p++ = '\033';
					*p++ = '[';
					p = px_wint(p, y + 1, 0);
					*p++ = ';';
					p = px_wint(p, x + 1, 0);
					*p++ = 'H';
#ifdef PX_STATS
					px_stats.out_mv_last++;
#endif /* PX_STATS */
				}
				cut = false;
			}
			*p++ = c;
			*(out0 + offs) = c;
		} else {
			if (cut_buf_end - cut_buf < PP_CUT_BUF_SZ) {
				*cut_buf_end++ = c;
			}
			cut = true;
		}

		x++;
		offs++;
		if (x == px_out_w) {
			y++;
			x = 0;
		}
	}
	*p++ = '\033';
	*p++ = '[';
	p = px_wint(p, px_cy + 1, 0);
	*p++ = ';';
	p = px_wint(p, px_cx + 1, 0);
	*p++ = 'H';

	PX_CTR(out_bytes,
	write(PP_STDOUT, out_buf, p - out_buf)
	);
}

void px_winsz(void) {
	ioctl(PP_STDOUT, TIOCGWINSZ, &winsz);

	if (px_out_h != winsz.ws_row || px_out_w != winsz.ws_col) {
		px_out_resize(winsz.ws_row, winsz.ws_col);
		out_clr = true;
	}
}

void px_clear(void) {
	memset(out, ' ', px_out_h * px_out_w);
}

void px_out_resize(ui16 h, ui16 w) {
	size_t sz = h * w * sizeof(char);
	if (out_sz < sz) {
		if (out) out = realloc(out, sz); else out = malloc(sz);
		if (out0) out0 = realloc(out0, sz); else out0 = malloc(sz);
		if (out_buf) out_buf = realloc(out_buf, sz * PP_OUT_BUF_MULT);
		else out_buf = malloc(sz * PP_OUT_BUF_MULT);
		out_sz = sz;
		out_end = out + out_sz;
	}

	px_out_h = h;
	px_out_w = w;
	px_clear();
	memset(out0, 0, px_out_h * px_out_w);
}

void px_read_in(void) {
	ssize_t nread;
	int avail;
	in_buf_end = in_buf;
	in_buf_p = in_buf;
	if (!px_block) {
		ioctl(PP_STDOUT, FIONREAD, &avail);
		if (avail == 0) return;
	}
	PX_CTR(in_bytes,
	nread = read(PP_STDIN, &in_buf, 256)
	);
	if (nread > 0) {
		in_buf_end = in_buf + nread;
	}
}

ui8 px_get_key(void) {
	if (in_buf_p == in_buf_end) px_read_in();
	if (in_buf_p < in_buf_end) {
		return *in_buf_p++;
	} else {
		return 0;
	}
}

void px_cleanup(void) {
	if (out) {
		free(out);
		out = 0;
		out_sz = 0;
	}
	if (out0) {
		free(out0);
		out_sz = 0;
	}
	if (out_buf) {
		free(out_buf);
		out_buf = 0;
	}
	tcsetattr(PP_STDOUT, TCSANOW, &torig);
	write(PP_STDOUT, TRESTSCR, sizeof(TRESTSCR) - 1);
}

#ifdef PX_STATS
void px_stats_print(void) {
#define _P(X) px_cy++; px_printf(# X" = %d     ", X);
	_P(px_block);
	_P(px_stats.out_bytes_last);
	_P(px_stats.out_bytes_max);
	_P(px_stats.out_bytes_total);
	_P(px_stats.in_bytes_last);
	_P(px_stats.in_bytes_max);
	_P(px_stats.in_bytes_total);
	_P(px_stats.out_cut_last);
	_P(px_stats.out_mv_last);
#undef _P
}
#endif /* PX_STATS */
