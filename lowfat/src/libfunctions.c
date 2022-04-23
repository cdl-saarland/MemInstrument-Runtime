#include "LFSizes.h"
#include "fail_function.h"
#include "freelist.h"
#include "statistics.h"

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// for reading symbols from glibc (not portable)
#include <dlfcn.h>

// mutex for locking
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * framework to override standard c functions is taken from Fabian's
 * libfunctions.c in the splay approach
 */
uint64_t __lowfat_ptr_index(void *ptr);

// system-dependent, but often 4KB
uint64_t page_size;

// pointers pointing to the next free memory space for each region
static void *regions[NUM_REGIONS];
static void *stack_regions[NUM_REGIONS];

// Internal structures (i.e. the free list) use the glibc functions for
// allocating and freeing, as they don't need runtime checks. This flag ensures
// that behavior. The flag is active by default, so new threads can use the low
// fat allocator.
_Thread_local static int hooks_active = 1;

typedef int (*start_main_type)(int *(main)(int, char **, char **), int argc,
                               char **ubp_av, void (*init)(void),
                               void (*fini)(void), void (*rtld_fini)(void),
                               void(*stack_end));

static start_main_type start_main_found = NULL;

void *__libc_malloc(size_t);
void *__libc_calloc(size_t, size_t);

typedef void *(*malloc_type)(size_t);
static malloc_type malloc_found =
    __libc_malloc; // malloc is necessary for dlopen

typedef void (*free_type)(void *);
static free_type free_found = NULL;

typedef void *(*calloc_type)(size_t, size_t);
static calloc_type calloc_found =
    __libc_calloc; // calloc is necessary for dlopen if pthread is included

typedef void *(*realloc_type)(void *, size_t);
static realloc_type realloc_found = NULL;

typedef void *(*aligned_alloc_type)(size_t, size_t);
static aligned_alloc_type aligned_alloc_found = NULL;

typedef int (*posix_memalign_type)(void **, size_t, size_t);
static posix_memalign_type posix_memalign_found = NULL;

typedef void *(*memalign_type)(size_t, size_t);
static memalign_type memalign_found = NULL;

typedef void *(*valloc_type)(size_t);
static valloc_type valloc_found = NULL;

typedef void *(*pvalloc_type)(size_t);
static pvalloc_type pvalloc_found = NULL;

void initDynamicFunctions(void) {
    const char *libname = "libc.so.6";
    void *handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    char *msg = NULL;
    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error loading libc:\n%s\n", msg);
        exit(74);
    }

    start_main_found = (start_main_type)dlsym(handle, "__libc_start_main");
    malloc_found = (malloc_type)dlsym(handle, "malloc");
    free_found = (free_type)dlsym(handle, "free");
    calloc_found = (calloc_type)dlsym(handle, "calloc");
    realloc_found = (realloc_type)dlsym(handle, "realloc");
    aligned_alloc_found = (aligned_alloc_type)dlsym(handle, "aligned_alloc");
    posix_memalign_found = (posix_memalign_type)dlsym(handle, "posix_memalign");
    memalign_found = (memalign_type)dlsym(handle, "memalign");
    valloc_found = (valloc_type)dlsym(handle, "valloc");
    pvalloc_found = (pvalloc_type)dlsym(handle, "pvalloc");

    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error finding libc symbols:\n%s\n",
                msg);
        exit(74);
    }
}

/* alignment must be a power of 2
 * returns 1 if value is a multiple of alignment, otherwise 0
 */
int is_aligned(uint64_t value, uint64_t alignment) {
    return (value & (alignment - 1)) == 0;
}

/*
 * returns 1 if value is a power of 2, otherwise 0
 */
