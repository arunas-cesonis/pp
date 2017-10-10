#ifndef PP_SCAN_H
#define PP_SCAN_H

void pp_scan(char *path, int (*filter)(char*), int (*add)(char*, int));

#endif /* PP_SCAN_H */
