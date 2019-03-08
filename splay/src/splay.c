#include "splay.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "fail_function.h"
#include "macros.h"
#include "statistics.h"
#include "tracer.h"
#include "tree.h"
#include "tree_tools.h"

Tree __memTree;

static void dumpSplayAllocMapConditionally(void) {
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
    dumpSplayAllocMap(stderr, &__memTree);
#endif
}

MI_NO_RETURN static void __mi_fail_wrapper(const char *msg, void *ptr,
                                           char *vmsg) {
    dumpSplayAllocMapConditionally();
    if (vmsg) {
        __mi_fail_verbose_with_ptr(msg, ptr, vmsg);
    } else {
        __mi_fail_with_ptr(msg, ptr);
    }
}

void __splay_dumpAllocationMap(void) { dumpSplayAllocMap(stderr, &__memTree); }

void __splay_dumpProcMap(void) {
    char buf[32];
    pid_t pid = getpid();
    snprintf(buf, 32, "/proc/%d/maps", pid);
    FILE *F = fopen(buf, "r");

    char *line;
    size_t sz = 0;

    for (; getline(&line, &sz, F) != -1; free(line), sz = 0) {
        fprintf(stderr, "%s", line);
    }
}

static bool isNullPtr(uintptr_t ptr) { return ptr < NULLPTR_BOUND; }

static void checkNullPtr(uintptr_t ptr, const char *errormsg) {
    if (isNullPtr(ptr)) {
        STAT_INC(NumFatalNullAllocations);
        __mi_fail_wrapper(errormsg, (void *)ptr, NULL);
    }
}

static int getInodeForFile(const char *file) {
    int fd, inode;
    fd = open(file, O_RDONLY);

    if (fd < 0) {
        return -1;
    }

    struct stat file_stat;
    int ret;
    ret = fstat(fd, &file_stat);
    if (ret < 0) {
        return -1;
    }
    inode = file_stat.st_ino;
    close(fd);
    return inode;
}

static bool accessMemoryMap(uintptr_t witness_val) {
#ifdef USE_MEMORY_MAP
    STAT_INC(NumMemMap);
    int own_inode = getInodeForFile(__get_prog_name());
    ASSERTION(own_inode > 0, AL_INPUT)

    char buf[32];
    pid_t pid = getpid();
    snprintf(buf, 32, "/proc/%d/maps", pid);
    FILE *F = fopen(buf, "r");

    char *line;
    size_t sz = 0;

    uintptr_t lower, upper;
    bool success = false;

    for (; getline(&line, &sz, F) != -1; free(line), sz = 0) {
        /* fprintf(stderr, "%s", line); */
        uint64_t inode;

        // format: [lower]-[upper] [permissions] [offset] [device] [inode]
        // [filename]
        if (sscanf(line, "%lx-%lx %*s %*s %*s %lu", &lower, &upper, &inode) !=
            3) {
            continue;
        }

#ifdef USE_MEMORY_MAP_FILEBACKED_ONLY
        if (inode == 0) {
            continue;
        }
#endif
        if ((int)inode == own_inode) {
            continue;
        }

        if (lower <= witness_val && witness_val < upper) {
            /* fprintf(stderr, "Found range [%lx - %lx) for %lx\n", lower,
             * upper, witness_val); */
            STAT_INC(NumMemMapSucc);
            splayInsert(&__memTree, lower, upper, IB_ERROR);
            success = true;
            break;
        }
    }
    free(line);
    fclose(F);
    return success;
#else
    UNUSED(witness_val)
    return false;
#endif
}

/** This function reads the symbol table of the currently executed binary via
 *  nm and inserts the static objects that it finds as globals into the splay
 *  tree.
 *
 *  This is necessary for static values from uninstrumented statically linked
 *  libraries (e.g. builtin typeinfo from the stdc++ lib).
 */
