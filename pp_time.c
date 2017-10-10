#include <sys/time.h>
#include "pp_time.h"

void pp_clock_start(pp_clock_t *h) {
	gettimeofday(&h->start, 0);
	h->dt.tv_sec = 0;
	h->dt.tv_usec = 0;
}

double pp_clock_tick(pp_clock_t *h) {
	struct timeval now;
	struct timeval dt;
	gettimeofday(&now, 0);

	dt.tv_sec = now.tv_sec - h->start.tv_sec;
	dt.tv_usec = now.tv_usec - h->start.tv_usec;
	if (dt.tv_usec < 0) {
		dt.tv_sec--;
		dt.tv_usec = 1000000 - h->start.tv_usec + now.tv_usec;
	}

	return (double)dt.tv_sec + (double)dt.tv_usec * 1e-6;
}
