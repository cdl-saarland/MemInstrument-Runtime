#pragma once

#include "config.h"

#include <assert.h>

#define UNUSED(x) (void)x;

#define UNREACHABLE(msg) assert(false && msg);

#ifdef CONTINUE_ON_FATAL
#define MI_NO_RETURN
#else
#define MI_NO_RETURN _Noreturn
#endif

#if ASSERT_LEVEL >= AL_TREE_COMPLEX
#define ASSERT_IMPL_AL_TREE_COMPLEX(x)                                         \
    do {                                                                       \
        assert(x);                                                             \
    } while (false);
#else
#define ASSERT_IMPL_AL_TREE_COMPLEX(x)                                         \
    do {                                                                       \
    } while (false);
#endif

#if ASSERT_LEVEL >= AL_TREE
#define ASSERT_IMPL_AL_TREE(x)                                                 \
    do {                                                                       \
        assert(x);                                                             \
    } while (false);
#else
#define ASSERT_IMPL_AL_TREE(x)                                                 \
    do {                                                                       \
    } while (false);
#endif

#if ASSERT_LEVEL >= AL_INPUT
#define ASSERT_IMPL_AL_INPUT(x)                                                \
    do {                                                                       \
        assert(x);                                                             \
    } while (false);
#else
#define ASSERT_IMPL_AL_INPUT(x)                                                \
    do {                                                                       \
    } while (false);
#endif

#define ASSERTION(x, l) ASSERT_IMPL_##l(x)

#ifdef STATISTICS
#define STATS(x)                                                               \
    do {                                                                       \
        x;                                                                     \
    } while (false);
#else
#define STATS(x)                                                               \
    do {                                                                       \
    } while (false);
#endif

#if ASSERT_LEVEL >= 4
#endif

#ifdef TRACER_FILE
#define ENABLE_TRACER 1
#endif
