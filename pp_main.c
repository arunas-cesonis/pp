#include "pp_config.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <memory.h>
#include <limits.h>
#ifndef ENABLE_PX
#include <ncurses.h>
#endif /* ENABLE_PX */

#include "pp_debug.h"
#include "pp_macros.h"
#include "pp_time.h"
#include "pp_str.h"
#include "pp_scan.h"
#include "pp_data.h"
#ifdef ENABLE_PX
#include "px.h"
#endif /* ENABLE_PX */
#ifdef ENABLE_TREE
#include "pp_tree.h"
#endif

#ifdef ENABLE_PX
#define TERM_INIT() px_init()
#define TERM_CLOSE() px_cleanup()
#define TERM_H() px_out_h
#define TERM_W() px_out_w
#define TERM_BLOCK() px_block = true
#define TERM_NONBLOCK() px_block = false
#define TERM_NO_KEY() 0
#define TERM_GET_KEY() px_get_key()
#define TERM_ERASE() px_clear()
#define TERM_MOVE(Y,X) px_cy = (Y); px_cx = (X)
#define TERM_MVPRINT(Y,X,S) TERM_MOVE(Y,X); px_print(S)
#define TERM_FLUSH() px_flush()
#else /* ENABLE_PX */
#define TERM_INIT() initscr(); raw(); noecho()
#define TERM_CLOSE() endwin()
#define TERM_H() getmaxy(stdscr)
#define TERM_W() getmaxx(stdscr)
#define TERM_BLOCK() timeout(-1)
#define TERM_NONBLOCK() timeout(0)
#define TERM_NO_KEY() ERR
#define TERM_GET_KEY() getch()
#define TERM_ERASE() erase()
#define TERM_MOVE(Y,X) move(Y, X)
#define TERM_MVPRINT(Y,X,S) mvprintw(Y,X,"%s",S)
#define TERM_FLUSH()
#endif /* ENABLE_PX */

#define FILL_CH ' '
#define MATCHES_SZ 128
#define MATCH_POS_SZ 64
#define LINE_SZ 32
#define MAX_W 256

#define CMD_NOOP 0
#define CMD_SEL_DOWN 1
#define CMD_SEL_UP 2
#define CMD_RESCAN 3
#define CMD_EXEC 4
#define CMD_CANCEL 5
#define CMD_TIMEOUT 6

typedef struct {
	pp_rec_t *rec;
	int score;
	int t;
} pp_m_t;

static char line[LINE_SZ];
static int line_cr;
static pp_m_t m[MATCHES_SZ];
static pp_data_t *data;
#ifdef ENABLE_TREE
static pp_node *tree;
#endif
static int mlen;
static int mbase;
static int ui_loop;
static int ui_w;
static int ui_wb;
static int ui_h;
static char choice[PATH_MAX];
static char data_path[PATH_MAX];
static int sel;
static int cmd;
static int opt_2;

#define SCAN_NOOP 0
#define SCAN_RUNNING 1
#define SCAN_ABORTED 2
#define SCAN_DONE 3

#ifdef ENABLE_STATS
static int draw_stats;
static struct {
	int draw_while_find_count;
	double find_time_last;
	int find_time_count;
	double find_time_sum;
	double scan_time;
	int ca;
} stats;
#endif /* ENABLE_STATS */

static int scan_status;

static char* ignore[] = {
	"node_modules",
	"/target/",
	"src/tests/java"
};

void _ui(void);
void _ui_poll(void);
#ifdef ENABLE_STATS
void _ui_draw_stats(void);
#endif /* ENABLE_STATS */
void _ui_draw(void);
void _ui(void);
void _ui_init(void);
int _filter(char *path);
int _add_scan(char *path, int len);
void _do_scan(void);
uint32_t _now(void);
void _exec_prep(void);
void _cleanup(void);
void pp_insert_match(pp_m_t* match);
void pp_find_matches2(void);
#ifdef ENABLE_TREE
int pp_insert_match_tree(pp_node *c, int cd, int d);
void pp_find_matches_tree(void);
#endif /* ENABLE_TREE */
void _get_key(void);
void _clamp_sel(void);
void _do_exec(void);
void _do_cmd(void);
void _mk_data_file(char *data_file, int max);
void _init_data(void);
#ifdef ENABLE_TREE
void _init_tree(void);
#endif
void _init_vars(void);
void _init_scan(void);
void _usage(void);
void _init_opts(int argc, char **argv);