void __splay_insertGlobalsFromBinary(void) {
#ifdef INSERT_GLOBALS_FROM_BINARY
    FILE *fp;
    const char *prefix = "nm -S --defined-only ";
    const char *prog_name = __get_prog_name();
    size_t len = strlen(prog_name);
    len += strlen(prefix);
    len += 1;
    char *cmd = malloc(len * sizeof(char));
    strcpy(cmd, prefix);
    strcat(cmd, prog_name);

    fp = popen(cmd, "r");

    ASSERTION(fp != NULL, AL_INPUT)
    if (fp == NULL) {
        return;
    }

    char *line = NULL;
    size_t sz = 0;

    uint64_t base;
    uint64_t extent;

    for (; getline(&line, &sz, fp) != -1; free(line), sz = 0) {
        /* fprintf(stderr, "%s", line); */

        // format: [address] [size(optional)] [kind] [symbol]
        if (sscanf(line, "%lx %lx", &base, &extent) != 2) {
            continue;
        }
        if (extent <= 0) {
            continue;
        }

        __splay_alloc_or_merge_with_msg((void *)base, extent, "from binary");
    }
    free(line);
    pclose(fp);
    free(cmd);
#endif
}

static Node *getNode(uintptr_t witness_val, const char *str) {
    Node *n = splayFind(&__memTree, witness_val);
    if (n == NULL) {
        if (accessMemoryMap(witness_val)) {
            n = splayFind(&__memTree, witness_val);
            ASSERTION(n != NULL, AL_INPUT)
            return n;
        }
        STAT_INC(NumNoWitness);
#ifdef CRASH_ON_MISSING_WITNESS
        __mi_fail_wrapper(str, (void *)witness_val, NULL);
#else
        UNUSED(str)
#endif
        return NULL;
    }
    return n;
}

void __setup_splay(void) { splayInit(&__memTree); }

#ifdef TREE_ANNOTATE_NODES
static const char *stringForKind(char c) {
    switch (c) {
    case 'g':
        return "global";
    case 'h':
        return "heap";
    case 's':
        return "stack";
    default:
        return "strange";
    }
}
#endif

void __splay_check_inbounds_named(void *witness, void *ptr, char *name) {
    STAT_INC(NumInboundsChecks);
    uintptr_t witness_val = (uintptr_t)witness;
    uintptr_t ptr_val = (uintptr_t)ptr;

    tracerRegisterCheck(ptr_val, ptr_val + 1, name);

    if (isNullPtr(ptr_val)) {
        if (isNullPtr(witness_val)) {
            STAT_INC(NumSuccessfulInboundsChecksNULL);
            return;
        }
        STAT_INC(NumFailedInboundsChecksNULL);
        __mi_fail_wrapper("Outflowing out-of-bounds pointer", (void *)ptr,
                          name);
    }
    Node *n = getNode(witness_val, "Invariant check in unknown allocation");
    if (n == NULL) {
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        // ignore the potential access size here
        STAT_INC(NumFailedInboundsChecksOOB);
        if (name) {
            __mi_fail_wrapper("Outflowing out-of-bounds pointer", (void *)ptr,
                              name);
        } else {
            size_t off = ptr_val - n->base;
            size_t obj_size = n->bound - n->base;
            dumpSplayAllocMapConditionally();
#ifdef TREE_ANNOTATE_NODES
            __mi_fail_fmt(
                stderr,
                "Outflowing out-of-bounds pointer %p with offset %d, "
                "associated to a %s object of size %dB at [%p-%p)",
                ptr, off, stringForKind(n->kind), obj_size, n->base, n->bound);
#else
            __mi_fail_fmt(stderr,
                          "Outflowing out-of-bounds pointer %p with offset "
                          "%d, associated to an object of size %dB at [%p-%p)",
                          ptr, off, obj_size, n->base, n->bound);
#endif
        }
    }
}

void __splay_check_inbounds(void *witness, void *ptr) {
    __splay_check_inbounds_named(witness, ptr, NULL);
}

