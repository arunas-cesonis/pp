#ifndef PP_DATA_H
#define PP_DATA_H

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

typedef struct {
	int fd;
	size_t len;
	void *map;
} pp_data_map_t;

typedef struct {
	uint32_t str;
	uint32_t str_clean;
	uint32_t base;
	uint32_t hits;
	uint32_t t;
} pp_rec_t;

typedef struct {
	uint32_t sz;
	uint32_t len;
	char v[];
} pp_str_t;

typedef struct {
	uint32_t ts;
	uint32_t sz;
	uint32_t len;
#ifdef ENABLE_J_ZERO
	uint32_t j_zero;
#endif /* ENABLE_J_ZERO */
	pp_rec_t v[];
} pp_idx_t;

typedef struct {
	pp_data_map_t midx;
	pp_data_map_t mstr;
	pp_idx_t *idx;
	pp_str_t *str;
} pp_data_t;

pp_data_t *pp_data_open(char *path_idx, char *path_str);
uint32_t pp_data_append(pp_data_t *h, char *s, char *sc, uint32_t base,
	uint32_t hits, uint32_t t);
void pp_data_clear(pp_data_t *h);
void pp_data_close(pp_data_t *h);

#endif /* PP_DATA_H */
