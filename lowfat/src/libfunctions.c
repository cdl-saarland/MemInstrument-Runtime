#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

// for reading symbols from glibc (not portable)
#define __USE_GNUS
#include <dlfcn.h>
#include <errno.h>

#include "freelist.h"
#include "statistics.h"
#include "config.h"

/*
 * framework to override standard c functions is taken from Fabian's libfunctions.c in the splay approach
 */

uintptr_t _ptr_index(void *ptr);

// supported object sizes for region based heap allocation (bigger size use original glibc functions)
// TODO make this easier to configure
size_t sizes[NUM_REGIONS] = {16, 32, 64, 128};

// pointers pointing to the next free memory space for each region
static void *regions[NUM_REGIONS];

// internal structures (i.e free list) use the glibc functions for allocating and freeing, as they don't need runtime checks
// this flag ensures that behavior
static int hooks_active = 0;

typedef int (*start_main_type)(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void),
                               void (*fini)(void), void (*rtld_fini)(void), void (*stack_end));

static start_main_type start_main_found = NULL;


void *__libc_malloc(size_t);

typedef void *(*malloc_type)(size_t);
static malloc_type malloc_found = __libc_malloc;

typedef void(*free_type)(void *);
static free_type free_found = NULL;

typedef void *(*calloc_type)(size_t, size_t);
static calloc_type calloc_found = NULL;

typedef void *(*realloc_type)(void *, size_t);
static realloc_type realloc_found = NULL;

typedef void *(*aligned_alloc_type)(size_t, size_t);
static aligned_alloc_type aligned_alloc_found = NULL;

typedef int (*posix_memalign_type)(void **, size_t, size_t);
static posix_memalign_type posix_memalign_found = NULL;

typedef void *(*memalign_type)(size_t, size_t);
static memalign_type memalign_found = NULL;

typedef void*(*valloc_type)(size_t);
static valloc_type valloc_found = NULL;

typedef void*(*pvalloc_type)(size_t);
static pvalloc_type pvalloc_found = NULL;

void initDynamicFunctions(void) {
    const char *libname = "libc.so.6";
    void *handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    char *msg = NULL;
    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error loading libc:\n%s\n", msg);
        exit(74);
    }

    start_main_found = (start_main_type) dlsym(handle, "__libc_start_main");
    malloc_found = (malloc_type) dlsym(handle, "malloc");
    free_found = (free_type) dlsym(handle, "free");
    calloc_found = (calloc_type) dlsym(handle, "calloc");
    realloc_found = (realloc_type) dlsym(handle, "realloc");
    aligned_alloc_found = (aligned_alloc_type) dlsym(handle, "aligned_alloc");
    posix_memalign_found = (posix_memalign_type) dlsym(handle, "posix_memalign");
    memalign_found = (memalign_type) dlsym(handle, "memalign");
    valloc_found = (valloc_type) dlsym(handle, "valloc");
    pvalloc_found = (pvalloc_type) dlsym(handle, "pvalloc");

    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error finding libc symbols:\n%s\n", msg);
        exit(74);
    }
}

/* alignment must be a power of 2
 * returns 1 if value is a multiple of alignment, otherwise 0
 */
int is_aligned(unsigned long value, unsigned long alignment) {
    return (value & (alignment - 1)) == 0;
}

/*
 * returns 1 if value is a power of 2, otherwise 0
 */