void __splay_check_dereference_named(void *witness, void *ptr, size_t sz,
                                     char *name) {
    STAT_INC(NumDerefChecks);
    uintptr_t ptr_val = (uintptr_t)ptr;

    tracerRegisterCheck(ptr_val, ptr_val + sz, name);

    if (isNullPtr(ptr_val)) {
        STAT_INC(NumFailedDerefChecksNULL);
        __mi_fail_wrapper("NULL dereference", (void *)ptr, name);
    }
    Node *n =
        getNode((uintptr_t)witness, "Dereference check in unknown allocation");
    if (n == NULL) {
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        STAT_INC(NumFailedDerefChecksOOB);
        size_t off = ptr_val - n->base;
        size_t obj_size = n->bound - n->base;

        dumpSplayAllocMapConditionally();
        if (name) {
#ifdef TREE_ANNOTATE_NODES
            __mi_fail_fmt(stderr,
                          "Out-of-bounds dereference of pointer %p with access "
                          "size %dB to offset %d, associated to a %s object "
                          "of size %dB at [%p-%p):\n'%s'",
                          ptr, sz, off, stringForKind(n->kind), obj_size,
                          n->base, n->bound, name);
#else
            __mi_fail_fmt(stderr,
                          "Out-of-bounds dereference of pointer %p with access "
                          "size %dB to offset %d, associated to an object of "
                          "size %dB at [%p-%p)\n'%s'",
                          ptr, sz, off, obj_size, n->base, n->bound, name);
#endif
        } else {
#ifdef TREE_ANNOTATE_NODES
            __mi_fail_fmt(stderr,
                          "Out-of-bounds dereference of pointer %p with access "
                          "size %dB to offset %d, associated to a %s object "
                          "of size %dB at [%p-%p)",
                          ptr, sz, off, stringForKind(n->kind), obj_size,
                          n->base, n->bound);
#else
            __mi_fail_fmt(
                stderr,
                "Out-of-bounds dereference of pointer %p with access size %dB "
                "to offset %d, associated to an object of size %dB at [%p-%p)",
                ptr, sz, off, obj_size, n->base, n->bound);
#endif
        }
    }
}

void __splay_check_dereference(void *witness, void *ptr, size_t sz) {
    __splay_check_dereference_named(witness, ptr, sz, NULL);
}

#define WIDE_LOWER 0
#define WIDE_UPPER (UINTPTR_MAX >> 1)

// Storage for caching lookups via get_lower/upper
void *__prev_witness = NULL;
bool __prev_available = 0;
uintptr_t __prev_lower = 0;
uintptr_t __prev_upper = 0;

uintptr_t __splay_get_lower(void *witness) {
    STAT_INC(NumGetLower);
    if (isNullPtr((uintptr_t)witness)) {
        return 0;
    }
    if (__prev_available && witness == __prev_witness) {
        uintptr_t t = __prev_lower;
        __prev_lower = 0;
        __prev_upper = 0;
        __prev_available = 0;
        __prev_witness = NULL;
        return t;
    }
    Node *n =
        getNode((uintptr_t)witness, "Taking bounds of unknown allocation");
    __prev_available = 1;
    __prev_witness = witness;
    if (n == NULL) {
        __prev_lower = WIDE_LOWER;
        __prev_upper = WIDE_UPPER;
        return WIDE_LOWER;
    }
    __prev_lower = n->base;
    __prev_upper = n->bound;
    return n->base;
}

void *__splay_get_lower_as_ptr(void *witness) {
    return (void *)__splay_get_lower(witness);
}

void *__splay_get_base(void *witness) {
    return __splay_get_lower_as_ptr(witness);
}

