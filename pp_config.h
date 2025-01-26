#ifndef PP_CONFIG_H
#define PP_CONFIG_H

// FIXME: PX_STATS is broken thus ENABLE_STATS is also

// #define ENABLE_STATS
#define ENABLE_DATATS
// #define ENABLE_TREE
#define ENABLE_J_ZERO
#define ENABLE_PX
#define ENABLE_PP_SCAN_FIND

#if defined(ENABLE_STATS) && defined(ENABLE_PX)
// #define PX_STATS
// -DPX_STATS
#endif
// -DPX_NORECT

#ifdef MAKEFILE_OBJ
#ifdef ENABLE_TREE
pp_tree.o
#endif
#ifdef ENABLE_PX
px.o
#endif
#endif

#ifdef MAKEFILE_LDFLAGS
#ifndef ENABLE_PX
-lncurses
#endif
#endif

#ifdef ENABLE_DATATS
#ifdef PP_DATATS
#if PP_DATATS + 0 == 0
#undef PP_DATATS
#define PP_DATATS 0x11
#endif
#else
#define PP_DATATS 0x22
#endif /* PP_DATATS */
#endif /* ENABLE_DATATS */

#endif /* PP_CONFIG_H */
