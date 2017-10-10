#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/mman.h>

#include "pp_config.h"
#include "pp_debug.h"
#include "pp_data.h"

#define INIT_IDX_SZ 1024
#define INIT_STR_SZ 1024
#define idx2MAP(X) sizeof(pp_idx_t) + X * sizeof(pp_rec_t)
#define str2MAP(X) sizeof(pp_str_t) + X * sizeof(char)
#define RESIZE(X,SZ) \
_map_resize(&h->m ## X, X ## 2MAP(SZ)); \
h->X = h->m ## X.map; \
h->X->sz = SZ;

#define TRACE_STATE(X) TRACE(X" idx_fd=%d idx->sz=%d idx->sz=%d \n", \
	h->midx.fd, h->idx->sz, h->idx->len)

#define _GROW_BUF_SZ 4096
static char _grow_buf[_GROW_BUF_SZ];

void _map_open(pp_data_map_t *h, char *path);
void _map_close(pp_data_map_t *h);
void _map_resize(pp_data_map_t *h, size_t len);
uint32_t _put_str(pp_data_t *h, char *s);

void _map_open(pp_data_map_t *h, char *path) {
	size_t len;
	
	h->fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	TRACE("_map_open fd=%d\n", h->fd)

	len = lseek(h->fd, 0, SEEK_END);

	if (!len) {
		h->map = 0;
		h->len = 0;
		return;
	}

	lseek(h->fd, 0, SEEK_SET);
	h->map = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, h->fd, 0);
	h->len = len;

	ASSERT(h->map != MAP_FAILED)
	TRACE("_map_open fd len=%li\n", h->len)
}

void _map_close(pp_data_map_t *h) {
	msync(h->map, 0, MS_SYNC);
	munmap(h->map, h->len);
	// fsync(h->fd);
	close(h->fd);
}

void _map_resize(pp_data_map_t *h, size_t len) {

	off_t end = lseek(h->fd, 0, SEEK_END);
	ssize_t n;
	size_t left;

	if (len < end) {
		ftruncate(h->fd, len);
	} else {
		left = len - end;
		while (left > 0) {
			n = write(h->fd, _grow_buf, left > _GROW_BUF_SZ ? _GROW_BUF_SZ : left);
			left -= n;
		}
	}

	TRACE("_map_resize ")

	if (h->map) {
		TRACE("mremap %lu -> %lu\n", h->len, len)
#ifdef MREMAP_MAYMOVE
		h->map = mremap(h->map, h->len, len, MREMAP_MAYMOVE);
#else
		munmap(h->map, h->len);
		h->map = mmap(h->map, len, PROT_READ | PROT_WRITE, MAP_SHARED, h->fd, 0);
#endif
		h->len = len;
	} else {
		TRACE("mmap %lu -> %lu\n", h->len, len)
		h->map = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, h->fd, 0);
		h->len = len;
	}

	ASSERT(h->map != MAP_FAILED)

}

pp_data_t *pp_data_open(char *path_idx, char *path_str) {

	TRACE("path_idx=%s\n", path_idx)
	TRACE("path_str=%s\n", path_str)

#ifdef DEBUG
	memset(_grow_buf, 0xA7, _GROW_BUF_SZ);
#endif /* DEBUG */

	pp_data_t *h = malloc(sizeof(pp_data_t));

	_map_open(&h->midx, path_idx);
	_map_open(&h->mstr, path_str);

	if (!h->midx.len || !h->mstr.len
#ifdef ENABLE_DATATS
		|| (h->midx.len < offsetof(pp_idx_t, ts) + sizeof(uint32_t)
		|| ((pp_idx_t*)h->midx.map)->ts != PP_DATATS)
#endif
		) {
		RESIZE(idx,INIT_IDX_SZ)
#ifdef ENABLE_DATATS
		h->idx->ts = PP_DATATS;
#endif
		h->idx->len = 0;
		RESIZE(str,INIT_STR_SZ)
		h->str->len = 0;
#ifdef ENABLE_J_ZERO
		h->idx->j_zero = 0;
#endif
	} else {
		h->idx = h->midx.map;
		h->str = h->mstr.map;
	}

	TRACE_STATE("pp_data_open")

	return h;
}

void pp_data_close(pp_data_t *h) {
	_map_close(&h->midx);
	_map_close(&h->mstr);
	free(h);
}

uint32_t _put_str(pp_data_t *h, char *s) {
	char *ps;
	size_t ssz = strlen(s) + 1;
	uint32_t sz;
	uint32_t at;

	ASSERT(h->str->len <= h->str->sz)

	if (h->str->len + ssz >= h->str->sz) {
		sz = h->str->sz << 1;
		RESIZE(str,sz)
	}

	at = h->str->len;
	ps = h->str->v + at;
	memcpy(ps, s, ssz);
	h->str->len += ssz;

	return at;
}

uint32_t pp_data_append(pp_data_t *h, char *s, char *sc, uint32_t base,
	uint32_t hits, uint32_t t) {
	uint32_t sz;
	pp_rec_t *p;
	uint32_t idx;

	ASSERT(h->idx->len <= h->idx->sz)

	if (h->idx->len == h->idx->sz) {
		sz = h->idx->sz << 1;
		TRACE("RESIZE %d -> %d\n", h->idx->sz, sz);
		RESIZE(idx,sz)
	}
	
	idx = h->idx->len++;

	p = h->idx->v + idx;
	p->str = _put_str(h, s);
	p->hits = hits;
	p->str_clean = _put_str(h, sc);
	p->base = base;
	p->t = t;

	return idx;
}

void pp_data_clear(pp_data_t *h) {
	RESIZE(idx,INIT_IDX_SZ)
	h->idx->len = 0;
	RESIZE(idx,INIT_STR_SZ)
	h->str->len = 0;
}