uintptr_t __splay_get_upper(void *witness) {
    STAT_INC(NumGetUpper);
    if (isNullPtr((uintptr_t)witness)) {
        return 0;
    }
    if (__prev_available && witness == __prev_witness) {
        uintptr_t t = __prev_upper;
        __prev_lower = 0;
        __prev_upper = 0;
        __prev_available = 0;
        __prev_witness = NULL;
        return t;
    }
    Node *n =
        getNode((uintptr_t)witness, "Taking bounds of unknown allocation");
    __prev_available = 1;
    __prev_witness = witness;
    if (n == NULL) {
        __prev_lower = WIDE_LOWER;
        __prev_upper = WIDE_UPPER;
        return WIDE_UPPER;
    }
    __prev_lower = n->base;
    __prev_upper = n->bound;
    return n->bound;
}

void *__splay_get_upper_as_ptr(void *witness) {
    return (void *)__splay_get_upper(witness);
}

uintptr_t __splay_get_maxbyteoffset(void *witness) {
    STAT_INC(NumGetUpper);
    if (isNullPtr((uintptr_t)witness)) {
        return 0;
    }
    Node *n =
        getNode((uintptr_t)witness, "Taking bounds of unknown allocation");
    if (n == NULL) {
        return WIDE_UPPER;
    }
    return (n->bound - n->base) - 1;
}

void __splay_alloc_or_merge_with_msg(void *ptr, size_t sz, const char *msg) {
    STAT_INC(NumMergeAllocs);
    uintptr_t val = (uintptr_t)ptr;

    tracerStartOp();

    if (val == (uintptr_t)0) {
        // This means that a global variable is located at a nullptr,
        // apparently this can happen if no definition for a weak symbol exists.
        // We just note this in our statistics and do nothing else.
        STAT_INC(NumGlobalNullAllocations);
        return;
    }

    checkNullPtr(val, "Allocation at nullptr (merge)");
    splayInsert(&__memTree, val, val + sz, IB_EXTEND);

    tracerSetData("global allocation");
    if (msg) {
        tracerSetData(msg);
    }
    tracerEndOp();
}

void __splay_alloc_or_merge(void *ptr, size_t sz) {
    __splay_alloc_or_merge_with_msg(ptr, sz, NULL);
}

void __splay_alloc_with_msg(void *ptr, size_t sz, const char *msg) {
    tracerStartOp();

    STAT_INC(NumStrictAllocs);
    uintptr_t val = (uintptr_t)ptr;
    checkNullPtr(val, "Allocation at nullptr (heap)");
    splayInsert(&__memTree, val, val + sz, IB_ERROR);

    tracerSetData("heap allocation");
    if (msg) {
        tracerSetData(msg);
    }
    tracerEndOp();
}

void __splay_alloc(void *ptr, size_t sz) {
    __splay_alloc_with_msg(ptr, sz, NULL);
}

void __splay_alloc_or_replace_with_msg(void *ptr, size_t sz, const char *msg) {
    tracerStartOp();

    STAT_INC(NumReplaceAllocs);
    uintptr_t val = (uintptr_t)ptr;
    checkNullPtr(val, "Allocation at nullptr (replace)");
    splayInsert(&__memTree, val, val + sz, IB_REPLACE);

    tracerSetData("stack allocation");
    if (msg) {
        tracerSetData(msg);
    }
    tracerEndOp();
}

void __splay_alloc_or_replace(void *ptr, size_t sz) {
    __splay_alloc_or_replace_with_msg(ptr, sz, NULL);
}

void __splay_free_with_msg(void *ptr, const char *msg) {
    tracerStartOp();

    STAT_INC(NumFrees);
    uintptr_t val = (uintptr_t)ptr;
    if (val == 0) {
        return;
    }

    bool success = splayRemove(&__memTree, val);

    tracerSetData("heap free");
    if (msg) {
        tracerSetData(msg);
    }
    tracerEndOp();

    if (!success) {
        STAT_INC(NumDoubleFrees);
#ifdef CHECK_DOUBLE_FREE
        __mi_fail_wrapper("Double free", (void *)val, NULL);
#endif
    }
}

void __splay_free(void *ptr) { __splay_free_with_msg(ptr, NULL); }