int _filter(char *path) {
	return pp_str_str(path, ignore, sizeof(ignore) / sizeof(char*)) == -1;
}

int _add_scan(char *path, int len) {
	char *clean;
	int base;
	pp_str_clean(path, len, &clean, &base);

#ifdef ENABLE_TREE
	uint32_t ref;
	ref =
#endif
	pp_data_append(data, path, clean, base, 0, 0);

#ifdef ENABLE_TREE
	pp_tree_add(tree, clean, strlen(clean), ref);
#else
	free(clean);
#endif

	_ui_poll();
	if (cmd == CMD_EXEC || cmd == CMD_CANCEL) {
		return 1;
	}
	return 0;
}

void _do_scan(void) {
#ifdef ENABLE_STATS
	pp_clock_t clk;
#endif /* ENABLE_STATS */
	scan_status = SCAN_RUNNING;
	TERM_NONBLOCK();
	_ui_draw();
#ifdef ENABLE_STATS
	pp_clock_start(&clk);
#endif /* ENABLE_STATS */
	pp_scan(".", _filter, _add_scan);
#ifdef ENABLE_STATS
	stats.scan_time = pp_clock_tick(&clk);
#endif /* ENABLE_STATS */
	TERM_BLOCK();
	if (scan_status == SCAN_RUNNING) {
		scan_status = SCAN_DONE;
	}
}

uint32_t _now(void) {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec;
}

void _exec_prep(void) {
	TRACE("_exec_prep\n")
	
#ifdef ENABLE_J_ZERO
	pp_rec_t tmp;
	pp_rec_t* dst;
#endif /* ENABLE_J_ZERO */
	pp_rec_t* rec;

	rec = m[sel].rec;

	rec->hits++;
	rec->t = _now();
	strcpy(choice, data->str->v + rec->str);

#ifdef ENABLE_J_ZERO
	if (rec->hits == 1 && (data->idx->j_zero < data->idx->len)) {
		dst = data->idx->v + data->idx->j_zero;
		tmp = *dst;
		*dst = *(m[sel].rec);
		*(m[sel].rec) = tmp;
		data->idx->j_zero++;
	}
#endif /* ENABLE_J_ZERO */

}

void _cleanup(void) {
	TERM_CLOSE();
	if (data) {
		pp_data_close(data);
		data = 0;
	}
#ifdef ENABLE_TREE
	if (tree) {
		pp_tree_free(tree);
		tree = 0;
	}
#endif
}

void pp_insert_match(pp_m_t* match) {
	int n;
	int d;
	int j;
	j = mlen;
	while (j > 0) {
		j--;
		d = match->score - m[j].score;
		if (d < 0 || (d == 0 && match->t < m[j].t)) {
			j++;
			break;
		}
	}
	if (j == mlen) {
		if (mlen < ui_h) {
			m[j] = *match;
			mlen++;
		}
		return;
	}
	n = mlen;
	if (mlen == ui_h) {
		n--;
	} else {
		mlen++;
	}
	while (n > j) {
		m[n] = m[n - 1];
		n--;
	}
	m[j] = *match;
}

void pp_find_matches2(void) {
	pp_rec_t *p;
	pp_rec_t *end;
	pp_m_t match;
	char *clean;
	int score_text;
	int score_hits;
#ifdef ENABLE_STATS
	pp_clock_t clk;
	pp_clock_start(&clk);
#endif /* ENABLE_STATS */

	mlen = 0;
	ui_h = TERM_H();

	p = data->idx->v;
	end = p + data->idx->len;

	while (p < end) {
		clean = data->str->v + p->str_clean;
		score_text = pp_str_match(line, clean + (mbase ? p->base : 0));
		score_hits = p->hits;

		if (score_hits > score_text) {
			score_hits = score_text;
		}

		match.score = score_text + score_hits;
		match.rec = p;
		match.t = p->t;
		pp_insert_match(&match);
		p++;
	}

#ifdef ENABLE_STATS
	stats.find_time_last = pp_clock_tick(&clk);
	stats.find_time_sum += stats.find_time_last;
	stats.find_time_count++;
#endif /* ENABLE_STATS */
}

#ifdef ENABLE_TREE
int pp_insert_match_tree(pp_node *c, int cd, int d) {
	pp_m_t match;

	match.score = -cd;
	match.rec = data->idx->v + c->ref;
	match.t = 0;
	pp_insert_match(&match);

	TRACE("pp_insert_match_tree: c->s=%s data=%u\n", c->s, c->ref);
	return d;
}