int is_power_of_2(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

/*
 * returns the index of the low-fat region that is appropriate for size
 */
unsigned compute_size_index(size_t size) {
    // get the index of the next higher power of 2 by counting leading zeros
    // Note: this only works, if there are no powers of 2 skipped
    int index = 64 - __builtin_clzll(size) - MIN_ALLOC_SIZE_LOG;
    if (is_power_of_2(size))
        index--; // corner case if size is already a power 2
    return index < 0
               ? 0
               : (unsigned)
                     index; // sizes 1, 2, 4 and 8 are rounded up to 16 bytes
}

/**
 * Lowfat allocator
 *
 * @param size the size of the allocation
 * @param alignment alignment requirement for the allocation, must be power of 2
 * (or 0 if there is no requirement)
 * @return lowfat pointer if possible, NULL if not (e.g. because no space left)
 */
void *lowfat_aligned_alloc(size_t size, size_t alignment) {

    if (size == 0) {
        STAT_INC(NumSizeZeroAllocs);
        return NULL;
    }

    STAT_INC(NumLowFatAllocs);

    // a pointer is allowed to point to the address right after an array, so we
    // pad the size by 1 to avoid false positives for OOB detection
    size_t padded_size = size + 1;

    // use fallback allocator for sizes that are too large (-> non low fat
    // pointer)
    if (padded_size > MAX_HEAP_ALLOC_SIZE) {
        STAT_INC(NumTooLargeNonFatAllocs);
        return NULL;
    }

    unsigned index = compute_size_index(padded_size);

    pthread_mutex_lock(&mutex);

    // first check free list for corresponding region
    // if alignment is required this step is omitted as searching the whole free
    // list for a suitable address might be expensive
    if (!alignment && !free_list_is_empty(index)) {
        STAT_INC(NumFreeListPops);
        void *free_res = free_list_pop(index);
        pthread_mutex_unlock(&mutex);
        return free_res;
    }

    size_t allocation_size = MIN_ALLOC_SIZE << index;
    void *res = regions[index];

    // for unaligned allocations this loop finishes in the first iteration
    while (1) {

        // check if there is still fresh space left in this region
        if ((uintptr_t)res >= (index + 2) * REGION_SIZE) {
            STAT_INC(NumFullRegionNonFatAllocs);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        if (!alignment || is_aligned((uintptr_t)res, alignment)) {
            // allow read/write on allocated memory (only required for page
            // aligned addresses)
            if ((is_aligned((uintptr_t)res, page_size)))
                mprotect(res, allocation_size, PROT_READ | PROT_WRITE);

            regions[index] = res + allocation_size;
            pthread_mutex_unlock(&mutex);
            return res;
        }

        // space is not aligned, add it to the free list
        free_list_push(index, res);
        STAT_INC(NumNonAlignedFreeListAdds);

        // check next fresh space slot
        res += allocation_size;
    }
}

void *lowfat_alloc(size_t size) { return lowfat_aligned_alloc(size, 0); }

void internal_free(void *p) {
    // add freed address to free list for corresponding region
    uint64_t index = __lowfat_ptr_index(p);
    if (index < NUM_REGIONS) {
        pthread_mutex_lock(&mutex);
        STAT_INC(NumLowFatFrees);
        free_list_push(index, p);
        pthread_mutex_unlock(&mutex);
    } else
        free_found(p);
}

void *malloc(size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumMalloc);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return malloc_found(size);
    }

    hooks_active = 0;

    void *res = lowfat_alloc(size);
    if (res == NULL)
        res = malloc_found(size);

    hooks_active = 1;
    return res;
}

void *calloc(size_t nmemb, size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumCalloc);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return calloc_found(nmemb, size);
    }

    hooks_active = 0;
    void *res;

    size_t total_size = nmemb * size;

    int overflow = size != 0 && total_size / size != nmemb;
    if (overflow) {
        STAT_INC(NumOverflowingCallocs);
        res = calloc_found(nmemb, size);
    } else {
        res = lowfat_alloc(total_size);
        if (res != NULL)
            memset(res, 0, total_size);
        else
            res = calloc_found(nmemb, size);
    }

    hooks_active = 1;
    return res;
}

void *realloc(void *ptr, size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumRealloc);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return realloc_found(ptr, size);
    }

    // if ptr is NULL, simply use malloc
    if (ptr == NULL) {
        STAT_INC(NumNullRealloc);
        return malloc(size);
    }

    // otherwise we have 3 cases:

    // 1. ptr is low fat and the new one will be low fat as well -> our
    // malloc, copy, our free
    // 2. ptr is low fat but the new one is not (size too big)   -> glibc
    // malloc, copy, our free Note: copy and free are only done if the
    // allocation succeeded (i.e. errno is 0)
    // 3. ptr is not low fat -> glibc realloc (even if new size would fit
    // into low fat ptr)
    // Note for 3: if ptr isn't low fat, we can't use the low fat allocator for
    // the new pointer in any case because we don't know how many bytes to copy
    // from ptr, so glibc_realloc is the only way here
    hooks_active = 0;
    void *res = NULL;
    if (__lowfat_ptr_index(ptr) < NUM_REGIONS) {
        res = lowfat_alloc(size); // case 1

        if (res == NULL)
            res = malloc_found(
                size); // case 2 (or lowfat_alloc wasn't successful)

        if (res != NULL) {
            size_t old_size = MIN_ALLOC_SIZE << __lowfat_ptr_index(ptr);
            size_t copy_size = old_size < size ? old_size : size;
            memcpy(res, ptr, copy_size);
            internal_free(ptr);
        }
    } else {
        res = realloc_found(ptr, size); // case 3
    }

    hooks_active = 1;
    return res;
}

