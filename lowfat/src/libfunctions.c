#include "LFSizes.h"
#include "core.h"
#include "fail_function.h"
#include "freelist.h"
#include "lowfat_defines.h"
#include "statistics.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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

// system-dependent, but often 4KB
uint64_t __lowfat_page_size;

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

/**
 * Lowfat allocator
 *
 * @param size the size of the allocation
 * @param alignment alignment requirement for the allocation, must be power of 2
 * (or 0 if there is no requirement)
 * @return lowfat pointer if possible, NULL if not (e.g. because no space left)
 */
static void *lowfat_aligned_alloc(size_t size, size_t alignment) {

    if (size == 0) {
        STAT_INC(NumSizeZeroAllocs);
        return NULL;
    }

    // a pointer is allowed to point to the address right after an array, so we
    // pad the size by 1 to avoid false positives for OOB detection
    size_t padded_size = size + 1;

    // use fallback allocator for sizes that are too large (-> non low fat
    // pointer)
    if (padded_size > MAX_HEAP_ALLOC_SIZE) {
        STAT_INC(NumTooLargeNonFatAllocs);
        return NULL;
    }

    unsigned region_index = __lowfat_index_for_size(padded_size);
    unsigned zero_based_index = __lowfat_get_zero_based_index(region_index);

    pthread_mutex_lock(&mutex);

    // first check free list for corresponding region
    // if alignment is required this step is omitted as searching the whole free
    // list for a suitable address might be expensive
    if (!alignment && !__lowfat_free_list_is_empty(zero_based_index)) {
        STAT_INC(NumFreeListPops);
        void *free_res = __lowfat_free_list_pop(zero_based_index);
        pthread_mutex_unlock(&mutex);
        STAT_INC(NumLowFatAllocs);
        return free_res;
    }

    size_t allocation_size = __lowfat_ptr_size(region_index);
    __mi_debug_printf("Incoming size: %d\nSize to be allocated: %d\nRegion "
                      "index: %d\nZero-based region index: %d\n",
                      size, allocation_size, region_index, zero_based_index);
    void *res = regions[zero_based_index];

    // for unaligned allocations this loop finishes in the first iteration
    while (1) {

        // check if there is still fresh space left in this region
        if ((uintptr_t)res >= (zero_based_index + 2) * REGION_SIZE) {
            STAT_INC(NumFullRegionNonFatAllocs);
            pthread_mutex_unlock(&mutex);
            __mi_debug_printf("Region %d is full, using fall-back allocator\n",
                              region_index);
            return NULL;
        }

        if (!alignment || __lowfat_is_aligned((uintptr_t)res, alignment)) {
            // allow read/write on allocated memory (only required for page
            // aligned addresses)
            if ((__lowfat_is_aligned((uintptr_t)res, __lowfat_page_size)))
                mprotect(res, allocation_size, PROT_READ | PROT_WRITE);

            regions[zero_based_index] = res + allocation_size;
            pthread_mutex_unlock(&mutex);
            __mi_debug_printf("Allocated address: %p\n", res);
            STAT_INC(NumLowFatAllocs);
            return res;
        }

        // space is not aligned, add it to the free list
        __lowfat_free_list_push(zero_based_index, res);
        STAT_INC(NumNonAlignedFreeListAdds);

        // check next fresh space slot
        res += allocation_size;
    }
}

static void *lowfat_alloc(size_t size) { return lowfat_aligned_alloc(size, 0); }

