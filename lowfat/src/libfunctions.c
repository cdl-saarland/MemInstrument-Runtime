#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

// for reading symbols from glibc (not portable)
#define __USE_GNUS
#include <dlfcn.h>
#include <errno.h>

#include "config.h"
#include "freelist.h"
#include "statistics.h"

// mutex for locking
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * framework to override standard c functions is taken from Fabian's libfunctions.c in the splay approach
 */
uint64_t __lowfat_ptr_index(void *ptr);

extern size_t sizes[];

// system-dependent, but usually 4KB
uint64_t page_size;

// pointers pointing to the next free memory space for each region
static void *regions[NUM_REGIONS];

// internal structures (i.e free list) use the glibc functions for allocating and freeing, as they don't need runtime checks
// this flag ensures that behavior
static int hooks_active = 0;

typedef int (*start_main_type)(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void (*stack_end));

static start_main_type start_main_found = NULL;


void *__libc_malloc(size_t);
void *__libc_calloc(size_t, size_t);

typedef void *(*malloc_type)(size_t);
static malloc_type malloc_found = __libc_malloc; // malloc is necessary for dlopen

typedef void(*free_type)(void *);
static free_type free_found = NULL;

typedef void *(*calloc_type)(size_t, size_t);
static calloc_type calloc_found = __libc_calloc; // calloc is necessary for dlopen if pthread is included

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
 * if size is contained in sizes, return its index
 * otherwise, returns index of next bigger size contained in sizes
 */
int compute_size_index(size_t size) {
#ifdef POW_2_SIZES
    // get the index of the next higher power of 2 by counting leading zeros
    // Note: this only works, if there are no powers of 2 skipped in sizes!
    int index = 64 - __builtin_clzll(size) - 4; // offset of 4  because the smallest size is 16 Bytes
    if (is_power_of_2(size))
        index--; // corner case if size is already a power 2
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
    // as we want the next bigger size, low is the desired index
    return low;
#endif
}

/**
 * Lowfat allocator
 *
 * @param size the size of the allocation
 * @param alignment alignment requirement for the allocation, must be power of 2 (or 0 if there is no requirement)
 * @return lowfat pointer if possible, NULL if not (e.g. because no space left)
 */
