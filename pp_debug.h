#ifndef PP_DEBUG_H
#define PP_DEBUG_H

#define CLOSE_DEBUG_OUT
#define OPEN_DEBUG_OUT
#define ASSERT(X)
#define TRACE(...)
#define TRACE_d(...)
#define TRACE_s(...)
#define TRACE_X(...)

#ifdef DEBUG
// 
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

//
#undef ASSERT
#define ASSERT(X) assert(X);

//
FILE* debug_out;
#undef TRACE
#undef TRACE_d
#undef TRACE_s
#undef TRACE_X
#undef T
#define TRACE(...) fprintf(debug_out, __VA_ARGS__); fflush(debug_out);
#define TRACE_d(X) TRACE(PP_STR(X)" = %d\n", (int) X)
#define TRACE_s(X) TRACE(PP_STR(X)" = %s\n", (char*) X)
#define TRACE_X(X) TRACE(PP_STR(X)" = 0x%X\n", (char*) X)

// 
#ifdef DEBUG_OUT
#undef CLOSE_DEBUG_OUT
#define CLOSE_DEBUG_OUT fclose(debug_out);
#undef OPEN_DEBUG_OUT
#define OPEN_DEBUG_OUT debug_out = fopen(PP_STR(DEBUG_OUT), "a"); \
if (!debug_out) { \
	perror(PP_STR(DEBUG_OUT)); \
	exit(3); \
}
#else
#undef OPEN_DEBUG_OUT
#define OPEN_DEBUG_OUT debug_out = stderr;
#endif /* DEBUG_OUT */

#endif /* DEBUG */
#endif /* PP_DEBUG_H */
