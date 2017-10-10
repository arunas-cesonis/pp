#ifndef PP_STR_H
#define PP_STR_H

#include "pp_config.h"

int pp_str_str(char *s, char **sv, int sv_len);
int pp_str_match(char *patt, char *text);
void pp_str_clean(char* path, int path_len, char **clean, int *base);
#ifdef ENABLE_TREE
int pp_str_dist(char *s1, int l1, char *s2, int l2);
#endif /* ENABLE_TREE */

#endif /* PP_STR_H */