int is_power_of_2(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

/*
 * if size is contained in sizes, return its index
 * otherwise, returns index of next bigger size contained in sizes
 */
int compute_size_index(size_t size) {
#ifdef POW_2_SIZES
    // round up to next higher power of 2 by counting leading zeros
    // TODO this only works, if there are no powers of 2 skipped in sizes!
    int index = 64 - __builtin_clzll(size) - 5; // -5 because the smallest size is 16 Bytes
    return index < 0 ? 0 : index; // sizes 1, 2, 4 and 8 are rounded up to 16 bytes
#else
    // binary search over sizes to find next bigger (or equal) size
    int low = 0, high = NUM_REGIONS - 1;
    int mid;
    while (low <= high) {
        mid = (low + high) / 2;
        if (size == sizes[mid])
            return mid;
        else if (size < sizes[mid])
            high = mid - 1;
        else
            low = mid + 1;
    }
    // if low > high, then the size lies between size[low] and size[high] and low = high + 1 at this point
    // as we want to next higher value, low is the desired index
    return low;
#endif
}

void *internal_allocation(size_t size) {
    // use glibc malloc for sizes that are too large (-> non low fat pointer)
    if (size > sizes[NUM_REGIONS - 1])
        return malloc_found(size);

    int index = compute_size_index(size);

    // first check free list for corresponding region
    if (!free_list_is_empty(index))
        return free_list_pop(index);

    // otherwise use fresh space (if available)
    size_t allocation_size = sizes[index];

    void *res = regions[index];

    // if no more fresh space left, fallback to glibc malloc TODO nicer way to check this?
    if ((uintptr_t) res >= (index + 2) * REGION_SIZE)
        return malloc(size);

    // increase region pointer to point to fresh space for next allocation
    regions[index] += allocation_size;

    // allow read/write on allocated memory (only required for page aligned addresses)
    if (is_aligned((uintptr_t) res, sysconf(_SC_PAGESIZE)))
        mprotect(res, allocation_size, PROT_READ | PROT_WRITE);

    return res;

}

/* difference to internal_allocation:
 - doesn't check free list, as searching an aligned element might iterate over the whole list
 - increases fresh space pointer until space with correct alignment is found, all space until then is added to free list
 */
void *internal_aligned_allocation(size_t size, size_t alignment) {
    // use glibc malloc for sizes that are too large (-> non low fat pointer)
    if (size > sizes[NUM_REGIONS - 1])
        return malloc_found(size);

    int index = compute_size_index(size);

    // otherwise use fresh space (if available)
    size_t allocation_size = sizes[index];

    void *res = regions[index];

    while (1) {

        // if no more fresh space left, fallback to glibc malloc TODO nicer way to check this?
        if ((uintptr_t) res >= (index + 2) * REGION_SIZE)
            return malloc(size);

        if (is_aligned(res, alignment)) {
            // allow read/write on allocated memory (only required for page aligned addresses)
            if ((is_aligned(res, sysconf(_SC_PAGESIZE))))
                mprotect(res, allocation_size, PROT_READ | PROT_WRITE);

            regions[index] = res;
            return res;
        }

        // space is not aligned, add it to the free list
        free_list_push(index, res);

        // check next fresh space slot
        res += allocation_size;
    }
}

void internal_free(void *p) {
    // add freed address to free list for corresponding region
    uintptr_t index = _ptr_index(p);
    if (index < NUM_REGIONS)
        free_list_push(index, p);
    else
        free_found(p);
}

void *malloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res = internal_allocation(size);

        hooks_active = 1;
        return res;
    }
    return malloc_found(size);
}

void *calloc(size_t nmemb, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res;
        // TODO check for multiplication overflow
        size_t total_size = nmemb * size;
        if (total_size > sizes[NUM_REGIONS - 1])
            res = calloc_found(nmemb, size);
        else {
            res = internal_allocation(total_size);
            memset(res, 0, total_size);
        }

        hooks_active = 1;
        return res;
    }
    return calloc_found(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res;

        // we have 4 cases:
        // 1. ptr is low fat and the new one will be low fat as well
        // 2. ptr is low fat but the new one is not (size too big)
        // 3. ptr is not low fat but the new one will be
        // 4. ptr is not low fat and the new one won't be either
        // case 1 and 2 require new allocation with our/glibc malloc, copy and our free
        // case 3 is like case 1 but uses glibc free
        // case 4 can simply use glibc realloc

        if (_ptr_index(ptr) < NUM_REGIONS) {
            if (size <= sizes[NUM_REGIONS - 1])
                res = internal_allocation(size); // case 1
            else
                res = malloc_found(size); // case 2

            if (ptr != NULL) {
                memcpy(res, ptr, size);
                internal_free(ptr);
            }

        } else {
            if (size <= sizes[NUM_REGIONS - 1]) {
                res = internal_allocation(size); // case 3

                if (ptr != NULL) {
                    memcpy(res, ptr, size);
                    free_found(ptr);
                }

            } else
                res = realloc_found(ptr, size); // case 4
        }

        hooks_active = 1;
        return res;
    }
    return realloc_found(ptr, size);
}