void *aligned_alloc(size_t alignment, size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumAlignedAlloc);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return aligned_alloc_found(alignment, size);
    }

    hooks_active = 0;
    void *res;

    if (!is_power_of_2(alignment) || !is_aligned(size, alignment)) {
        STAT_INC(NumNonPowTwoAllocs);
        errno = EINVAL;
        res = NULL;
    } else {
        // since size must be a multiple of alignment here, we can simply
        // use the normal allocation routine
        res = lowfat_alloc(size);
        if (res == NULL)
            res = aligned_alloc_found(alignment, size);
    }

    hooks_active = 1;
    return res;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumPosixMemAlign);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return posix_memalign_found(memptr, alignment, size);
    }

    hooks_active = 0;
    int err_status = 0; // 0 for Success
    void *res = NULL;

    // check valid parameters
    if (!is_power_of_2(alignment) || !is_aligned(alignment, sizeof(void *))) {
        STAT_INC(NumNonPowTwoAllocs);
        err_status = EINVAL;
    } else {
        res = lowfat_aligned_alloc(size, alignment);

        if (res == NULL)
            err_status = posix_memalign(memptr, alignment, size);
        else
            *memptr = res;
    }

    hooks_active = 1;
    return err_status;
}

void *memalign(size_t alignment, size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumMemAlign);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return memalign_found(alignment, size);
    }

    hooks_active = 0;
    void *res;

    if (!is_power_of_2(alignment)) {
        STAT_INC(NumNonPowTwoAllocs);
        errno = EINVAL;
        res = NULL;
    } else {
        res = lowfat_aligned_alloc(size, alignment);
        if (res == NULL)
            res = memalign_found(alignment, size);
    }

    hooks_active = 1;
    return res;
}

void *valloc(size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumValloc);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return valloc_found(size);
    }

    hooks_active = 0;
    void *res;

    res = lowfat_aligned_alloc(size, page_size);
    if (res == NULL)
        res = valloc_found(size);

    hooks_active = 1;
    return res;
}

void *pvalloc(size_t size) {
    STAT_INC(NumAllocs);
    STAT_INC(NumPValloc);
    if (!hooks_active) {
        STAT_INC(NumHookDisabled);
        return pvalloc_found(size);
    }

    hooks_active = 0;
    void *res;

    size_t rounded_size = (size + page_size - 1) & ~(page_size - 1);
    res = lowfat_alloc(rounded_size);
    if (res == NULL)
        res = pvalloc_found(size);

    hooks_active = 1;
    return res;
}

void free(void *p) {
    STAT_INC(NumFrees);
    if (!hooks_active) {
        free_found(p);
        return;
    }

    hooks_active = 0;

    if (p != NULL)
        internal_free(p);

    hooks_active = 1;
    return;
}

/// A pointer to the environment variables, defined in unistd.h.
extern char **environ;