void pp_find_matches_tree(void) {
#ifdef ENABLE_STATS
	pp_clock_t clk;
	pp_clock_start(&clk);
#endif /* ENABLE_STATS */
	TRACE("pp_find_matches_tree\n");
	mlen = 0;
	if (line_cr > 0) {
		pp_tree_find(tree, line_cr, line, line_cr, pp_insert_match_tree);
	}
#ifdef ENABLE_STATS
	stats.find_time_last = pp_clock_tick(&clk);
	stats.find_time_sum += stats.find_time_last;
	stats.find_time_count++;
#endif /* ENABLE_STATS */
}
#endif /* ENABLE_TREE */

void _get_key(void) {
	int ch;
	cmd = CMD_NOOP;
	ch = TERM_GET_KEY();
	switch (ch) {
	case 1:  // C-a
		mbase = !mbase;
		break;
	case 3:  // C-c
	case 4:  // C-d
		cmd = CMD_CANCEL;
		break;
#ifdef ENABLE_STATS
	case 9: // Tab
		draw_stats = !draw_stats;
		TRACE("draw_stats=%d\n", draw_stats);
		break;
#endif /* ENABLE_STATS */
	case 13: // Vim Enter
	case 10: // Enter
		cmd = CMD_EXEC;
		break;
	case 14: // C-n
		cmd = CMD_SEL_DOWN;
		break;
	case 16: // C-p
		cmd = CMD_SEL_UP;
		break;
	case 18: // C-r
		cmd = CMD_RESCAN;
	break;
	case 21: // C-u
	case 23: // C-w
		line_cr = 0;
		line[0] = '\0';
		break;
	case 32: // Space
		break;
	case -25195: // Vim C-h
	case 8: // C-h
	case 127: // C-h
		if (line_cr > 0) line[--line_cr] = '\0';
		break;
	case TERM_NO_KEY():
		cmd = CMD_TIMEOUT;
		break;
	default:
		if ((line_cr < LINE_SZ - 1)
			&& VALID_C(ch)
			) {
			TRACE("type=%c\n", ch);
			line[line_cr++] = ch;
			line[line_cr] = '\0';	
		}
	}
#ifdef DEBUG
	// TRACE("ch=%d cmd=%d\n", ch)
	if (cmd && cmd != CMD_TIMEOUT) {
		TRACE("cmd=%d\n", cmd)
	}
#endif /* DEBUG */
}

#ifdef ENABLE_STATS
void _ui_draw_stats(void) {
	int n;
	char out[MAX_W];
	int y = ui_h - 15;

/* char out_ ## NAME[MAX_W]; \ */
#define _FIELD_EX(NAME,FMT,VAL) \
	n = snprintf(out, ui_wb, PP_STR(NAME)" = "FMT, VAL); \
	TERM_MVPRINT(y++, ui_w - n, out);
#define _FIELD(NAME,FMT) _FIELD_EX(NAME,FMT,NAME)
#define _STAT(NAME,FMT) _FIELD_EX(NAME,FMT,PP_P(stats)PP_DOT()PP_P(NAME))

	_STAT(draw_while_find_count,"%-7d")
	_STAT(find_time_count,"%-7d")
	_FIELD_EX(find_time_avg,"%.4fs", stats.find_time_sum / stats.find_time_count)
	_STAT(find_time_last,"%.4fs")
	_FIELD(data->idx->len,"%-7d")
	_FIELD(data->idx->sz,"%-7d")
	_STAT(scan_time,"%.4fs")
	_STAT(ca,"%-7d")
#ifdef ENABLE_PX
	_FIELD((int)px_stats.out_bytes_last,"%-7d")
	_FIELD((int)px_stats.out_bytes_max,"%-7d")
	_FIELD((int)px_stats.out_bytes_total,"%-7d")
	_FIELD((int)px_stats.out_cut_last,"%-7d")
	_FIELD((int)px_stats.out_mv_last,"%-7d")
#endif /* ENABLE_PX */

#undef _FIELD_EX
#undef _STAT

}
#endif /* ENABLE_STATS */

