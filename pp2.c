#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
// #include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "pp2.h"
#include "ascii_mask.h"

#define MEM_MAX (16 * 1024 *1024)
#define FIRST_READ (4096)
#define BUF_MAX (256 * 1024)
#define STACK_MAX 8192

static char *stack[STACK_MAX];
static char **stackp = stack;

static uint8_t buf[BUF_MAX];

inline static int
scan_text(const char *path, const uint8_t *s, int len) {
	const uint8_t *g = s;
	const uint8_t *e = s + len;
	while (g < e && ascii_mask[*g]!= 0) {
		 g++;
	}
	return g < e ? -1 : 0;
}

#define _FILE int
#define _GOOD(F) ((F) != -1)
#define _OPEN(S) open((S), O_RDONLY)
#define _READ(F,P,N) read((F), (P), (N))
#define _CLOSE(F) close((F))
#define _SEEK_END(F) lseek((F), 0, SEEK_END)
#define _SEEK_BEG(F) lseek((F), 0, SEEK_SET)

inline static void
scan_file(const char *s) {
		printf("TXT %s\n", s);
		return;
	struct stat st;
	_FILE fd = _OPEN(s);
	if (!_GOOD(fd)) return;
	off_t g = 0;
	off_t len = st.st_size;

	len = _SEEK_END(fd);
	if (-1 == len) err(EX_IOERR, "%s", s);
	if (-1 == _SEEK_BEG(fd)) err(EX_IOERR, "%s", s);

	int nr = _READ(fd, buf, FIRST_READ);
	while (g < len) {
		if (nr > 0) {
			g = g + nr;

			if (scan_text(s, buf, nr) == -1) {
				printf("size=%lu g=%llu BIN %s\n", (unsigned long)len, g, s);
				_CLOSE(fd);
				return;
			}
			
			nr = _READ(fd, buf, BUF_MAX);

		} else if (nr == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
			err(EX_IOERR, "%s", s);
		} 
	}
		
	if (len > 0) {
		printf("size=%lu g=%llu TXT %s\n", (unsigned long)len, g, s);
	} else {
		printf("size=0 NIL %s\n", s);
	}

	_CLOSE(fd);
	return;
}

void scan(const char *s) {

	char *path;
	char *sub;
	int path_len;
	DIR *dir;
	struct dirent *entry;

	path_len = strlen(s);
	path = malloc(path_len);
	memcpy(path, s, path_len);
	*(stackp++) = path;

	while (stackp > stack) {
		path = *(--stackp);
		dir = opendir(path);

		if (dir) {
			path_len = strlen(path);
			while ((entry = readdir(dir))) {

				const int type = entry->d_type;
				if (type != DT_REG && type != DT_DIR
					&& type != DT_LNK) continue;

				const char *d_name = entry->d_name;
				const int d_namelen = strlen(d_name);

				if (d_namelen <= 2
					&& (d_name[0] == '.'
					&& (d_namelen == 1 || d_name[1] == '.'))) {
					continue;
				}

				const int len = path_len + d_namelen + 2;
				sub = malloc(len * 2);
				strcpy(sub, path);
				strcat(sub, "/");
				strcat(sub, d_name);

				switch (entry->d_type) {
				case DT_REG:
					scan_file(sub);
					//write(1, sub, strlen(sub));
					//write(1, "\n", 1);
					free(sub);
					break;
				case DT_DIR:
					*(stackp++) = sub;
					break;
				}
			}
			closedir(dir);
		}
		free(path);
	}
}

int main(int argc, char **argv) {
	scan(".");
	return 0;
}
