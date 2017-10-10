#ifndef PX_H
#define PX_H
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define ui8 uint_fast8_t
#define ui8p uint8_t
#define ui16 uint_fast16_t
#define ui32 uint_fast32_t

#ifndef PX_EXTERN
#define PX_EXTERN extern
#endif /* PX_EXTERN */

PX_EXTERN ui16 px_cy;
PX_EXTERN ui16 px_cx;
PX_EXTERN ui16 px_out_h;
PX_EXTERN ui16 px_out_w;
PX_EXTERN bool px_block;
PX_EXTERN void (*px_on_resize)(void);
#ifdef PX_STATS
PX_EXTERN struct {
	ssize_t out_bytes_last;
	ssize_t out_bytes_total;
	ssize_t out_bytes_max;
	ssize_t in_bytes_last;
	ssize_t in_bytes_total;
	ssize_t in_bytes_max;
	ssize_t out_cut_last;
	ssize_t out_mv_last;
} px_stats;
#endif /* PX_STATS */

void px_init(void);
void px_print(char*);
#ifndef PX_NOPRINTF
void px_printf(char*, ...);
#endif /* PX_NOPRINTF */
#ifndef PX_NORECT
void px_rect(ui16, ui16, char);
#endif /* PX_NORECT */
void px_flush(void);
ui8 px_get_key(void);
void px_clear(void);
void px_cleanup(void);
#endif /* PX_H */
#ifdef PX_STATS
void px_stats_print(void);
#endif /* PX_STATS */