void _ui_draw(void) {
	int count;
	int y;
	int n;
	int status_h = 1;
	char out[MAX_W];
	char *s_sel;

	ui_h = TERM_H();
	ui_w = TERM_W();
	ui_wb = ui_w + 1;

	if (ui_wb > MAX_W) ui_w = MAX_W;

	count = ui_h - status_h;
	if (count > mlen) count = mlen;

	y = ui_h - status_h - 1;
	TERM_ERASE();
	for (n = 0; n < count; n++) {
		if (sel == n) s_sel = ">";
		else s_sel = " ";

#ifdef ENABLE_STATS
		if (draw_stats) {
			snprintf(out, ui_wb, "%2d %2d %2d %s %s %s",
				m[n].score, m[n].rec->hits, m[n].rec->t,
				s_sel,
				data->str->v + m[n].rec->str_clean,
				data->str->v + m[n].rec->str);
			TERM_MVPRINT(y, 0, out);
		} else {
#endif /* ENABLE_STATS */
			snprintf(out, ui_wb, "%s %s", s_sel,
				data->str->v + m[n].rec->str);
			TERM_MVPRINT(y, 0, out);
#ifdef ENABLE_STATS
		}
#endif /* ENABLE_STATS */

		y--;
	}

#ifdef ENABLE_STATS
	if (draw_stats) _ui_draw_stats();
#endif /* ENABLE_STATS */

#define _DRAW_PROMPT(PROMPT,PROMPT_LEN) { \
	snprintf(out, ui_wb, PROMPT"%s", line); \
	TERM_MVPRINT(ui_h - 1, 0, out); \
	TERM_MOVE(ui_h - 1, line_cr + PROMPT_LEN); \
}

#ifdef ENABLE_STATS
	if (draw_stats)
		_DRAW_PROMPT("      > ", 8)
	 else
#endif /* ENABLE_STATS */
		_DRAW_PROMPT("> ", 2)
	
	TERM_FLUSH();
}

void _clamp_sel(void) {
	if (sel < 0) sel = 0;
	else if (sel >= mlen) sel = mlen - 1;
}

void _do_exec(void) {
	TRACE("_do_exec choice=\"%s\"\n", choice)
	if (*choice == '\0') return;

	if (opt_2) {
		TRACE("_do -2\n")
		fprintf(stderr, "%s\n", choice);
		return;
	}

	int ret;
	char *cmd_path = 0;

	struct stat sb;
	if (stat(choice, &sb) != -1) {
		if (sb.st_mode & S_IFDIR) {
			if (chdir(choice) != -1) {
				cmd_path = "pp";
			}
		}
	}

	if (cmd_path == 0)  {
		cmd_path = getenv("EDITOR");
		if (!cmd_path) cmd_path = "vi";
	}

	_cleanup();
	ret = execlp(cmd_path, cmd_path, choice, (char*)NULL);
	if (ret == -1) {
		perror(choice);
	}
}


void _do_cmd(void) {

	switch (cmd) {

	case CMD_SEL_DOWN:
		TRACE("CMD_SEL_DOWN\n")
		sel--;
		_clamp_sel();
		break;

	case CMD_SEL_UP:
		TRACE("CMD_SEL_UP\n")
		sel++;
		_clamp_sel();
		break;

	case CMD_RESCAN:
		TRACE("CMD_RESCAN scan_status=%d (need %d)\n",
			scan_status, SCAN_DONE);
		if (scan_status == SCAN_DONE) {
			mlen = 0;
			sel = 0;
			pp_data_clear(data);
			_do_scan();
		}
		break;

	case CMD_EXEC:
		TRACE("CMD_EXEC\n")
		ui_loop = 0;
		if (scan_status == SCAN_RUNNING) {
			scan_status = SCAN_ABORTED;
		}
		if (mlen) _exec_prep();
		// _cleanup();
		break;

	case CMD_CANCEL:
		TRACE("CMD_CANCEL\n")
		if (scan_status == SCAN_RUNNING) {
			scan_status = SCAN_ABORTED;
		}
		ui_loop = 0;
		// _cleanup();
		break;

	case CMD_TIMEOUT:
		ui_loop = 0;
		break;
		
	}

	TRACE("sel=%d mlen=%d\n", sel, mlen)
}

void _ui_poll(void) {
	_get_key();
	if (cmd != CMD_TIMEOUT) {
		_do_cmd();
		if (scan_status != SCAN_ABORTED) {
#ifdef ENABLE_TREE
			pp_find_matches_tree();
#else /* ENABLE_TREE */
			pp_find_matches2();
#endif /* ENABLE_TREE */
			_ui_draw();
		}
	}
}

