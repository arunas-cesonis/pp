#ifndef PP_TREE_H
#define PP_TREE_H

#include <stdint.h>
#include <string.h>

#define PP_TREE_STACK_SZ 1024

struct pp_node {
	char *s;
	int slen;
	struct pp_key *keys;
	uint32_t ref;
};

struct pp_key {
	int d;
	struct pp_node *node;
	struct pp_key *prev;
};

typedef struct pp_key pp_key;
typedef struct pp_node pp_node;

pp_node *pp_tree_alloc(char *s, int slen, uint32_t ref);
void pp_tree_add(pp_node *h, char *s, int slen, uint32_t ref);
void pp_tree_free(pp_node *h);
void pp_keys_free(pp_key *it);
void pp_tree_find(pp_node *h, int d, char *s, int slen,
	int (*add)(pp_node*, int, int));

#endif /* PP_TREE_H */