/// Move the program stack to a different location
void *__lowfat_move_stack(void *stack_current) {
    char **env_trav = environ;

    // Find the beginning of the program stack.
    // We are searching for the highest address of any environment variable
    // string, not just the first pointer to the enviornment variables (which
    // would be the address just before the NULL that marks the end of the
    // environment variable pointers).
    char *stack_begin = *env_trav;
    while (*env_trav != NULL) {
        // Compute the highst char of the string (len + nul terminator)
        char *end_of_env_var = *env_trav + strlen(*env_trav) + 1;
        if (end_of_env_var > stack_begin) {
            stack_begin = end_of_env_var;
        }
        env_trav++;
    }
    if (stack_begin < (char *)env_trav) {
        puts("Highest address is smaller than the env pointer\n");
        exit(1);
    }

    // Align to page size
    unsigned long not_aligned = (uintptr_t)stack_begin % page_size;
    if (not_aligned) {
        stack_begin = stack_begin + page_size - not_aligned;
    }
    ptrdiff_t stack_size = stack_begin - (char *)stack_current;

    if ((unsigned long long)stack_size > STACK_SIZE) {
        printf("LF: Program stack larger than expected (was %lu, allowed %llu)",
               stack_size, STACK_SIZE);
        exit(1);
    }

    // Location where to allocate the stack
    uintptr_t new_stack_base =
        BASE_STACK_REGION_NUM * REGION_SIZE + STACK_REGION_OFFSET;
    // Allocate the new stack
    void *allocated_new_stack =
        mmap((void *)new_stack_base, STACK_SIZE, PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);

    if ((uintptr_t)allocated_new_stack != new_stack_base) {
        printf("LF: Failed to allocate the new stack\n Tried %p (got %p)",
               (void *)new_stack_base, allocated_new_stack);
        exit(1);
    }

    void *new_copied_top = allocated_new_stack + STACK_SIZE - stack_size;
    // Copy the old stack to the new one
    memcpy(new_copied_top, stack_current, stack_size);

    // Set environ to the new location of the environment variables
    // Compute the offset they previously had to the beginning of the stack and
    // subtract it from the beginning of the stack
    // TODO Not sure if this is actually necessary, as the environment variables
    // are not protected anyway.
    environ = allocated_new_stack + STACK_SIZE -
              ((void *)stack_begin - (void *)environ);

    // Make sure that pointers on the stack pointing to the stack are updated.
    // Traverse the new stack and search for old addresses.
    // FIXME This is an awful hack, as it can also overwrite values on the stack
    // that look like pointers, but are not actually pointers...
    for (void **next = (void **)new_copied_top;
         next < (void **)new_copied_top - stack_size; next++) {

        // Check if the pointer points to the stack
        if ((uintptr_t)*next < (uintptr_t)stack_begin &&
            (uintptr_t)stack_current <= (uintptr_t)*next) {
            // Overrite it with the location on the new stack
            ptrdiff_t offset = *next - stack_current;
            *next = (void *)((uintptr_t)new_copied_top + offset);
        }
    }

    return new_copied_top;
}

__asm__("\t.align 16, 0x90\n"
        "\t.type __lowfat_move_stack_trampoline,@function\n"
        "__lowfat_move_stack_trampoline:\n"
        "\tmovq %rsp, %rdi\n"
        "\tmovabsq $__lowfat_move_stack, %rax\n"
        "\tcallq *%rax\n"
        "\tmovq %rax, %rsp\n"
        "\tretq\n");

void __lowfat_move_stack_trampoline(void);

int __libc_start_main(int *(main)(int, char **, char **), int argc,
                      char **ubp_av, void (*init)(void), void (*fini)(void),
                      void (*rtld_fini)(void), void(*stack_end)) {
    // for program initialization we can't use the low fat allocator
    hooks_active = 0;

    // get original functions from dynamic linker
    initDynamicFunctions();

    // Ignore llvm tooling functions
    if (__is_llvm_tooling_function(ubp_av[0])) {
        return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini,
                                   stack_end);
    }

    page_size = sysconf(_SC_PAGESIZE);

    // set up statistics counters etc.
    __setup_statistics(ubp_av[0]);

    // Create heap regions for each size
    for (unsigned i = 0; i < NUM_REGIONS; i++) {
        uintptr_t region_address = (i + 1) * REGION_SIZE + HEAP_REGION_OFFSET;
        regions[i] = mmap((void *)region_address, HEAP_REGION_SIZE,
                          PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);

        // Check that mmap mapped the desired addresses (otherwise we might get
        // false positive later for the safety checks)
        if ((uintptr_t)regions[i] != region_address) {
            __mi_printf("LF: Failed to mmap the required memory location "
                        "(heap)!\n\tWanted: %p\n\tGot: %p\n",
                        (void *)region_address, regions[i]);
            exit(99);
        }
        __mi_debug_printf("Allocated heap region %p (num %d size %llu)\n",
                          (void *)region_address, i, HEAP_REGION_SIZE);
    }

    // Create stack regions for each size
    for (unsigned i = 0; i < NUM_REGIONS; i++) {
        uintptr_t region_address = (i + 1) * REGION_SIZE + STACK_REGION_OFFSET;
        stack_regions[i] = mmap(
            (void *)region_address, STACK_REGION_SIZE, PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);

        // Make sure we mmaped the requested region
        if ((uintptr_t)stack_regions[i] != region_address) {
            __mi_printf("LF: Failed to mmap the required memory location "
                        "(stack)!\n\tWanted: %p\n\tGot: %p\n",
                        (void *)region_address, stack_regions[i]);
            exit(99);
        }
        __mi_debug_printf("Allocated stack region %p (num %d size %llu)\n",
                          (void *)region_address, i, STACK_REGION_SIZE);
    }
    __mi_debug_printf("Allocated the stack\n");

    // Move the stack
    __lowfat_move_stack_trampoline();

    // enable our malloc etc. replacements
    hooks_active = 1;

    // call the actual main function
    return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini,
                               stack_end);
}