static void internal_free(void *p) {
    // add freed address to free list for corresponding region
    if (__is_lowfat(p)) {
        pthread_mutex_lock(&mutex);
        STAT_INC(NumLowFatFrees);
        __lowfat_free_list_push(
            __lowfat_get_zero_based_index(__lowfat_ptr_index(p)), p);
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
    if (__is_lowfat(ptr)) {
        res = lowfat_alloc(size); // case 1

        if (res == NULL)
            res = malloc_found(
                size); // case 2 (or lowfat_alloc wasn't successful)

        if (res != NULL) {
            size_t old_size = __lowfat_ptr_size(__lowfat_ptr_index(ptr));
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

    if (!__lowfat_is_power_of_2(alignment) ||
        !__lowfat_is_aligned(size, alignment)) {
        STAT_INC(NumNonPowTwoAllocs);
        errno = EINVAL;
        res = NULL;
    } else {
        // since size must be a multiple of alignment here, we can simply
        // use the normal allocation routine
        res = lowfat_alloc(size);
        if (res == NULL) {
            res = aligned_alloc_found(alignment, size);
        }
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
    if (!__lowfat_is_power_of_2(alignment) ||
        !__lowfat_is_aligned(alignment, sizeof(void *))) {
        STAT_INC(NumNonPowTwoAllocs);
        err_status = EINVAL;
    } else {
        res = lowfat_aligned_alloc(size, alignment);

        if (res == NULL) {
            err_status = posix_memalign(memptr, alignment, size);
        } else {
            *memptr = res;
        }
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

    if (!__lowfat_is_power_of_2(alignment)) {
        STAT_INC(NumNonPowTwoAllocs);
        errno = EINVAL;
        res = NULL;
    } else {
        res = lowfat_aligned_alloc(size, alignment);
        if (res == NULL) {
            res = memalign_found(alignment, size);
        }
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

    res = lowfat_aligned_alloc(size, __lowfat_page_size);
    if (res == NULL) {
        res = valloc_found(size);
    }

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

    size_t rounded_size =
        (size + __lowfat_page_size - 1) & ~(__lowfat_page_size - 1);
    res = lowfat_alloc(rounded_size);
    if (res == NULL) {
        res = pvalloc_found(size);
    }

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

    if (p != NULL) {
        internal_free(p);
    }

    hooks_active = 1;
    return;
}

// Create a file descriptor to a file with the given name and size.
static int __lowfat_create_file(char *name, size_t size) {

    // Generate a full path containing the given name
    char *start = "/dev/shm/lf.";
    // Use the process/thread id to avoid conflicts with other instances running
    // in parallel.
    pid_t current_pid = getpid();
    pid_t current_tid = gettid();
    // snprintf with NULL+0 determines the length of the string result as if it
    // would have been printed some output buffer.
    int pid_length = snprintf(NULL, 0, "%d", current_pid);
    int tid_length = snprintf(NULL, 0, "%d", current_tid);
    char *ending = ".tmp";
    // Create the full path: all parts, plus two dots and the NUL terminator
    size_t full_length = strlen(start) + strlen(name) + 1 + pid_length + 1 +
                         tid_length + strlen(ending) + 1;
    char path[full_length];
    snprintf(path, full_length, "%s%s.%d.%d%s", start, name, current_pid,
             current_tid, ending);

    // Create a file at path (O_CREAT), make sure it did not exist before
    // (O_EXCL), and allow reading and writing it (O_RDWR).
    int fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0);
    if (fd < 0) {
        __mi_printf("[File creation] Failed to open \"%s\": %s\n", path,
                    strerror(errno));
        __mi_fail();
    }
    if (unlink(path) < 0) {
        __mi_printf("[File creation] Failed to unlink \"%s\": %s\n", path,
                    strerror(errno));
        __mi_fail();
    }
    // Acquire the (write) file lease
    if (fcntl(fd, F_SETLEASE, F_WRLCK) < 0) {
        __mi_printf("[File creation] Failed to lease \"%s\": %s\n", path,
                    strerror(errno));
        __mi_fail();
    }
    // Use the given size as for the file
    if (ftruncate(fd, size) < 0) {
        __mi_printf("[File creation] Failed to truncate \"%s\": %s\n", path,
                    strerror(errno));
        __mi_fail();
    }
    return fd;
}

#if MIRT_LF_TABLE

static_assert(
    NUM_REGIONS - 1 + MIN_ALLOC_SIZE_LOG <
        sizeof(unsigned long long) * CHAR_BIT,
    "The LowFat table will be filled with powers of two, computed by a shift. "
    "This check ensures that the given configuration will not result in an "
    "invalid shift. If this assertion is triggered, you will likely have "
    "selected a huge amount of regions (allowing very large allocation sizes, "
    "in the order of EiB), or a very large smallest allocation size. Use a "
    "smaller MIN_ALLOC_SIZE or MAX_(HEAP/STACK/GLOBAL)_ALLOC_SIZE.");

static void __lowfat_create_tables_for_sizes_and_magics() {

    // First, reserve the virtual address space for all possible index values
    size_t num_pages =
        (MAXIMAL_ADDRESS / REGION_SIZE) / (__lowfat_page_size / sizeof(size_t));
    size_t len = num_pages * __lowfat_page_size;
    void *sizes_mapped =
        mmap((void *)SIZES_ADDRESS, len, PROT_READ | PROT_WRITE,
             MAP_NORESERVE | MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (sizes_mapped != (void *)SIZES_ADDRESS) {
        __mi_printf("[Table creation] Could not mmap space for SIZES: %s\n",
                    strerror(errno));
        __mi_fail();
    }

    // Create a file which backs the mapping of the virtual addresses
    int fd = __lowfat_create_file("sizes_and_magics", __lowfat_page_size);

    // Skip the first page, as this is a non-fat region
    int number_of_pages_required_for_sizes_table = 1;
    void *current_page_address =
        (uint8_t *)SIZES_ADDRESS +
        __lowfat_page_size * number_of_pages_required_for_sizes_table;
    // Compute where our mmaped memory ends
    void *past_last_page = (uint8_t *)SIZES_ADDRESS + len;

    int allow_write = 1;
    while (current_page_address < past_last_page) {
        int flags = PROT_READ;
        if (allow_write) {
            flags |= PROT_WRITE;
        }
        void *mapped = mmap(current_page_address, __lowfat_page_size, flags,
                            MAP_NORESERVE | MAP_FIXED | MAP_SHARED, fd, 0);
        if (mapped != current_page_address) {
            __mi_printf(
                "[Table creation] Could not alias space for SIZES: %s\n",
                strerror(errno));
            __mi_fail();
        }
        current_page_address += __lowfat_page_size;
        allow_write = 0;
    }
    if (close(fd) < 0) {
        __mi_printf("[Table creation] Failed to close fd for SIZES: %s\n",
                    strerror(errno));
        __mi_fail();
    }

    size_t *sizes_base_ptr = (size_t *)SIZES_ADDRESS;
    // Initialize the sizes
    // Start with region index zero; it should contain a wide upper bound as
    size_t index = 0;
    sizes_base_ptr[index] = SIZE_MAX;
    index++;
    // Store the sizes for every lowfat region
    for (size_t j = 0; j < NUM_REGIONS; j++) {
        // Compute the size on the fly.
        // We have powers of two, so 2^(MIN_ALLOC_SIZE_LOG + j) computes the
        // correct power.
        sizes_base_ptr[index] = 1ULL << (MIN_ALLOC_SIZE_LOG + j);
        index++;
    }
    // Fill the rest of the sizes table with SIZE_MAX values for non-lowfat
    // allocations
    while (((uintptr_t)(sizes_base_ptr + index) % __lowfat_page_size) > 0) {
        sizes_base_ptr[index] = SIZE_MAX;
        index++;
    }
    for (size_t j = 0; j < __lowfat_page_size / sizeof(size_t); j++) {
        sizes_base_ptr[index] = SIZE_MAX;
        index++;
    }

    // Protect the sizes from modifications
    if (mprotect((void *)sizes_base_ptr, len, PROT_READ)) {
        __mi_printf("[Table creation] Failed to protect SIZES: %s\n",
                    strerror(errno));
        __mi_fail();
    }

    // Generate the magics
    void *mag_mapped =
        mmap((void *)MAGICS_ADDRESS, len, PROT_READ | PROT_WRITE,
             MAP_NORESERVE | MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mag_mapped != (void *)MAGICS_ADDRESS) {
        __mi_printf("[Table creation] Could not mmap space for MAGICS: %s\n",
                    strerror(errno));
        __mi_fail();
    }

    // Initialize them
    uint64_t *magics_base_ptr = (uint64_t *)MAGICS_ADDRESS;

    uint64_t idx = 0;
    // Region zero is not lowfat, so store zero
    magics_base_ptr[idx] = 0;
    idx++;
    // Store the masks for all valid regions
    for (size_t j = 0; j < NUM_REGIONS; j++) {
        magics_base_ptr[idx] = UINT64_MAX << (MIN_ALLOC_SIZE_LOG + j);
        idx++;
    }
    // The rest is not lowfat, and should hence be zero
    while (((uintptr_t)(magics_base_ptr + idx) % __lowfat_page_size) > 0) {
        magics_base_ptr[idx] = 0;
        idx++;
    }
    // As the mapped memory is MAP_ANONYMOUS, its default value is zero and we
    // do not initialize the rest here.

    // Protect the magics from modifications
    if (mprotect((void *)magics_base_ptr, len, PROT_READ)) {
        __mi_printf("[Table creation] Failed to protect MAGICS: %s\n",
                    strerror(errno));
        __mi_fail();
    }
}

#endif

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
    unsigned long not_aligned = (uintptr_t)stack_begin % __lowfat_page_size;
    if (not_aligned) {
        stack_begin = stack_begin + __lowfat_page_size - not_aligned;
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

    __mi_disable_stats();

    // get original functions from dynamic linker
    initDynamicFunctions();

    // Ignore llvm tooling functions
    if (__is_llvm_tooling_function(ubp_av[0])) {
        return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini,
                                   stack_end);
    }

    __lowfat_page_size = sysconf(_SC_PAGESIZE);

    // set up statistics counters etc.
    __setup_statistics(ubp_av[0]);

#if MIRT_LF_TABLE
    __lowfat_create_tables_for_sizes_and_magics();
#endif

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
    int fd = __lowfat_create_file("stack", STACK_REGION_SIZE);
    for (unsigned i = 0; i < NUM_REGIONS; i++) {
        uintptr_t region_address = (i + 1) * REGION_SIZE + STACK_REGION_OFFSET;
        stack_regions[i] = mmap(
            (void *)region_address, STACK_REGION_SIZE, PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, fd, 0);

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

    if (close(fd) < 0) {
        __mi_printf("[Stack aliasing] Closing failed");
        __mi_fail();
    }
    __mi_debug_printf("Allocated the stack\n");

    // Move the stack
    __lowfat_move_stack_trampoline();

    // enable our malloc etc. replacements
    hooks_active = 1;

    __mi_enable_stats();
    // call the actual main function
    return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini,
                               stack_end);
}
