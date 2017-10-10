#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "pp_debug.h"
#include "pp_str.h"
#include "pp_tree.h"

pp_key *node_getkey(pp_node *h, int d);

void pp_keys_free(pp_key *it) {
	pp_key *tmp;
	while (it) {
		tmp = it->prev;
		// TRACE("pp_keys_free: [%s]\n", it->node->s);
		free(it);
		it = tmp;
	}
}

pp_node *pp_tree_alloc(char *s, int slen, uint32_t ref) {
	pp_node *h;
	h = malloc(sizeof(pp_node));
	h->s = s;
	h->slen = slen;
	h->keys = 0;
	h->ref = ref;
	return h;
}

void pp_tree_free(pp_node *h) {
	pp_node *stack[PP_TREE_STACK_SZ];
	pp_node **p;
	pp_node *c;
	pp_key *it;
	pp_key *it_tmp;
	p = stack;
	*p = h;
	while (p >= stack) {
		c = *p--;
		it = c->keys;
		free(c->s);
		free(c);
		while (it) {
			*(++p) = it->node;
			it_tmp = it->prev;
			free(it);
			it = it_tmp;
		}
	}
}

pp_key *node_getkey(pp_node *h, int d) {
	pp_key *it = h->keys;
	while (it) {
		if (it->d == d) return it;
		it = it->prev;
	}
	return 0;
}

void pp_tree_add(pp_node *h, char *s, int slen, uint32_t ref) {
	int d;
	pp_key *key;

	d = pp_str_dist(s, slen, h->s, h->slen);

	while ((key = node_getkey(h, d))) {
		h = key->node;
		d = pp_str_dist(s, slen, h->s, h->slen);
	}

	key = malloc(sizeof(pp_key));
	key->d = d;
	key->node = pp_tree_alloc(s, slen, ref);
	key->prev = h->keys;
	h->keys = key;
}

void pp_tree_find(pp_node *h, int d, char *s, int slen,
	int (*add)(pp_node*, int, int)) {
	int cd;
	int cdmin;
	int cdmax;
	pp_node *c;
	pp_key *it;
	pp_node *stack[PP_TREE_STACK_SZ];
	pp_node **p;
	TRACE("pp_tree_find: (%d) %s\n", d, s);

	p = stack;
	*p = h;

	while (p - stack >= 0) {
		c = *p;
		p--;

		cd = pp_str_dist(s, slen, c->s, c->slen);
		if (c->slen > 0 && cd <= d) {
			TRACE("pp_tree_find:(%d) <= (%d) %s\n", cd, d, c->s);
			d = add(c, cd, d);
		} else {
			TRACE("pp_tree_find:DROP (%d) <= (%d) %s\n", cd, d, c->s);
		}

		cdmin = cd - d;
		cdmax = cd + d;

		it = c->keys;
		while (it) {
			if (it->d >= cdmin && it->d <= cdmax) {
				p++;
				*p = it->node;
			}
			it = it->prev;
		}
	}
}
