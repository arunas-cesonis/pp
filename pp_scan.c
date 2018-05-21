#include "pp_config.h"
#include <string.h>
#ifdef ENABLE_PP_SCAN_FIND
// #define _BSD_SOURCE
#include <stdio.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#endif /* ENABLE_PP_SCAN_FIND */

#include "pp_debug.h"
#include "pp_scan.h"

#define BUF_SZ 1024

#ifdef ENABLE_PP_SCAN_FIND
void pp_scan(char *path, int (*filter)(char*), int (*add)(char*, int)) {
	FILE *f = popen("find . "
		"-name '.?*' -prune "
		"-o \\( -type f -o -type d -o -type l \\) "
		"-a -print "
		"2>/dev/null", "r");
	char buf[BUF_SZ];
	int len;
	if (!f) return;
	while (fgets(buf, BUF_SZ, f)) {
		len = strlen(buf);
		buf[--len] = 0;
		if (!filter(buf)) continue;
		if (add(buf, len)) break;
	}
	pclose(f);
}

#else /* ENABLE_PP_SCAN_FIND */
#define STACK_SZ 16384

void pp_scan(char *path, int (*filter)(char*), int (*add)(char*, int)) {

	TRACE("pp_scan [%s]\n", path)
	char *s[STACK_SZ];
	char **p;
	char *c;
	char *sub;
	char *sub_p;
	DIR *dir;
	struct dirent *e;
	char *d_name;
	int len;

#ifdef DEBUG
	int opendir_fails = 0;
	int ignored_paths = 0;
	int unknowns = 0;
#endif /* DEBUG */

	len = strlen(path) + 1;
	c = malloc(sizeof(char) * len);
	memcpy(c, path, len);

	*s = c;
	p = s;

	while (p >= s) {
		c = *p;
		p--;

		dir = opendir(c);

		if (!dir) {
#ifdef DEBUG
			TRACE("pp_scan opendir fail: %s\n", c)
			opendir_fails++;
#endif /* DEBUG */
			free(c);
			continue;
		}

		if (!filter(c)) {
#ifdef DEBUG
			TRACE("pp_scan ignore path: %s\n", c)
			ignored_paths++;
#endif /* DEBUG */
			free(c);
			closedir(dir);
			continue;
		}

		while ((e = readdir(dir))) {
			d_name = e->d_name;
			if (*d_name == '.') continue;
	
			len = strlen(c) + strlen(d_name) + 1;
			sub = (char*) malloc(sizeof(char) * (len + 1));

			sub_p = stpcpy(sub, c);
			*sub_p++ = '/';
			stpcpy(sub_p, d_name);

			switch (e->d_type) {

			case DT_DIR:
				ASSERT(p - s < STACK_SZ)
				*(++p) = sub;

			case DT_REG:
			case DT_LNK:
				if (add(sub, len)) {
					TRACE("pp_scan: cancelled\n")
					free(c);
					closedir(dir);
					if (*p != sub) {
						free(sub);
					}
					while (p >= s) {
						c = *p;
						free(c);
						p--;
					}
					return;
				}
				if (e->d_type != DT_DIR) {
					free(sub);
				}
				break;

			default:
				free(sub);
#ifdef DEBUG
				unknowns++;
#endif
			}
		}
		
		free(c);
		closedir(dir);
	}

	TRACE("opendir_fails=%d\n", opendir_fails)
	TRACE("ignored_paths=%d\n", ignored_paths)
	TRACE("unknowns=%d\n", unknowns)
}
#endif /* ENABLE_PP_SCAN_FIND */