void *aligned_alloc(size_t alignment, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        if (!is_power_of_2(alignment) || !is_aligned(size, alignment)) {
            errno = EINVAL;
            return NULL;
        }

        if (size > sizes[NUM_REGIONS - 1])
            return aligned_alloc_found(alignment, size);

        // since size must be a multiple of alignment here, we can simply use the normal allocation routine
        void *res = internal_allocation(size);

        hooks_active = 1;
        return res;
    }
    return aligned_alloc_found(alignment, size);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        // check valid parameters
        if (!is_power_of_2(alignment) || !is_aligned(size, sizeof(void *)))
            return EINVAL;

        if (size > sizes[NUM_REGIONS - 1])
            return posix_memalign(memptr, alignment, size);

        *memptr = internal_aligned_allocation(size, alignment);

        if (*memptr == NULL)
            return ENOMEM;

        hooks_active = 1;
        return 0;
    }
    return posix_memalign_found(memptr, alignment, size);
}

void *memalign(size_t alignment, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        if (!is_power_of_2(alignment)) {
            errno = EINVAL;
            return NULL;
        }

        if (size > sizes[NUM_REGIONS - 1])
            return memalign_found(alignment, size);

        void *res = internal_aligned_allocation(size, alignment);

        hooks_active = 1;
        return res;
    }
    return memalign_found(alignment, size);
}

void *valloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        if (size > sizes[NUM_REGIONS - 1])
            return valloc_found(size);

        void *res = internal_aligned_allocation(size, sysconf(_SC_PAGESIZE));

        hooks_active = 1;
        return res;
    }
    return valloc_found(size);
}

void *pvalloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        if (size > sizes[NUM_REGIONS - 1])
            return pvalloc_found(size);

        long page_size = sysconf(_SC_PAGESIZE);
        // round size to next greater multiple of page size;
        long rounded_size = (size + page_size-1) & ~(page_size-1);
        void *res = internal_allocation(rounded_size);

        hooks_active = 1;
        return res;
    }
    return pvalloc_found(size);
}

void free(void *p) {
    if (hooks_active) {
        hooks_active = 0;

        internal_free(p);

        hooks_active = 1;
        return;
    }
    free_found(p);
}

#if ENABLE_MPX
void enable_mpx(void);
#endif

int
__libc_start_main(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void (*stack_end)) {

    // create regions for each size
    for (int i = 0; i < NUM_REGIONS; i++) {
        uintptr_t region_address = (i + 1) * REGION_SIZE;
        regions[i] = mmap((void *) region_address, REGION_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);

        // check that mmap mapped the desired address (otherwise we might get false positive later for the safety checks)
        if ((uintptr_t) regions[i] % REGION_SIZE != 0)
            exit(99); // TODO more info than just exotic return code
    }

    // get original functions from dynamic linker
    initDynamicFunctions();

    // set up statistics counters etc.
    __setup_statistics(ubp_av[0]);

#if ENABLE_MPX
    // call magic code to enable the intel mpx extension
    enable_mpx();
#endif

    // enable our malloc etc. replacements
    hooks_active = 1;

    // call the actual main function
    return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}