void *lowfat_aligned_alloc(size_t size, size_t alignment) {

    STAT_INC(NumLowFatAllocs);

    if (size == 0)
        return NULL;

    // a pointer is allowed to point to the address right after an array, so we pad the size by 1 to avoid false positives for OOB detection
    // though, if FAST_BASE is enabled, allocations > 2MB are padded anyway, so the 1 byte padding is unnecessary in that case
#ifdef FAST_BASE
    size_t padded_size;
    if (size > FAST_BASE_NO_PAD_LIMIT)
        padded_size = size + page_size;
    else
        padded_size = size + 1;
#else
    size_t padded_size = size + 1;
#endif

    // use fallback allocator for sizes that are too large (-> non low fat pointer)
    if (padded_size > sizes[NUM_REGIONS - 1])
        return NULL;

    int index = compute_size_index(padded_size);

    pthread_mutex_lock(&mutex);

    // first check free list for corresponding region
    // if alignment is required this step is omitted as searching the whole free list for a suitable address might be expensive
    if (!alignment && !free_list_is_empty(index)) {
        STAT_INC(NumFreeListPops);
        void* free_res = free_list_pop(index);
        pthread_mutex_unlock(&mutex);
        return free_res;
    }

    size_t allocation_size = sizes[index];
    void *res = regions[index];

    // for unaligned allocations this loop finishes in the first iteration
    while (1) {

        // check if there is still fresh space left in this region
        if ((uintptr_t) res >= (index + 2) * REGION_SIZE) {
            STAT_INC(NumFullRegionNonFatAllocs);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        if (!alignment || is_aligned((uintptr_t) res, alignment)) {
            // allow read/write on allocated memory (only required for page aligned addresses)
            if ((is_aligned((uintptr_t) res, page_size)))
                mprotect(res, allocation_size, PROT_READ | PROT_WRITE);

#ifdef FAST_BASE
            else {
                // for some sizes, due to alignment requirements res can be located in a new page (with PROT_NONE)
                // so the PROT needs to be adjusted for that page
                // additionally, if the allocation overlaps into more pages, those are covered as well
                uint64_t next_smaller_page_addr = ((uintptr_t) res & ~(page_size - 1)); //fast round down bithack
                size_t prot_change_size = allocation_size + (uintptr_t) res - next_smaller_page_addr;
                mprotect((void *) next_smaller_page_addr, prot_change_size, PROT_READ | PROT_WRITE);
            }
#endif

            regions[index] = res + allocation_size;
            pthread_mutex_unlock(&mutex);
            return res;
        }

        // space is not aligned, add it to the free list
        free_list_push(index, res);

        // check next fresh space slot
        res += allocation_size;

        STAT_INC(NumNonAlignedFreeListAdds);
    }
}

void *lowfat_alloc(size_t size) {
    return lowfat_aligned_alloc(size, 0);
}

void internal_free(void *p) {
    // add freed address to free list for corresponding region
    uintptr_t index = __lowfat_ptr_index(p);
    if (index < NUM_REGIONS) {
        pthread_mutex_lock(&mutex);
        STAT_INC(NumLowFatFrees);
        free_list_push(index, p);
        pthread_mutex_unlock(&mutex);
    }
    else
        free_found(p);
}

void *malloc(size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {

        void *res = lowfat_alloc(size);
        if (res == NULL)
            res = malloc_found(size);

        return res;
    }
    return malloc_found(size);
}

void *calloc(size_t nmemb, size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {
        void *res;

        size_t total_size = nmemb * size;

        int overflow = size != 0 && total_size / size != nmemb;
        if (overflow || total_size > sizes[NUM_REGIONS - 1])
            res = calloc_found(nmemb, size);
        else {
            res = lowfat_alloc(total_size);
            if (res != NULL)
                memset(res, 0, total_size);
            else
                res = calloc_found(nmemb, size);
        }

        return res;
    }
    return calloc_found(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {
        void *res = NULL;

        // if ptr is NULL, simply use malloc
        // otherwise we have 3 cases:

        // 1. ptr is low fat and the new one will be low fat as well -> our malloc, copy, our free
        // 2. ptr is low fat but the new one is not (size too big)   -> glibc malloc, copy, our free
        // Note: copy and free are only done if the allocation succeeded (i.e. errno is 0)

        // 3. ptr is not low fat -> glibc realloc (even if new size would fit into low fat ptr)
        // Note: if ptr isn't low fat, we can't use the low fat allocator for the new pointer in any case
        //       because we don't know how many bytes too copy from ptr, so realloc is the only way here

        if (ptr == NULL)
            res = malloc(size);
        else if (__lowfat_ptr_index(ptr) < NUM_REGIONS) {
            if (size <= sizes[NUM_REGIONS - 1])
                res = lowfat_alloc(size); // case 1

            if (res == NULL)
                res = malloc_found(size); // case 2 (or lowfat_alloc wasn't successful

            if (res != NULL) {
                size_t old_size = sizes[__lowfat_ptr_index(ptr)];
                size_t copy_size = old_size < size ? old_size : size;
                memcpy(res, ptr, copy_size);
                internal_free(ptr);
            }
        }
        else
            res = realloc_found(ptr, size); // case 3

        return res;
    }
    return realloc_found(ptr, size);
}

void *aligned_alloc(size_t alignment, size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {
        void *res;

        if (!is_power_of_2(alignment) || !is_aligned(size, alignment)) {
            errno = EINVAL;
            res = NULL;
        }
        else if (size > sizes[NUM_REGIONS - 1])
            res = aligned_alloc_found(alignment, size);
        else {
            // since size must be a multiple of alignment here, we can simply use the normal allocation routine
            res = lowfat_alloc(size);
            if (res == NULL)
                res = aligned_alloc_found(alignment, size);
        }

        return res;
    }
    return aligned_alloc_found(alignment, size);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {
        int err_status = 0; // 0 for Success
        void *res = NULL;

        // check valid parameters
        if (!is_power_of_2(alignment) || !is_aligned(alignment, sizeof(void *)))
            err_status = EINVAL;
        else if (size > sizes[NUM_REGIONS - 1])
            err_status = posix_memalign(memptr, alignment, size);
        else {
            res = lowfat_aligned_alloc(size, alignment);

            if (res == NULL)
                err_status = posix_memalign(memptr, alignment, size);
            else
                *memptr = res;
        }

        return err_status;
    }
    return posix_memalign_found(memptr, alignment, size);
}

void *memalign(size_t alignment, size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {
        void *res;

        if (!is_power_of_2(alignment)) {
            errno = EINVAL;
            res = NULL;
        }
        else if (size > sizes[NUM_REGIONS - 1])
            res = memalign_found(alignment, size);
        else {
            res = lowfat_aligned_alloc(size, alignment);
            if (res == NULL)
                res = memalign_found(alignment, size);
        }

        return res;
    }
    return memalign_found(alignment, size);
}

void *valloc(size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {
        void *res;

        if (size > sizes[NUM_REGIONS - 1])
            res = valloc_found(size);
        else {
            res = lowfat_aligned_alloc(size, page_size);
            if (res == NULL)
                res = valloc_found(size);
        }

        return res;
    }
    return valloc_found(size);
}

void *pvalloc(size_t size) {
    STAT_INC(NumAllocs);
    if (hooks_active) {
        void *res;

        if (size > sizes[NUM_REGIONS - 1])
            res = pvalloc_found(size);
        else {
            uint64_t rounded_size = (size + page_size - 1) & ~(page_size - 1);
            res = lowfat_alloc(rounded_size);
            if (res == NULL)
                res = pvalloc_found(size);
        }

        return res;
    }
    return pvalloc_found(size);
}

void free(void *p) {
    STAT_INC(NumFrees);
    if (hooks_active) {

        if (p != NULL)
            internal_free(p);

        return;
    }
    free_found(p);
}

#if ENABLE_MPX
void enable_mpx(void);
#endif

#if defined(FAST_BASE) && !defined(POW_2_SIZES)
void init_inv_sizes(void);
#endif

int
__libc_start_main(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void (*stack_end)) {

    // create regions for each size
    for (unsigned i = 0; i < NUM_REGIONS; i++) {
        uintptr_t region_address = (i + 1) * REGION_SIZE;
        regions[i] = mmap((void *) region_address, REGION_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);

        // check that mmap mapped the desired address (otherwise we might get false positive later for the safety checks)
        if ((uintptr_t) regions[i] % REGION_SIZE != 0)
            exit(99); // TODO more info than just exotic return code
    }
    
    // get original functions from dynamic linker
    initDynamicFunctions();

    page_size = sysconf(_SC_PAGESIZE);

#if defined(FAST_BASE) && !defined(POW_2_SIZES)
    init_inv_sizes();
#endif

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
