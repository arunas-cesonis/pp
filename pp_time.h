#ifndef PP_TIME_H
#define PP_TIME_H

typedef struct {
	struct timeval start;
	struct timeval dt;
} pp_clock_t;

void pp_clock_start(pp_clock_t *h);
double pp_clock_tick(pp_clock_t *h);

#endif /* PP_TIME_H */