void _ui(void) {
	while (ui_loop) {
#ifdef ENABLE_TREE
		pp_find_matches_tree();
#else /* ENABLE_TREE */
		pp_find_matches2();
#endif /* ENABLE_TREE */
		_ui_draw();

#ifdef DEBUG_AUTO
		cmd = DEBUG_AUTO;
#else
		_get_key();
#endif

		_do_cmd();
	}
}

void _mk_data_file(char *data_file, int max) {
	char* p;
	getcwd(data_file, max);
	p = data_file;
	while (*p != '\0') {
		if (!isalnum((int)*p)) *p = '_';
		p++;
	}
}

void _init_data(void) {
	char data_file[PATH_MAX];
	char path_idx[PATH_MAX];
	char path_str[PATH_MAX];
	
	_mk_data_file(data_file, PATH_MAX / 2);

	sprintf(data_path, "%s/.cache", getenv("HOME"));
	mkdir(data_path, S_IRWXU);
	sprintf(data_path, "%s/pp", data_path);
	mkdir(data_path, S_IRWXU);
	sprintf(data_path, "%s/%s", data_path, data_file);

	TRACE("data_file='%s'\n", data_file)
	TRACE("data_path='%s'\n", data_path)

	sprintf(path_str, "%s.str", data_path);
	sprintf(path_idx, "%s.idx", data_path);
	data = pp_data_open(path_idx, path_str);

#ifdef ENABLE_J_ZERO
#ifdef DEBUG
	TRACE("data->idx->j_zero=%d\n", data->idx->j_zero);
	TRACE("data->idx->len=%d\n", data->idx->len);
	int i;
	for (i = 0; i < data->idx->j_zero; i++) {
		TRACE("data->j_zero(-%d)=%s\n", i, data->str->v + data->idx->v[i].str);
	}
#endif
#endif /* ENABLE_J_ZERO */
}

#ifdef ENABLE_TREE
void _init_tree(void) {
	char *s = malloc(sizeof(char));
	*s = 0;
	tree = pp_tree_alloc(s, 0, 0);
}
#endif /* ENABLE_TREE */

void _init_vars(void) {

	data = 0;
#ifdef ENABLE_TREE
	tree = 0;
#endif /* ENABLE_TREE */
	ui_loop = 1;
	cmd = CMD_NOOP;
	sel = 0;
	scan_status = SCAN_NOOP;
#ifdef ENABLE_STATS
	draw_stats = 0;
	stats.draw_while_find_count = 0;
	stats.find_time_last = 0;
	stats.find_time_sum = 0;
	stats.find_time_count = 0;
	stats.ca = 0;
#endif /* ENABLE_STATS */
	*choice = '\0';

#ifdef DEBUG_LINE
	strcpy(line, PP_STR(DEBUG_LINE));
	line_cr = strlen(line);
#else
	*line = '\0';
	line_cr = 0;
#endif

	mlen = 0;
	mbase = 1;
}

void _init_scan(void) {
	if (!data->idx->len) {
		TRACE("_init_scan\n")
		_do_scan();
	} else {
		scan_status = SCAN_DONE;
		TRACE("_init_scan skip\n")
	}
}

#define USAGE ""\
"Usage: pp [-2]\n"\
"   -2  output selected path to stderr\n"

void _usage(void) {
	write(STDERR_FILENO, USAGE, sizeof(USAGE));
}

void _init_opts(int argc, char **argv) {
	TRACE("_init_opts\n")
	int ch;
	opt_2 = 0;
	while ((ch = getopt(argc, argv, "2h")) != -1) {

		switch (ch) {
		case '2':
			TRACE("getopt: -2\n")
			opt_2 = 1;
			break;
		case 'h':
		default:
			_usage();
			exit(1);
		}
	}
}

void _ui_init(void) {
	TERM_INIT();
	ui_h = TERM_H();
	ui_w = TERM_W();
}

int main(int argc, char **argv) {
	OPEN_DEBUG_OUT

	TRACE("== pp ==\n");

	_init_opts(argc, argv);
	_init_vars();
	_ui_init();
	atexit(_cleanup);
	_init_data();
#ifdef ENABLE_TREE
	_init_tree();
#endif
	_init_scan();
	_ui();
	_do_exec();

	CLOSE_DEBUG_OUT
	return 0;
}

