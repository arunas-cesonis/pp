#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "pp_debug.h"
#include "pp_macros.h"
#include "pp_str.h"

int pp_str_str(char *s, char **sv, int sv_len) {
	char *sv_score[sv_len];
	// char **sv_score_last = sv_score + sv_len;
	// char **psv_score;
	int n;
	char *p;
	char c;

	memcpy(sv_score, sv, sv_len * sizeof(char*));

	while ((c = *s)) {

		n = 0;

		while (n < sv_len) {
			
			p = sv_score[n];

			if (*p == c) {
				p++;
				if (!*p) return n;
				sv_score[n] = p;
			} else {
				sv_score[n] = sv[n];	
			}

			n++;
		}

		s++;
	}

	return -1;
}

int pp_str_match(char *patt, char *text) {
	int score = 0;
	int ad = 1;
	char* pi = patt;
	char* pj = text;

	while (*pi != '\0' && *pj != '\0') {
		if (*pi == *pj) {
			pi++;
			pj++;
			score += 2 + ad;
			ad *= 2;
		} else {
			pj++;
			ad = 1;
		}
	}

	return score;
}

void pp_str_clean(char* path, int path_len, char **clean, int *base) {
	char *s;
	int b;
	char *p;
	char c;
	int len;

	len = path_len;

	s = malloc(len * sizeof(char) + 1);
	b = 0;

	p = s;
	while ((c = *path++)) {
		if (VALID_C(c)) {
			*p++ = c;
		} else {
			if (c == '/') {
				b = p - s;
			}
		}
	}
	*p = 0;

	*clean = s;
	*base = b;
}

#ifdef ENABLE_TREE
int pp_str_dist(char *s1, int l1, char *s2, int l2) {
	int a;
	int b;
	int c;
    int i;
	int j;
	int ld;
	int od;
    int m[512];

	if (l1 == 0) return l2;
	if (l2 == 0) return l1;
	while (*s1 == *s2) {
		s1++;
		s2++;
		l1--;
		l2--;
	}
	while (l1 && l2 && s1[l1 - 1] == s2[l2 - 1]) {
		l1--;
		l2--;
	}
	if (l1 == 0) return l2;
	if (l2 == 0) return l1;

    for (j = 1; j <= l1; j++) {
        m[j] = j;
	}

    for (i = 1; i <= l2; i++) {
        m[0] = i;
        for (j = 1, ld = i - 1; j <= l1; j++) {
            od = m[j];
			a = m[j] + 1;
			b = m[j - 1] + 1;
			c = ld + (s1[j - 1] == s2[i - 1] ? 0 : 1);
			if (a > b) a = b;
			if (a > c) m[j] = c;
			else m[j] = a;
            ld = od;
        }
    }
    return m[l1];
}
#endif /* ENABLE_TREE */
