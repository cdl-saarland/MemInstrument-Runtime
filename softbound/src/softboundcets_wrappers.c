#include "softboundcets_common.h"
#include "softboundcets_defines.h"
#include "softboundcets_spatial.h"
#include "softboundcets_temporal.h"

#include "fail_function.h"

#include <arpa/inet.h>

#if defined(__linux__)
#include <errno.h>
#include <libintl.h>
#include <obstack.h>
#include <sys/wait.h>
#include <wait.h>
#endif

#include <fnmatch.h>
#include <sys/epoll.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <wchar.h>

#include <uuid/uuid.h>

#include <netinet/in.h>

#include <assert.h>
#include <crypt.h>
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <getopt.h>
#include <glob.h>
#include <grp.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <netdb.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <langinfo.h>
#include <regex.h>
#include <time.h>
#include <ttyent.h>
#include <unistd.h>

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#include <locale.h>
#include <utime.h>

#include <fcntl.h>
#include <wctype.h>

typedef void (*sighandler_t)(int);
typedef void (*void_func_ptr)(void);

//===----------------------------------------------------------------------===//
//                            Helper functions
//===----------------------------------------------------------------------===//

__WEAK_INLINE void
__softboundcets_read_shadow_stack_metadata_store(char **endptr, int arg_num) {

#if __SOFTBOUNDCETS_SPATIAL
    void *nptr_base = __softboundcets_load_base_shadow_stack(arg_num);
    void *nptr_bound = __softboundcets_load_bound_shadow_stack(arg_num);

    __softboundcets_metadata_store(endptr, nptr_base, nptr_bound);

#elif __SOFTBOUNDCETS_TEMPORAL

    key_type nptr_key = __softboundcets_load_key_shadow_stack(arg_num);
    lock_type nptr_lock = __softboundcets_load_lock_shadow_stack(arg_num);

    __softboundcets_metadata_store(endptr, nptr_key, nptr_lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    void *nptr_base = __softboundcets_load_base_shadow_stack(arg_num);
    void *nptr_bound = __softboundcets_load_bound_shadow_stack(arg_num);
    key_type nptr_key = __softboundcets_load_key_shadow_stack(arg_num);
    lock_type nptr_lock = __softboundcets_load_lock_shadow_stack(arg_num);
    __softboundcets_metadata_store(endptr, nptr_base, nptr_bound, nptr_key,
                                   nptr_lock);

#endif
}

__WEAK_INLINE void
__softboundcets_propagate_metadata_shadow_stack_from(int from_argnum,
                                                     int to_argnum) {

#if __SOFTBOUNDCETS_SPATIAL

    void *base = __softboundcets_load_base_shadow_stack(from_argnum);
    void *bound = __softboundcets_load_bound_shadow_stack(from_argnum);
    __softboundcets_store_base_shadow_stack(base, to_argnum);
    __softboundcets_store_bound_shadow_stack(bound, to_argnum);

#elif __SOFTBOUNDCETS_TEMPORAL

    key_type key = __softboundcets_load_key_shadow_stack(from_argnum);
    lock_type lock = __softboundcets_load_lock_shadow_stack(from_argnum);
    __softboundcets_store_key_shadow_stack(key, to_argnum);
    __softboundcets_store_lock_shadow_stack(lock, to_argnum);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    void *base = __softboundcets_load_base_shadow_stack(from_argnum);
    void *bound = __softboundcets_load_bound_shadow_stack(from_argnum);
    key_type key = __softboundcets_load_key_shadow_stack(from_argnum);
    lock_type lock = __softboundcets_load_lock_shadow_stack(from_argnum);

    __softboundcets_store_base_shadow_stack(base, to_argnum);
    __softboundcets_store_bound_shadow_stack(bound, to_argnum);
    __softboundcets_store_key_shadow_stack(key, to_argnum);
    __softboundcets_store_lock_shadow_stack(lock, to_argnum);

#endif
}

__WEAK_INLINE void __softboundcets_store_null_return_metadata() {

#if __SOFTBOUNDCETS_SPATIAL

    __softboundcets_store_base_shadow_stack(NULL, 0);
    __softboundcets_store_bound_shadow_stack(NULL, 0);

#elif __SOFTBOUNDCETS_TEMPORAL

    __softboundcets_store_key_shadow_stack(0, 0);
    __softboundcets_store_lock_shadow_stack(NULL, 0);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    __softboundcets_store_base_shadow_stack(NULL, 0);
    __softboundcets_store_bound_shadow_stack(NULL, 0);
    __softboundcets_store_key_shadow_stack(0, 0);
    __softboundcets_store_lock_shadow_stack(NULL, 0);

#endif
}

__WEAK_INLINE void __softboundcets_store_return_metadata(void *base,
                                                         void *bound,
                                                         key_type key,
                                                         lock_type lock) {

#if __SOFTBOUNDCETS_SPATIAL

    __softboundcets_store_base_shadow_stack(base, 0);
    __softboundcets_store_bound_shadow_stack(bound, 0);

#elif __SOFTBOUNDCETS_TEMPORAL

    __softboundcets_store_key_shadow_stack(key, 0);
    __softboundcets_store_lock_shadow_stack(lock, 0);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    __softboundcets_store_base_shadow_stack(base, 0);
    __softboundcets_store_bound_shadow_stack(bound, 0);
    __softboundcets_store_key_shadow_stack(key, 0);
    __softboundcets_store_lock_shadow_stack(lock, 0);

#endif
}

// Helper function to do basic checks on an argument from the shadow stack.
// Note that this should only be used for wrapper checks. Find information on
// them in the description of __SOFTBOUNDCETS_WRAPPER_CHECKS.
__WEAK_INLINE
void __softboundcets_wrapper_check_shadow_stack_ptr(int stack_slot,
                                                    const char *ptr,
                                                    size_t width) {

// We should only ensure that the arguments passed over to a standard library
// function are valid, do nothing if these kinds of checks are disabled.
#if !__SOFTBOUNDCETS_WRAPPER_CHECKS
    return;
#endif

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    lock_type lock = __softboundcets_load_lock_shadow_stack(stack_slot);
    key_type key = __softboundcets_load_key_shadow_stack(stack_slot);
    __softboundcets_temporal_dereference_check(lock, key);
#endif
#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    void *base = __softboundcets_load_base_shadow_stack(stack_slot);
    void *bound = __softboundcets_load_bound_shadow_stack(stack_slot);
    __softboundcets_spatial_dereference_check(base, bound, ptr, width);
#endif
}

//===----------------------------------------------------------------------===//
//                  Generic Wrappers for frequent cases
//===----------------------------------------------------------------------===//

// There is no information on the allocation size, we need to assume all
// memory is readable through this pointer.
__WEAK_INLINE
void __softboundcets_generic_wide_bounds_return(void *ret_ptr) {
    __softboundcets_store_return_metadata((void *)WIDE_LOWER,
                                          (void *)WIDE_UPPER, 1,
                                          __softboundcets_get_global_lock());
}

#if __SOFTBOUNDCETS_SPATIAL

// TODO those are written with only spatial safety in mind.
// Find out which generalize, make them available to configurations that include
// temporal safety.

// This function is a similar convenience function to the one above, just
// for metadata stores instead of returned pointers.
__WEAK_INLINE
void __softboundcets_metadata_store_generic_wide_bounds(void *addr_of_ptr) {
    __softboundcets_metadata_store(addr_of_ptr, (void *)WIDE_LOWER,
                                   (void *)WIDE_UPPER);
}

__WEAK_INLINE
void __softboundcets_metadata_store_generic_null_bounds(void *addr_of_ptr) {
    __softboundcets_metadata_store(addr_of_ptr, NULL, NULL);
}
#endif

//===----------------------------------------------------------------------===//
//                           stdlib.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE void softboundcets_abort() { abort(); }

__WEAK_INLINE int softboundcets_rand() { return rand(); }

__WEAK_INLINE __SOFTBOUNDCETS_NORETURN void softboundcets_exit(int status) {
    exit(status);
}

__WEAK_INLINE __SOFTBOUNDCETS_NORETURN void softboundcets__Exit(int status) {
    _Exit(status);
}

__WEAK_INLINE double softboundcets_drand48() { return drand48(); }

__WEAK_INLINE long int softboundcets_lrand48() { return lrand48(); }

__WEAK_INLINE char *softboundcets_getenv(const char *name) {

    char *ret_ptr = getenv(name);

    if (ret_ptr) {
        __softboundcets_store_return_metadata(
            ret_ptr, ret_ptr + strlen(ret_ptr) + 1, 1,
            __softboundcets_get_global_lock());
    } else {
        __softboundcets_store_null_return_metadata();
    }

    return ret_ptr;
}

__WEAK_INLINE int softboundcets_atexit(void_func_ptr function) {
    return atexit(function);
}

__WEAK_INLINE int softboundcets_putenv(char *string) {
    int ret = putenv(string);
    __softboundcets_update_environment_metadata();
    return ret;
}

__WEAK_INLINE int softboundcets_setenv(const char *name, const char *value,
                                       int overwrite) {

    int ret = setenv(name, value, overwrite);
    __softboundcets_update_environment_metadata();
    return ret;
}

__WEAK_INLINE int softboundcets_unsetenv(const char *name) {
    int ret = unsetenv(name);
    __softboundcets_update_environment_metadata();
    return ret;
}

__WEAK_INLINE int softboundcets_system(char *ptr) { return system(ptr); }

__WEAK_INLINE int softboundcets_mkstemp(char *template) {

    /* tested */
    return mkstemp(template);
}

__WEAK_INLINE double softboundcets_atof(char *ptr) { return atof(ptr); }

__WEAK_INLINE int softboundcets_rpmatch(const char *response) {
    return rpmatch(response);
}

__WEAK_INLINE char *softboundcets_mkdtemp(char *template) {

    char *ret_ptr = mkdtemp(template);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE size_t softboundcets___ctype_get_mb_cur_max(void) {
    return __ctype_get_mb_cur_max();
}

__WEAK_INLINE unsigned long int softboundcets_strtoul(const char *nptr,
                                                      char **endptr, int base) {

    unsigned long temp = strtoul(nptr, endptr, base);
    if (endptr != NULL) {
        __softboundcets_read_shadow_stack_metadata_store(endptr, 0);
    }

    return temp;
}

__WEAK_INLINE double softboundcets_strtod(const char *nptr, char **endptr) {

    double temp = strtod(nptr, endptr);

    if (endptr != NULL) {
        __softboundcets_read_shadow_stack_metadata_store(endptr, 0);
    }
    return temp;
}

__WEAK_INLINE long softboundcets_strtol(const char *nptr, char **endptr,
                                        int base) {

    long temp = strtol(nptr, endptr, base);
    if (endptr != NULL) {
        __mi_debug_printf("[strtol] *endptr=%p\n", *endptr);
        __softboundcets_read_shadow_stack_metadata_store(endptr, 0);
    }
    return temp;
}

__WEAK_INLINE int softboundcets_atoi(const char *ptr) {

    if (ptr == NULL) {
        __mi_fail();
    }
    return atoi(ptr);
}

__WEAK_INLINE long softboundcets_atol(const char *nptr) { return atol(nptr); }

__WEAK_INLINE char *softboundcets_gcvt(double number, int ndigit, char *buf) {
#if __SOFTBOUNDCETS_WRAPPER_CHECKS
    // __mi_fail_with_msg("Missing wrapper check for gcvt.");
#endif
    char *res = gcvt(number, ndigit, buf);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return res;
}

//===------------------------ stdlib-bsearch.h ----------------------------===//

__WEAK_INLINE void *
softboundcets_bsearch(const void *key, const void *base, size_t nmemb,
                      size_t size, int (*compar)(const void *, const void *)) {

    void *ret_ptr = bsearch(key, base, nmemb, size, compar);

    if (ret_ptr) {
        __softboundcets_propagate_metadata_shadow_stack_from(2, 0);
    } else {
        __softboundcets_store_null_return_metadata();
    }
    return ret_ptr;
}

//===----------------------------------------------------------------------===//
//                         resources.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_getrlimit(int resource, struct rlimit *rlim) {

    /* tested */
    return getrlimit(resource, rlim);
}

__WEAK_INLINE int softboundcets_setrlimit(int resource,
                                          const struct rlimit *rlim) {

    /* tested */
    return setrlimit(resource, rlim);
}

//===----------------------------------------------------------------------===//
//                            stat.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE mode_t softboundcets_umask(mode_t mask) {

    /* tested */
    return umask(mask);
}

__WEAK_INLINE int softboundcets_mkdir(const char *pathname, mode_t mode) {

    /* tested */
    return mkdir(pathname, mode);
}

__WEAK_INLINE int softboundcets_stat(const char *path, struct stat *buf) {

    /* tested */
    return stat(path, buf);
}

__WEAK_INLINE int softboundcets_fstat(int filedes, struct stat *buff) {
    return fstat(filedes, buff);
}

__WEAK_INLINE int softboundcets___lxstat(int __ver, const char *__filename,
                                         struct stat *__stat_buf) {
    return __lxstat(__ver, __filename, __stat_buf);
}

__WEAK_INLINE int softboundcets___fxstat(int ver, int file_des,
                                         struct stat *stat_struct) {
    return __fxstat(ver, file_des, stat_struct);
}

__WEAK_INLINE int softboundcets___fxstatat(int ver, int file_des,
                                           const char *filename,
                                           struct stat *stat_struct, int flag) {
    return __fxstatat(ver, file_des, filename, stat_struct, flag);
}

__WEAK_INLINE int softboundcets_futimens(int fd,
                                         const struct timespec times[2]) {
    return futimens(fd, times);
}

__WEAK_INLINE int softboundcets_utimensat(int dirfd, const char *pathname,
                                          const struct timespec times[2],
                                          int flags) {
    return utimensat(dirfd, pathname, times, flags);
}

__WEAK_INLINE int softboundcets_mkdirat(int dirfd, const char *pathname,
                                        mode_t mode) {
    return mkdirat(dirfd, pathname, mode);
}

__WEAK_INLINE int softboundcets_fchmod(int fd, mode_t mode) {
    return fchmod(fd, mode);
}

__WEAK_INLINE int softboundcets_chmod(const char *path, mode_t mode) {
    return chmod(path, mode);
}

__WEAK_INLINE int softboundcets_fchmodat(int dirfd, const char *pathname,
                                         mode_t mode, int flags) {
    return fchmodat(dirfd, pathname, mode, flags);
}

#if defined(__linux__)

__WEAK_INLINE int softboundcets___xmknodat(int __ver, int __fd,
                                           const char *__path, __mode_t __mode,
                                           __dev_t *__dev) {
    return __xmknodat(__ver, __fd, __path, __mode, __dev);
}

__WEAK_INLINE int softboundcets_mkfifoat(int dirfd, const char *pathname,
                                         mode_t mode) {
    return mkfifoat(dirfd, pathname, mode);
}

#endif

//===----------------------------------------------------------------------===//
//                             math.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE double softboundcets_log(double x) { return log(x); }

__WEAK_INLINE float softboundcets_logf(float arg) { return logf(arg); }

__WEAK_INLINE long double softboundcets_logl(long double arg) {
    return logl(arg);
}

__WEAK_INLINE double softboundcets_acos(double x) { return acos(x); }

__WEAK_INLINE double softboundcets_atan(double x) { return atan(x); }

__WEAK_INLINE double softboundcets_atan2(double y, double x) {
    return atan2(y, x);
}

__WEAK_INLINE float softboundcets_atan2f(float y, float x) {
    return atan2f(y, x);
}

__WEAK_INLINE long double softboundcets_atan2l(long double y, long double x) {
    return atan2l(y, x);
}

__WEAK_INLINE double softboundcets_sqrt(double arg) { return sqrt(arg); }

__WEAK_INLINE float softboundcets_sqrtf(float x) { return sqrtf(x); }

__WEAK_INLINE long double softboundcets_sqrtl(long double arg) {
    return sqrtl(arg);
}

__WEAK_INLINE double softboundcets_exp(double arg) { return exp(arg); }

__WEAK_INLINE float softboundcets_expf(float x) { return expf(x); }

__WEAK_INLINE long double softboundcets_expl(long double arg) {
    return expl(arg);
}

__WEAK_INLINE double softboundcets_exp2(double x) { return exp2(x); }

__WEAK_INLINE double softboundcets_ceil(double x) { return ceil(x); }

__WEAK_INLINE float softboundcets_ceilf(float x) { return ceilf(x); }

__WEAK_INLINE long double softboundcets_ceill(long double arg) {
    return ceill(arg);
}
__WEAK_INLINE double softboundcets_floor(double x) { return floor(x); }

__WEAK_INLINE float softboundcets_floorf(float x) { return floorf(x); }

__WEAK_INLINE long double softboundcets_floorl(long double x) {
    return floorl(x);
}

__WEAK_INLINE double softboundcets_fabs(double x) { return fabs(x); }

__WEAK_INLINE int softboundcets_abs(int j) { return abs(j); }

__WEAK_INLINE void softboundcets_srand(unsigned int seed) { srand(seed); }

__WEAK_INLINE void softboundcets_srand48(long int seed) { srand48(seed); }

__WEAK_INLINE double softboundcets_pow(double x, double y) { return pow(x, y); }

__WEAK_INLINE float softboundcets_powf(float base, float exponent) {
    return powf(base, exponent);
}

__WEAK_INLINE long double softboundcets_powl(long double base,
                                             long double exponent) {
    return powl(base, exponent);
}

__WEAK_INLINE float softboundcets_fabsf(float x) { return fabsf(x); }

__WEAK_INLINE double softboundcets_tan(double x) { return tan(x); }

__WEAK_INLINE float softboundcets_tanf(float x) { return tanf(x); }

__WEAK_INLINE long double softboundcets_tanl(long double x) { return tanl(x); }

__WEAK_INLINE double softboundcets_log10(double x) { return log10(x); }

__WEAK_INLINE double softboundcets_sin(double x) { return sin(x); }

__WEAK_INLINE float softboundcets_sinf(float x) { return sinf(x); }

__WEAK_INLINE long double softboundcets_sinl(long double x) { return sinl(x); }

__WEAK_INLINE double softboundcets_cos(double x) { return cos(x); }

__WEAK_INLINE float softboundcets_cosf(float x) { return cosf(x); }

__WEAK_INLINE long double softboundcets_cosl(long double x) { return cosl(x); }

__WEAK_INLINE double softboundcets_ldexp(double x, int exp) {
    return ldexp(x, exp);
}

__WEAK_INLINE double softboundcets_modf(double x, double *intpart) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)intpart,
                                                   sizeof(*intpart));
    return modf(x, intpart);
}

__WEAK_INLINE float softboundcets_modff(float x, float *intpart) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)intpart,
                                                   sizeof(*intpart));
    return modff(x, intpart);
}

__WEAK_INLINE long double softboundcets_modfl(long double x,
                                              long double *intpart) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)intpart,
                                                   sizeof(*intpart));
    return modfl(x, intpart);
}

__WEAK_INLINE double softboundcets_fmod(double x, double y) {
    return fmod(x, y);
}

__WEAK_INLINE float softboundcets_fmodf(float x, float y) {
    return fmodf(x, y);
}

__WEAK_INLINE long double softboundcets_fmodl(long double x, long double y) {
    return fmodl(x, y);
}

__WEAK_INLINE double softboundcets_frexp(double x, int *exponent) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)exponent,
                                                   sizeof(*exponent));
    return frexp(x, exponent);
}

//===----------------------------------------------------------------------===//
//                         File Manipulation Wrappers
//                            (stdio.h/stdio2.h)
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_fputc(int c, FILE *stream) {

    /* tested */
    return fputc(c, stream);
}

__WEAK_INLINE int softboundcets_putc(int c, FILE *stream) {
    return putc(c, stream);
}

__WEAK_INLINE int softboundcets_printf(const char *format, ...) {
    va_list args;

    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}

__WEAK_INLINE int softboundcets_vprintf(const char *format, va_list ap) {
    return vprintf(format, ap);
}

__WEAK_INLINE int softboundcets_fprintf(FILE *stream, const char *format, ...) {
    va_list args;

    va_start(args, format);
    int ret = vfprintf(stream, format, args);
    va_end(args);
    return ret;
}

__WEAK_INLINE int softboundcets_vfprintf(FILE *stream, const char *format,
                                         va_list ap) {
    return vfprintf(stream, format, ap);
}

// TODO The functions (v)sprintf printing to _str_ are tricky to check as the
// length to be written to _str_ depends on the length of _format_ with the
// format specifiers replaced...
__WEAK_INLINE int softboundcets_sprintf(char *str, const char *format, ...) {

    va_list args;

    va_start(args, format);
    int ret = vsprintf(str, format, args);
    va_end(args);
    return ret;
}

__WEAK_INLINE int softboundcets_vsprintf(char *str, const char *format,
                                         va_list ap) {
    return vsprintf(str, format, ap);
}

__WEAK_INLINE int softboundcets_snprintf(char *str, size_t size,
                                         const char *format, ...) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, str, size);

    va_list args;

    va_start(args, format);
    int ret = vsnprintf(str, size, format, args);
    va_end(args);
    return ret;
}

__WEAK_INLINE int softboundcets_vsnprintf(char *str, size_t size,
                                          const char *format, va_list ap) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, str, size);

    int ret = vsnprintf(str, size, format, ap);

    return ret;
}

__WEAK_INLINE int softboundcets_fileno(FILE *stream) { return fileno(stream); }

__WEAK_INLINE int softboundcets_fgetc(FILE *stream) { return fgetc(stream); }

__WEAK_INLINE int softboundcets_getc(FILE *stream) { return getc(stream); }

__WEAK_INLINE int softboundcets_ungetc(int c, FILE *stream) {
    return ungetc(c, stream);
}

__WEAK_INLINE int softboundcets_putchar(int c) { return putchar(c); }

__WEAK_INLINE char *softboundcets_gets(char *s) {

    __mi_fail_with_msg("[Error] gets used and should not be used\n");
    return NULL;
}

__WEAK_INLINE void softboundcets_puts(char *ptr) { puts(ptr); }

__WEAK_INLINE char *softboundcets_fgets(char *s, int size, FILE *stream) {

    char *ret_ptr = fgets(s, size, stream);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);

    return ret_ptr;
}

__WEAK_INLINE void softboundcets_perror(const char *s) { perror(s); }

__WEAK_INLINE long long softboundcets_fwrite(char *ptr, size_t size,
                                             size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

__WEAK_INLINE int softboundcets_feof(FILE *stream) { return feof(stream); }

__WEAK_INLINE int softboundcets_remove(const char *pathname) {
    return remove(pathname);
}

__WEAK_INLINE void softboundcets_setbuf(FILE *stream, char *buf) {
    setbuf(stream, buf);
}

__WEAK_INLINE FILE *softboundcets_tmpfile(void) {

    void *ret_ptr = tmpfile();
    if (ret_ptr) {
        void *ret_ptr_bound = (char *)ret_ptr + sizeof(FILE);
        __softboundcets_store_return_metadata(
            ret_ptr, ret_ptr_bound, 1, __softboundcets_get_global_lock());
    }
    return ret_ptr;
}

__WEAK_INLINE void softboundcets_clearerr(FILE *stream) { clearerr(stream); }

__WEAK_INLINE int softboundcets_ferror(FILE *stream) { return ferror(stream); }

__WEAK_INLINE long softboundcets_ftell(FILE *stream) { return ftell(stream); }

__WEAK_INLINE FILE *softboundcets_fopen(const char *path, const char *mode) {

    FILE *ret_ptr = fopen(path, mode);
    if (ret_ptr) {
        void *ret_ptr_bound = (char *)ret_ptr + sizeof(FILE);

        __softboundcets_store_return_metadata(
            ret_ptr, ret_ptr_bound, 1, __softboundcets_get_global_lock());
    }

    return ret_ptr;
}

__WEAK_INLINE FILE *softboundcets_fdopen(int fildes, const char *mode) {

    FILE *ret_ptr = fdopen(fildes, mode);
    if (ret_ptr) {
        void *ret_ptr_bound = (char *)ret_ptr + sizeof(FILE);
        __softboundcets_store_return_metadata(
            ret_ptr, ret_ptr_bound, 1, __softboundcets_get_global_lock());
    }
    return ret_ptr;
}

__WEAK_INLINE int softboundcets_fseek(FILE *stream, long offset, int whence) {

    return fseek(stream, offset, whence);
}

__WEAK_INLINE FILE *softboundcets_popen(const char *command, const char *type) {

    FILE *ret_ptr = popen(command, type);
    if (ret_ptr) {
        void *ret_ptr_bound = (char *)ret_ptr + sizeof(FILE);
        __softboundcets_store_return_metadata(
            ret_ptr, ret_ptr_bound, 1, __softboundcets_get_global_lock());
    }
    return ret_ptr;
}

__WEAK_INLINE int softboundcets_fclose(FILE *fp) { return fclose(fp); }

__WEAK_INLINE int softboundcets_pclose(FILE *stream) { return pclose(stream); }

__WEAK_INLINE void softboundcets_rewind(FILE *stream) { rewind(stream); }

__WEAK_INLINE int softboundcets_fflush(FILE *stream) { return fflush(stream); }

__WEAK_INLINE int softboundcets_fputs(const char *s, FILE *stream) {

    return fputs(s, stream);
}

__WEAK_INLINE size_t softboundcets_fread_unlocked(void *ptr, size_t size,
                                                  size_t n, FILE *stream) {

    return fread_unlocked(ptr, size, n, stream);
}

#if 0
__WEAK_INLINE int
softboundcets_fputs_unlocked(const char *s, FILE *stream){
  return fputs_unlocked(s, stream);
}
#endif

__WEAK_INLINE size_t softboundcets_fread(void *ptr, size_t size, size_t nmemb,
                                         FILE *stream) {
    /* tested */
    return fread(ptr, size, nmemb, stream);
}

__WEAK_INLINE int softboundcets_rename(const char *old_path,
                                       const char *new_path) {

    return rename(old_path, new_path);
}

__WEAK_INLINE int softboundcets_renameat(int olddirfd, const char *oldpath,
                                         int newdirfd, const char *newpath) {
    return renameat(olddirfd, oldpath, newdirfd, newpath);
}

__WEAK_INLINE ssize_t softboundcets___getdelim(char **lineptr, size_t *n,
                                               int delim, FILE *stream) {

    // TODO
    // This should be checked:
    // Temporal:
    // The application shall ensure that *lineptr is a valid argument that could
    // be passed to the free() function.
    // Spatial:
    // If *n is non-zero, the application shall ensure that *lineptr either
    // points to an object of size at least *n bytes, or is a null pointer.

    ssize_t ret_val = __getdelim(lineptr, n, delim, stream);

    if (ret_val != -1) {
#if __SOFTBOUNDCETS_SPATIAL
        // TODO maybe the upper bound is max(*oldn, strlen(*lineptr))?
        // The lower pointer should be correct, as there is a constraint on it
        // to be freeable (or it is allocated by the function).
        __softboundcets_metadata_store(lineptr, *lineptr,
                                       (*lineptr) + strlen(*lineptr));
#elif __SOFTBOUNDCETS_TEMPORAL
        __softboundcets_metadata_store(lineptr, 1,
                                       __softboundcets_get_global_lock());
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __softboundcets_metadata_store(lineptr, *lineptr,
                                       (*lineptr) + strlen(*lineptr), 1,
                                       __softboundcets_get_global_lock());
#endif
    }

    return ret_val;
}

__WEAK_INLINE ssize_t softboundcets_getdelim(char **lineptr, size_t *n,
                                             int delim, FILE *stream) {
    return softboundcets___getdelim(lineptr, n, delim, stream);
}

__WEAK_INLINE int softboundcets_setvbuf(FILE *restrict stream,
                                        char *restrict buf, int mode,
                                        size_t size) {
#if __SOFTBOUNDCETS_WRAPPER_CHECKS
    // TODO Checks on stream?
    if (buf && mode != _IONBF) {
        __softboundcets_wrapper_check_shadow_stack_ptr(1, buf, size);
    }
#endif
    // TODO Under some complicated conditions, the internal buffer in stream (fp
    // below) is set to the buffer handed over here. If one can get the buffer
    // back (or access it externally after storing it), we need to propagate
    // metadata here. A `getvbuf` function seems not to be available though.
    // fp->_bf._base = fp->_p = (unsigned char *)buf;
    return setvbuf(stream, buf, mode, size);
}

//===----------------------------------------------------------------------===//
//                     wchar.h/wctype-wchar.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_iswprint(wint_t wc) { return iswprint(wc); }

__WEAK_INLINE size_t softboundcets_mbrtowc(wchar_t *pwc, const char *s,
                                           size_t n, mbstate_t *ps) {
    return mbrtowc(pwc, s, n, ps);
}

__WEAK_INLINE int softboundcets_mbsinit(const mbstate_t *ps) {
    return mbsinit(ps);
}

__WEAK_INLINE wint_t softboundcets_towlower(wint_t wc) { return towlower(wc); }

//===----------------------------------------------------------------------===//
//                           dirent.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE DIR *softboundcets_fdopendir(int fd) {

    DIR *ret_ptr = fdopendir(fd);
    if (ret_ptr) {
        __softboundcets_generic_wide_bounds_return(ret_ptr);
    }
    return ret_ptr;
}

__WEAK_INLINE int softboundcets_dirfd(DIR *dirp) { return dirfd(dirp); }

__WEAK_INLINE struct dirent *softboundcets_readdir(DIR *dir) {

    struct dirent *ret_ptr = readdir(dir);
    if (ret_ptr) {
        void *ret_ptr_bound = (char *)ret_ptr + sizeof(struct dirent);
        __softboundcets_store_return_metadata(
            ret_ptr, ret_ptr_bound, 1, __softboundcets_get_global_lock());
    }

    return ret_ptr;
}

__WEAK_INLINE DIR *softboundcets_opendir(const char *name) {

    DIR *ret_ptr = opendir(name);
    if (ret_ptr) {
        __softboundcets_generic_wide_bounds_return(ret_ptr);
    }
    return ret_ptr;
}

__WEAK_INLINE int softboundcets_closedir(DIR *dir) { return closedir(dir); }

__WEAK_INLINE void softboundcets_rewinddir(DIR *dirp) { rewinddir(dirp); }

//===----------------------------------------------------------------------===//
//                           local.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE struct lconv *softboundcets_localeconv(void) {
    struct lconv *temp = localeconv();

    __softboundcets_store_return_metadata(temp, temp + sizeof(struct lconv), 1,
                                          __softboundcets_get_global_lock());

    return temp;
}

#if defined(__linux__)

__WEAK_INLINE char *softboundcets_setlocale(int category, const char *locale) {

    void *ret_ptr = setlocale(category, locale);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr)), 1,
        __softboundcets_get_global_lock());
    return ret_ptr;
}
#endif

//===----------------------------------------------------------------------===//
//                           regex.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
int softboundcets_regcomp(regex_t *preg, const char *regex, int cflags) {
    return regcomp(preg, regex, cflags);
}

__WEAK_INLINE
size_t softboundcets_regerror(int errcode, const regex_t *preg, char *errbuf,
                              size_t errbuf_size) {

    return regerror(errcode, preg, errbuf, errbuf_size);
}

__WEAK_INLINE
int softboundcets_regexec(const regex_t *preg, const char *string,
                          size_t nmatch, regmatch_t pmatch[], int eflags) {
    return regexec(preg, string, nmatch, pmatch, eflags);
}

//===----------------------------------------------------------------------===//
//                           iconv.h Wrappers
//===----------------------------------------------------------------------===//

#ifdef HAVE_ICONV_H
__WEAK_INLINE
size_t softboundcets_iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
                           char **outbuf, size_t *outbytesleft) {

    return iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

__WEAK_INLINE
iconv_t softboundcets_iconv_open(const char *tocode, const char *fromcode) {

    return iconv_open(tocode, fromcode);
}
#endif

//===----------------------------------------------------------------------===//
//                           pwd.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
struct passwd *softboundcets_getpwnam(const char *name) {
    struct passwd *ret_ptr = getpwnam(name);
    if (ret_ptr) {
        __softboundcets_store_return_metadata(
            ret_ptr, (char *)ret_ptr + sizeof(struct passwd), 1,
            __softboundcets_get_global_lock());
    }

    return ret_ptr;
}

__WEAK_INLINE struct passwd *softboundcets_getpwuid(uid_t uid) {
    struct passwd *ret_ptr = getpwuid(uid);
    if (!ret_ptr) {
        __softboundcets_store_null_return_metadata();
        return ret_ptr;
    }
    __softboundcets_store_return_metadata(
        ret_ptr, (char *)ret_ptr + sizeof(struct passwd), 1,
        __softboundcets_get_global_lock());

    // Enter the length for every entry in the passwd struct into our shadow
    // memory.
    // Store pw_name length
    char *name = ret_ptr->pw_name;
    if (name) {

#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_store(&ret_ptr->pw_name, name,
                                       name + strlen(name) + 1);
#elif __SOFTBOUNDCETS_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "temporal safety not implemented");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "spatial+temporal safety not implemented");
#endif
    }
    // Store pw_passwd length
    char *passwd = ret_ptr->pw_passwd;
    if (passwd) {
#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_store(&ret_ptr->pw_passwd, passwd,
                                       passwd + strlen(passwd) + 1);
#elif __SOFTBOUNDCETS_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "temporal safety not implemented");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "spatial+temporal safety not implemented");
#endif
    }

    // Store pw_gecos length
    char *gecos = ret_ptr->pw_gecos;
    if (gecos) {
#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_store(&ret_ptr->pw_gecos, gecos,
                                       gecos + strlen(gecos) + 1);
#elif __SOFTBOUNDCETS_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "temporal safety not implemented");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "spatial+temporal safety not implemented");
#endif
    }

    // Store pw_dir length
    char *dir = ret_ptr->pw_dir;
    if (dir) {
#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_store(&ret_ptr->pw_dir, dir,
                                       dir + strlen(dir) + 1);
#elif __SOFTBOUNDCETS_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "temporal safety not implemented");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "spatial+temporal safety not implemented");
#endif
    }

    // Store pw_shell length
    char *shell = ret_ptr->pw_shell;
    if (shell) {
#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_store(&ret_ptr->pw_shell, shell,
                                       shell + strlen(shell) + 1);
#elif __SOFTBOUNDCETS_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "temporal safety not implemented");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __mi_fail_with_msg("getpwuid wrapper: metadata store for "
                           "spatial+temporal safety not implemented");
#endif
    }

    return ret_ptr;
}

//===----------------------------------------------------------------------===//
//                           grp.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
struct group *softboundcets_getgrnam(const char *name) {
    struct group *ret_ptr = getgrnam(name);
    if (ret_ptr) {
        __softboundcets_store_return_metadata(
            ret_ptr, (char *)ret_ptr + sizeof(struct group), 1,
            __softboundcets_get_global_lock());
    }
    return ret_ptr;
}

__WEAK_INLINE struct group *softboundcets_getgrgid(gid_t gid) {

    struct group *ret_ptr = getgrgid(gid);
    if (ret_ptr) {
        __softboundcets_store_return_metadata(
            ret_ptr, (char *)ret_ptr + sizeof(struct group), 1,
            __softboundcets_get_global_lock());
    }

    return ret_ptr;
}

//===----------------------------------------------------------------------===//
//                       fcntl.h/fcntl2.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_creat(const char *pathname, mode_t mode) {

    return creat(pathname, mode);
}

__WEAK_INLINE int softboundcets_open(const char *pathname, int flags) {
    return open(pathname, flags);
}

__WEAK_INLINE int softboundcets_openat(int dirfd, const char *pathname,
                                       int flags) {
    return openat(dirfd, pathname, flags);
}

//===----------------------------------------------------------------------===//
//                          fnmatch.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_fnmatch(const char *pattern, const char *string,
                                        int flags) {

    return fnmatch(pattern, string, flags);
}

//===----------------------------------------------------------------------===//
//                           unistd.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE __SOFTBOUNDCETS_NORETURN void softboundcets__exit(int status) {
    _exit(status);
}

__WEAK_INLINE ssize_t softboundcets_read(int fd, void *buf, size_t count) {
    return read(fd, buf, count);
}

__WEAK_INLINE ssize_t softboundcets_write(int fd, void *buf, size_t count) {
    return write(fd, buf, count);
}

__WEAK_INLINE off_t softboundcets_lseek(int fildes, off_t offset, int whence) {
    return lseek(fildes, offset, whence);
}

__WEAK_INLINE int softboundcets_getpagesize(void) { return getpagesize(); }

__WEAK_INLINE int softboundcets_fsync(int fd) { return fsync(fd); }

__WEAK_INLINE int softboundcets_ftruncate(int fd, off_t length) {
    return ftruncate(fd, length);
}

__WEAK_INLINE int softboundcets_chroot(const char *path) {

    /* tested */
    return chroot(path);
}

__WEAK_INLINE int softboundcets_rmdir(const char *pathname) {

    /* tested */
    return rmdir(pathname);
}

__WEAK_INLINE int softboundcets_setuid(uid_t uid) { return setuid(uid); }

__WEAK_INLINE int softboundcets_setreuid(uid_t ruid, uid_t euid) {

    /* tested */
    return setreuid(ruid, euid);
}

__WEAK_INLINE uid_t softboundcets_geteuid() { return geteuid(); }

__WEAK_INLINE uid_t softboundcets_getuid(void) {

    /* tested */
    return getuid();
}

__WEAK_INLINE long softboundcets_pathconf(char *path, int name) {
    return pathconf(path, name);
}

__WEAK_INLINE int softboundcets_linkat(int olddirfd, const char *oldpath,
                                       int newdirfd, const char *newpath,
                                       int flags) {

    return linkat(olddirfd, oldpath, newdirfd, newpath, flags);
}

__WEAK_INLINE unsigned int softboundcets_sleep(unsigned int seconds) {

    return sleep(seconds);
}

__WEAK_INLINE char *softboundcets_getcwd(char *buf, size_t size) {

    if (buf == NULL) {
        __mi_fail_with_msg("[getcwd] The given buffer is null.\n");
    }

    __softboundcets_wrapper_check_shadow_stack_ptr(1, buf, size);

    char *ret_ptr = getcwd(buf, size);

    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);

    return ret_ptr;
}

__WEAK_INLINE int softboundcets_setgid(gid_t gid) { return setgid(gid); }

__WEAK_INLINE gid_t softboundcets_getgid(void) { return getgid(); }

__WEAK_INLINE gid_t softboundcets_getegid(void) { return getegid(); }

__WEAK_INLINE int softboundcets_readlinkat(int dirfd, const char *pathname,
                                           char *buf, size_t bufsiz) {
    return readlinkat(dirfd, pathname, buf, bufsiz);
}

__WEAK_INLINE int softboundcets_unlinkat(int dirfd, const char *pathname,
                                         int flags) {
    return unlinkat(dirfd, pathname, flags);
}

__WEAK_INLINE int softboundcets_symlinkat(const char *oldpath, int newdirfd,
                                          const char *newpath) {

    return symlinkat(oldpath, newdirfd, newpath);
}

__WEAK_INLINE int softboundcets_fchown(int fd, uid_t owner, gid_t group) {
    return fchown(fd, owner, group);
}

__WEAK_INLINE int softboundcets_fchownat(int dirfd, const char *pathname,
                                         uid_t owner, gid_t group, int flags) {

    return fchownat(dirfd, pathname, owner, group, flags);
}

__WEAK_INLINE pid_t softboundcets_getpid(void) { return getpid(); }

__WEAK_INLINE pid_t softboundcets_getppid(void) { return getppid(); }

__WEAK_INLINE int softboundcets_chown(const char *path, uid_t owner,
                                      gid_t group) {
    return chown(path, owner, group);
}

__WEAK_INLINE int softboundcets_isatty(int desc) { return isatty(desc); }

__WEAK_INLINE int softboundcets_chdir(const char *path) { return chdir(path); }

__WEAK_INLINE int softboundcets_fchdir(int fd) { return fchdir(fd); }

__WEAK_INLINE int softboundcets_unlink(const char *pathname) {
    return unlink(pathname);
}

__WEAK_INLINE int softboundcets_close(int fd) { return close(fd); }

__WEAK_INLINE int softboundcets_dup(int oldfd) { return dup(oldfd); }

__WEAK_INLINE int softboundcets_dup2(int oldfd, int newfd) {
    return dup2(oldfd, newfd);
}

#ifdef _GNU_SOURCE
__WEAK_INLINE int softboundcets_dup3(int oldfd, int newfd, int flags) {
    return dup3(oldfd, newfd, flags);
}
#endif

__WEAK_INLINE
int softboundcets_execve(const char *pathname, char *const argv[],
                         char *const envp[]) {
    return execve(pathname, argv, envp);
}

__WEAK_INLINE
int softboundcets_execv(const char *file, char *const argv[]) {
    return execv(file, argv);
}

__WEAK_INLINE
int softboundcets_execvp(const char *file, char *const argv[]) {
    return execvp(file, argv);
}

#ifdef _GNU_SOURCE
__WEAK_INLINE
int softboundcets_execvpe(const char *file, char *const argv[],
                          char *const envp[]) {
    return execvpe(file, argv, envp);
}
#endif

__WEAK_INLINE pid_t softboundcets_fork(void) { return fork(); }

__WEAK_INLINE int softboundcets_pipe(int pipefd[2]) {
#if __SOFTBOUNDCETS_WRAPPER_CHECKS
    // TODO maybe check if pipefd is valid. It is unclear what conditions it has
    // to meet, it seems that some sanitization is done for pipefd, as the
    // function might return EFAULT in case of an "invalid" pipefd. If something
    // should be checked here, make sure to not modify the behavior for this
    // corner case.
#endif
    return pipe(pipefd);
}

__WEAK_INLINE
int softboundcets_truncate(const char *path, off_t length) {
    return truncate(path, length);
}

__WEAK_INLINE
int softboundcets_link(const char *path1, const char *path2) {
    return link(path1, path2);
}

//===----------------------------------------------------------------------===//
//                         String Function Wrappers
//                          (string.h/strings.h)
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_strcmp(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}

__WEAK_INLINE int softboundcets_strcasecmp(const char *s1, const char *s2) {
    return strcasecmp(s1, s2);
}

__WEAK_INLINE int softboundcets_strncasecmp(const char *s1, const char *s2,
                                            size_t n) {
    return strncasecmp(s1, s2, n);
}

__WEAK_INLINE size_t softboundcets_strlen(const char *s) {
    // TODO This should probably check that no memory beyond the bounds of s is
    // read...
    return strlen(s);
}

__WEAK_INLINE char *softboundcets_strpbrk(const char *s, const char *accept) {

    char *ret_ptr = strpbrk(s, accept);
    if (ret_ptr != NULL) {
        __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    } else {

        __softboundcets_store_null_return_metadata();
    }

    return ret_ptr;
}

__WEAK_INLINE int softboundcets_strncmp(const char *s1, const char *s2,
                                        size_t n) {
    return strncmp(s1, s2, n);
}

__WEAK_INLINE size_t softboundcets_strspn(const char *s, const char *accept) {
    return strspn(s, accept);
}

__WEAK_INLINE size_t softboundcets_strcspn(const char *s, const char *reject) {
    return strcspn(s, reject);
}

__WEAK_INLINE int softboundcets_memcmp(const void *s1, const void *s2,
                                       size_t n) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, s1, n);
    __softboundcets_wrapper_check_shadow_stack_ptr(1, s2, n);
    return memcmp(s1, s2, n);
}

#ifdef _GNU_SOURCE

__WEAK_INLINE void *softboundcets_memrchr(const void *s, int c, size_t n) {
    void *ret_ptr = memrchr(s, c, n);
    if (ret_ptr != NULL) {
        __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    } else {
        __softboundcets_store_null_return_metadata();
    }
    return ret_ptr;
}
#endif

__WEAK_INLINE void *softboundcets_memchr(const void *s, int c, size_t n) {
    void *ret_ptr = memchr(s, c, n);
    if (ret_ptr != NULL) {
        __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    } else {
        __softboundcets_store_null_return_metadata();
    }
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_index(char *s, int c) {

    char *ret_ptr = index(s, c);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_rindex(char *s, int c) {

    char *ret_ptr = rindex(s, c);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

#ifdef _GNU_SOURCE

__WEAK_INLINE char *softboundcets_strchrnul(const char *s, int c) {

    char *ret_ptr = strchrnul(s, c);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}
#endif

__WEAK_INLINE char *softboundcets_strchr(const char *s, int c) {

    char *ret_ptr = strchr(s, c);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_strrchr(const char *s, int c) {

    char *ret_ptr = strrchr(s, c);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_stpcpy(char *dest, char *src) {

    void *ret_ptr = stpcpy(dest, src);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_strcpy(char *dest, char *src) {

#if __SOFTBOUNDCETS_WRAPPER_CHECKS
#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    void *src_base = __softboundcets_load_base_shadow_stack(2);
    void *src_bound = __softboundcets_load_bound_shadow_stack(2);

    // TODO it might be reasonable to have an internal safe version of strlen
    // that gets base and bound as arguments, such that the shadow stack does
    // not need to be used (this would likely be more run-time efficient and
    // require less code in the wrappers that use it)

    // The amount of bytes copied is the length of the string + null terminator.
    __softboundcets_allocate_shadow_stack_space(1);
    __softboundcets_store_base_shadow_stack(src_base, 0);
    __softboundcets_store_bound_shadow_stack(src_bound, 0);
    size_t size = softboundcets_strlen(src) + 1;
    __softboundcets_deallocate_shadow_stack_space();

    void *dest_base = __softboundcets_load_base_shadow_stack(1);
    void *dest_bound = __softboundcets_load_bound_shadow_stack(1);
    __softboundcets_spatial_dereference_check(dest_base, dest_bound, dest,
                                              size);
#endif
#endif

    void *ret_ptr = strcpy(dest, src);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_strtok(char *str, const char *delim) {

    // TODO wrapper checks that str and delim are null terminated (safe strlen)

    char *ret_ptr = strtok(str, delim);
    if (!ret_ptr) {
        __softboundcets_store_null_return_metadata();
        return ret_ptr;
    }

    // In case the token is found, a null terminated string is returned.
    // Use this to determine the bounds with strlen (+1 for the null character).
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr) + 1), 1,
        __softboundcets_get_global_lock());
    return ret_ptr;
}

__WEAK_INLINE void __softboundcets_strdup_handler(void *ret_ptr) {

    key_type ptr_key = 0;
    lock_type ptr_lock = NULL;

    if (ret_ptr == NULL) {
        __softboundcets_store_null_return_metadata();
    } else {
        __mi_debug_printf("[strdup handler] str(n)dup malloced pointer %p\n",
                          ret_ptr);

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __softboundcets_memory_allocation(ret_ptr, &ptr_lock, &ptr_key);
#endif
        __softboundcets_store_return_metadata(
            ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr) + 1), ptr_key,
            ptr_lock);
    }
}

// strdup, allocates memory from the system using malloc, thus can be freed
__WEAK_INLINE char *softboundcets_strndup(const char *s, size_t n) {

    /* IMP: strndup just copies the string s */
    char *ret_ptr = strndup(s, n);
    __softboundcets_strdup_handler(ret_ptr);
    return ret_ptr;
}

// strdup, allocates memory from the system using malloc, thus can be freed
__WEAK_INLINE char *softboundcets_strdup(const char *s) {

    /* IMP: strdup just copies the string s */
    void *ret_ptr = strdup(s);

    __softboundcets_strdup_handler(ret_ptr);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets___strdup(const char *s) {

    void *ret_ptr = strdup(s);
    __softboundcets_strdup_handler(ret_ptr);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_strcat(char *dest, const char *src) {

    // TODO this should again use softboundcets_strlen for both, dest and src,
    // to determine how many bytes will be written to dest. Make sure to not add
    // the lengths without overflow checks. Check that dest is large enough to
    // hold the concatenated strings (use the dereference check function).
#if 0
  if(dest + strlen(dest) + strlen(src) > dest_bound){
    __mi_printf("overflow with strcat, dest = %p, strlen(dest)=%d,
            strlen(src)=%d, dest_bound=%p \n",
           dest, strlen(dest), strlen(src), dest_bound);
    __mi_fail();
  }
#endif

    char *ret_ptr = strcat(dest, src);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_strncat(char *dest, const char *src,
                                          size_t n) {
    // TODO missing checks (see also strcat)
    char *ret_ptr = strncat(dest, src, n);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_strncpy(char *dest, char *src, size_t n) {

#if __SOFTBOUNDCETS_WRAPPER_CHECKS
#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    // Make sure that dest is large enough to store n elements
    __softboundcets_wrapper_check_shadow_stack_ptr(1, dest, n);

    // TODO add check for src: either its size is at least n elements, or
    // safe_strlen(src) + 1 < n holds

    // This is more strict than the definition of strncpy.
    // n < src_bound - src is valid and defined as long as the src buffer is
    // nul-terminated.
    //  char* src_base = __softboundcets_load_base_shadow_stack(2);
    //  char* src_bound = __softboundcets_load_bound_shadow_stack(2);
    //  if(src < src_base || (src > src_bound -n) || (n > (size_t) src_bound)){
    //    __mi_fail();
    //  }
#endif
#endif

    char *ret_ptr = strncpy(dest, src, n);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_strstr(const char *haystack,
                                         const char *needle) {

    char *ret_ptr = strstr(haystack, needle);
    if (ret_ptr != NULL) {
        __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    } else {
        __softboundcets_store_null_return_metadata();
    }
    return ret_ptr;
}

#ifdef _GNU_SOURCE
__WEAK_INLINE char *softboundcets_strerror_r(int errnum, char *buf,
                                             size_t buf_len) {

    void *ret_ptr = strerror_r(errnum, buf, buf_len);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr) + 1), 1,
        __softboundcets_get_global_lock());
    return ret_ptr;
}
#endif

__WEAK_INLINE char *softboundcets_strerror(int errnum) {

    void *ret_ptr = strerror(errnum);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr) + 1), 1,
        __softboundcets_get_global_lock());
    return ret_ptr;
}

//===----------------------------------------------------------------------===//
//                            signal.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_kill(pid_t pid, int sig) {
    return kill(pid, sig);
}

__WEAK_INLINE int softboundcets_raise(int sig) { return raise(sig); }

__WEAK_INLINE sighandler_t softboundcets_signal(int signum,
                                                sighandler_t handler) {

    sighandler_t ptr = signal(signum, handler);
    __softboundcets_store_return_metadata((void *)ptr, (void *)ptr, 1,
                                          __softboundcets_get_global_lock());
    return ptr;
}

//===----------------------------------------------------------------------===//
//                    Memory Management Function Wrapper
//===----------------------------------------------------------------------===//

__WEAK_INLINE void *softboundcets_realloc(void *ptr, size_t size) {

    __mi_debug_printf("[realloc] ptr=%p\n", ptr);

    void *ret_ptr = realloc(ptr, size);
    __softboundcets_allocation_secondary_trie_allocate(ret_ptr);
    key_type ptr_key = 1;
    lock_type ptr_lock = __softboundcets_get_global_lock();

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    ptr_key = __softboundcets_load_key_shadow_stack(1);
    ptr_lock = __softboundcets_load_lock_shadow_stack(1);
#endif

    __softboundcets_store_return_metadata(ret_ptr, (char *)(ret_ptr) + size,
                                          ptr_key, ptr_lock);
    if (ret_ptr != ptr) {

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __softboundcets_check_remove_from_free_map(ptr_key, ptr);
        __softboundcets_add_to_free_map(ptr_key, ret_ptr);
#endif
        __softboundcets_copy_metadata(ret_ptr, ptr, size);
    }

    return ret_ptr;
}

// Generic helper for allocation functions that return a pointer to the
// allocated memory. It will store appropriate pointer metadata information to
// the shadow stack.
__WEAK_INLINE void *__softboundcets_generic_alloc(void *alloced_ptr,
                                                  size_t size) {

    if (!alloced_ptr) {
        __softboundcets_store_null_return_metadata();
        return alloced_ptr;
    }

    key_type ptr_key = 1;
    lock_type ptr_lock = NULL;

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_memory_allocation(alloced_ptr, &ptr_lock, &ptr_key);
#endif

    __softboundcets_store_return_metadata(
        alloced_ptr, (char *)alloced_ptr + size, ptr_key, ptr_lock);

    return alloced_ptr;
}

__WEAK_INLINE void *softboundcets_calloc(size_t nmemb, size_t size) {

    void *ret_ptr = calloc(nmemb, size);

    return __softboundcets_generic_alloc(ret_ptr, nmemb * size);
}

__WEAK_INLINE void *softboundcets_malloc(size_t size) {

    void *ret_ptr = malloc(size);

    return __softboundcets_generic_alloc(ret_ptr, size);
}

__WEAK_INLINE void *softboundcets_memalign(size_t alignment, size_t size) {

    void *ret_ptr = memalign(alignment, size);

    return __softboundcets_generic_alloc(ret_ptr, size);
}

__WEAK_INLINE void *softboundcets_aligned_alloc(size_t alignment, size_t size) {

    void *ret_ptr = aligned_alloc(alignment, size);

    return __softboundcets_generic_alloc(ret_ptr, size);
}

__WEAK_INLINE void *softboundcets_valloc(size_t size) {

    void *ret_ptr = valloc(size);

    return __softboundcets_generic_alloc(ret_ptr, size);
}

__WEAK_INLINE void *softboundcets_pvalloc(size_t size) {

    void *ret_ptr = pvalloc(size);

    // "The obsolete function pvalloc() is similar to valloc(), but rounds the
    // size of the allocation up to the next multiple of the system page size."
    if (size % sysconf(_SC_PAGESIZE)) {
        __mi_debug_printf("[pvalloc] Adapted size from %zu", size);
        size = ((size / sysconf(_SC_PAGESIZE)) + 1) * sysconf(_SC_PAGESIZE);
        __mi_debug_printf(" to %zu.\n", size);
    }

    return __softboundcets_generic_alloc(ret_ptr, size);
}

__WEAK_INLINE void *softboundcets_mmap(void *addr, size_t length, int prot,
                                       int flags, int fd, off_t offset) {

    void *ret_ptr = mmap(addr, length, prot, flags, fd, offset);
    if (ret_ptr == (void *)-1) {
        __softboundcets_store_null_return_metadata();
        return ret_ptr;
    }

    return __softboundcets_generic_alloc(ret_ptr, length);
}

__WEAK_INLINE int softboundcets_posix_memalign(void **memptr, size_t alignment,
                                               size_t size) {

    int ret = posix_memalign(memptr, alignment, size);

#if __SOFTBOUNDCETS_SPATIAL
    if (ret) {
        // The allocation failed, `memptr` stays unchanged.
        return ret;
    }
    __softboundcets_metadata_store(memptr, *memptr, ((char *)*memptr) + size);
#elif __SOFTBOUNDCETS_TEMPORAL
    __mi_fail_with_msg("posix_memalign wrapper: metadata store for "
                       "temporal safety not implemented");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __mi_fail_with_msg("posix_memalign metadata store for "
                       "temporal+spatial safety not implemented");
#endif

#if __SOFTBOUNDCETS_FREE_MAP
    __mi_fail_with_msg(
        "posix_memalign wrapper: free map entry setting not implemented");
#endif

    return ret;
}

__WEAK_INLINE void *softboundcets_memcpy(void *dest, const void *src,
                                         size_t n) {

    __softboundcets_wrapper_check_shadow_stack_ptr(1, (char *)dest, n);
    __softboundcets_wrapper_check_shadow_stack_ptr(2, (char *)src, n);

    void *ret_ptr = memcpy(dest, src, n);
    if (n > 0) {
        __softboundcets_copy_metadata(dest, src, n);
        __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    }
    return ret_ptr;
}

#ifdef _GNU_SOURCE

__WEAK_INLINE void *softboundcets_mempcpy(void *dest, const void *src,
                                          size_t n) {

    __softboundcets_wrapper_check_shadow_stack_ptr(1, (char *)dest, n);
    __softboundcets_wrapper_check_shadow_stack_ptr(2, (char *)src, n);

    void *ret_ptr = mempcpy(dest, src, n);
    if (n > 0) {
        __softboundcets_copy_metadata(dest, src, n);
        __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    }
    return ret_ptr;
}

#endif

__WEAK_INLINE void *softboundcets_memmove(void *dest, const void *src,
                                          size_t n) {

    __softboundcets_wrapper_check_shadow_stack_ptr(1, (char *)dest, n);
    __softboundcets_wrapper_check_shadow_stack_ptr(2, (char *)src, n);

    void *ret_ptr = memmove(dest, src, n);
    if (n > 0) {
        __softboundcets_copy_metadata(dest, src, n);
        __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    }
    return ret_ptr;
}

__WEAK_INLINE void *softboundcets_memset(void *s, int c, size_t n) {

    __softboundcets_wrapper_check_shadow_stack_ptr(1, (char *)s, n);

    void *ret_ptr = memset(s, c, n);
    __softboundcets_propagate_metadata_shadow_stack_from(1, 0);
    return ret_ptr;
}

__WEAK_INLINE void softboundcets_free(void *ptr) {
    // TODO shouldn't this make sure that the object is freeable and update the
    // list of freeable objects such that double frees can be avoided?
    free(ptr);
}

//===----------------------------------------------------------------------===//
//                      Time Related Library Wrappers
//===----------------------------------------------------------------------===//

//===----------------------------- time.h ---------------------------------===//

__WEAK_INLINE time_t softboundcets_time(time_t *t) { return time(t); }

__WEAK_INLINE time_t softboundcets_mktime(struct tm *tm) { return mktime(tm); }

__WEAK_INLINE clock_t softboundcets_clock(void) { return clock(); }

__WEAK_INLINE struct tm *softboundcets_localtime(const time_t *timep) {

    struct tm *ret_ptr = localtime(timep);
    __softboundcets_store_return_metadata(ret_ptr,
                                          (char *)ret_ptr + sizeof(struct tm),
                                          1, __softboundcets_get_global_lock());
    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_ctime(const time_t *timep) {

    char *ret_ptr = ctime(timep);

    if (ret_ptr == NULL) {
        __softboundcets_store_null_return_metadata();
    } else {
        __softboundcets_store_return_metadata(
            ret_ptr, ret_ptr + strlen(ret_ptr) + 1, 1,
            __softboundcets_get_global_lock());
    }
    return ret_ptr;
}

__WEAK_INLINE int softboundcets_gettimeofday(struct timeval *tv,
                                             struct timezone *tz) {
    return gettimeofday(tv, tz);
}

__WEAK_INLINE double softboundcets_difftime(time_t time1, time_t time0) {
    return difftime(time1, time0);
}

__WEAK_INLINE size_t softboundcets_strftime(char *s, size_t max,
                                            const char *format,
                                            const struct tm *tm) {

    return strftime(s, max, format, tm);
}

__WEAK_INLINE struct tm *softboundcets_gmtime(const time_t *timep) {

    struct tm *temp = gmtime(timep);

    if (!temp) {
        __softboundcets_store_null_return_metadata();
        return temp;
    }

    __softboundcets_store_return_metadata(temp, temp + sizeof(struct tm), 1,
                                          __softboundcets_get_global_lock());

    return temp;
}

__WEAK_INLINE time_t softboundcets_timegm(struct tm *tm) { return timegm(tm); }

__WEAK_INLINE int softboundcets_utimes(const char *filename,
                                       const struct timeval times[2]) {

    return utimes(filename, times);
}

__WEAK_INLINE int softboundcets_clock_gettime(clockid_t clk_id,
                                              struct timespec *tp) {
    return clock_gettime(clk_id, tp);
}

#if 0
__WEAK_INLINE int softboundcets_futimesat(int dirfd, const char *pathname,
                                          const struct timeval times[2]){

  return futimesat(dirfd, pathname, times);
}
#endif

//===---------------------------- times.h ---------------------------------===//

__WEAK_INLINE clock_t softboundcets_times(struct tms *buf) {
    return times(buf);
}

//===---------------------------- utime.h ---------------------------------===//

__WEAK_INLINE
int softboundcets_utime(const char *filename, const struct utimbuf *times) {

#if __SOFTBOUNDCETS_WRAPPER_CHECKS
    if (times) {
        __softboundcets_wrapper_check_shadow_stack_ptr(1, (char *)times,
                                                       sizeof(*times));
    }
#endif

    return utime(filename, times);
}

//===----------------------------------------------------------------------===//
//                          ctype.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_toupper(int c) { return toupper(c); }

__WEAK_INLINE int softboundcets_tolower(int c) { return tolower(c); }

#if defined(__linux__)
__WEAK_INLINE unsigned short const **softboundcets___ctype_b_loc(void) {

    unsigned short const **ret_ptr = __ctype_b_loc();

    // Exactly this pointer is dereferenceable
    __softboundcets_store_return_metadata((void *)ret_ptr,
                                          (void *)(ret_ptr + 1), 1,
                                          __softboundcets_get_global_lock());
#if __SOFTBOUNDCETS_SPATIAL
    // Store metadata for the pointer that ret_ptr points to.
    // From the definition of __ctype_to_upper:
    // "The array shall contain a total of 384 characters, and can be
    // indexed with any signed or unsigned char (i.e. with an index value
    // between -128 and 255)"
    __softboundcets_metadata_store(ret_ptr, (void *)(*ret_ptr - 128),
                                   (void *)(*ret_ptr + 256));
#elif __SOFTBOUNDCETS_TEMPORAL
    __softboundcets_metadata_store(ret_ptr, 1,
                                   __softboundcets_get_global_lock());
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_metadata_store(ret_ptr, (void *)(*ret_ptr - 128),
                                   (void *)(*ret_ptr + 256), 1,
                                   __softboundcets_get_global_lock());
#endif
    return ret_ptr;
}

void __softboundcets_handle_ctype_upper_lower(const int **ptr) {

    // Exactly this pointer is dereferenceable
    __softboundcets_store_return_metadata((void *)ptr, (void *)(ptr + 1), 1,
                                          __softboundcets_get_global_lock());
#if __SOFTBOUNDCETS_SPATIAL
    // Store metadata for the pointer that ret_ptr points to.
    // From the definition of __ctype_to_upper/__ctype_to_lower:
    // "The array shall contain a total of 384 characters, and can be
    // indexed with any signed or unsigned char (i.e. with an index value
    // between -128 and 255)"
    __softboundcets_metadata_store(ptr, (void *)(*ptr - 128),
                                   (void *)(*ptr + 256));
#elif __SOFTBOUNDCETS_TEMPORAL
    __softboundcets_metadata_store(ptr, 1, __softboundcets_get_global_lock());
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_metadata_store(ptr, (void *)(*ptr - 128),
                                   (void *)(*ptr + 256), 1,
                                   __softboundcets_get_global_lock());
#endif
}

__WEAK_INLINE const int **softboundcets___ctype_toupper_loc(void) {

    const int **ret_ptr = __ctype_toupper_loc();
    __softboundcets_handle_ctype_upper_lower(ret_ptr);

    return ret_ptr;
}

__WEAK_INLINE const int **softboundcets___ctype_tolower_loc(void) {

    const int **ret_ptr = __ctype_tolower_loc();
    __softboundcets_handle_ctype_upper_lower(ret_ptr);

    return ret_ptr;
}
#endif

//===----------------------------------------------------------------------===//
//                          select.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_select(int nfds, fd_set *readfds,
                                       fd_set *writefds, fd_set *exceptfds,
                                       struct timeval *timeout) {
    return select(nfds, readfds, writefds, exceptfds, timeout);
}

//===----------------------------------------------------------------------===//
//                          libintl.h Wrappers
//===----------------------------------------------------------------------===//

#if defined(__linux__)
__WEAK_INLINE char *softboundcets_textdomain(const char *domainname) {

    void *ret_ptr = textdomain(domainname);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr)), 1,
        __softboundcets_get_global_lock());

    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_bindtextdomain(const char *domainname,
                                                 const char *dirname) {

    void *ret_ptr = bindtextdomain(domainname, dirname);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr)), 1,
        __softboundcets_get_global_lock());

    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_gettext(const char *msgid) {

    void *ret_ptr = gettext(msgid);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr)), 1,
        __softboundcets_get_global_lock());

    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_dcngettext(const char *domainname,
                                             const char *msgid,
                                             const char *msgid_plural,
                                             unsigned long int n,
                                             int category) {

    void *ret_ptr = dcngettext(domainname, msgid, msgid_plural, n, category);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr)), 1,
        __softboundcets_get_global_lock());

    return ret_ptr;
}

__WEAK_INLINE char *softboundcets_dcgettext(const char *domainname,
                                            const char *msgid, int category) {

    void *ret_ptr = dcgettext(domainname, msgid, category);
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + strlen(ret_ptr)), 1,
        __softboundcets_get_global_lock());

    return ret_ptr;
}

//===----------------------------------------------------------------------===//
//                          netdb.h Wrappers
//===----------------------------------------------------------------------===//

/* IMP: struct hostent may have pointers in the structure being returned,
   we need to store the metadata for all those pointers */
__WEAK_INLINE
struct hostent *softboundcets_gethostbyname(const char *name) {

    struct hostent *ret_ptr = gethostbyname(name);

    void *ret_bound = ret_ptr + sizeof(struct hostent);
    __softboundcets_store_return_metadata(ret_ptr, ret_bound, 1,
                                          __softboundcets_get_global_lock());

    return ret_ptr;
}

#endif

#if defined(__linux__)
__WEAK_INLINE int *softboundcets___errno_location() {
    void *ret_ptr = (int *)__errno_location();
    __mi_debug_printf("[errno_location]");
    __softboundcets_store_return_metadata(
        ret_ptr, (void *)((char *)ret_ptr + sizeof(int *)), 1,
        __softboundcets_get_global_lock());

    return ret_ptr;
}
#endif

//===----------------------------------------------------------------------===//
//                            qsort...
//===----------------------------------------------------------------------===//

/* This is a custom implementation of qsort */
static int
compare_elements_helper(void *base, size_t element_size, int idx1, int idx2,
                        int (*comparer)(const void *, const void *)) {

    char *base_bytes = base;
    // Make sure the bounds for the pointer arguments of the comparator are
    // stored on the shadow stack

    // First, load the values from the shadow stack
#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    void *ptr_base = __softboundcets_load_base_shadow_stack(0);
    void *ptr_bound = __softboundcets_load_bound_shadow_stack(0);
#endif
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    lock_type ptr_lock = __softboundcets_load_lock_shadow_stack(0);
    key_type ptr_key = __softboundcets_load_key_shadow_stack(0);
#endif

    // Allocate space for the next call
    __softboundcets_allocate_shadow_stack_space(2);

    // Add the loaded values to the newly allocated region
#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_store_base_shadow_stack(ptr_base, 0);
    __softboundcets_store_bound_shadow_stack(ptr_bound, 0);
    __softboundcets_store_base_shadow_stack(ptr_base, 1);
    __softboundcets_store_bound_shadow_stack(ptr_bound, 1);
#endif
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_store_lock_shadow_stack(ptr_lock, 0);
    __softboundcets_store_key_shadow_stack(ptr_key, 0);
    __softboundcets_store_lock_shadow_stack(ptr_lock, 1);
    __softboundcets_store_key_shadow_stack(ptr_key, 1);
#endif

    int res = comparer(&base_bytes[idx1 * element_size],
                       &base_bytes[idx2 * element_size]);

    __softboundcets_deallocate_shadow_stack_space();

    return res;
}

#define element_less_than(i, j)                                                \
    (compare_elements_helper(base, element_size, (i), (j), comparer) < 0)

static void exchange_elements_helper(void *base, size_t element_size, int idx1,
                                     int idx2) {

    char *base_bytes = base;
    size_t i;

    for (i = 0; i < element_size; i++) {
        char temp = base_bytes[idx1 * element_size + i];
        base_bytes[idx1 * element_size + i] =
            base_bytes[idx2 * element_size + i];
        base_bytes[idx2 * element_size + i] = temp;
    }

    for (i = 0; i < element_size; i += 8) {

#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        void *base_idx1;
        void *bound_idx1;

        void *base_idx2;
        void *bound_idx2;
#endif

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        key_type key_idx1 = 1;
        key_type key_idx2 = 1;

        lock_type lock_idx1 = NULL;
        lock_type lock_idx2 = NULL;
#endif

        char *addr_idx1 = &base_bytes[idx1 * element_size + i];
        char *addr_idx2 = &base_bytes[idx2 * element_size + i];

#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_load(addr_idx1, &base_idx1, &bound_idx1);
        __softboundcets_metadata_load(addr_idx2, &base_idx2, &bound_idx2);

        __softboundcets_metadata_store(addr_idx1, base_idx2, bound_idx2);
        __softboundcets_metadata_store(addr_idx2, base_idx1, bound_idx1);

#elif __SOFTBOUNDCETS_TEMPORAL

        __softboundcets_metadata_load(addr_idx1, &key_idx1, &lock_idx1);
        __softboundcets_metadata_load(addr_idx2, &key_idx2, &lock_idx2);

        __softboundcets_metadata_store(addr_idx1, key_idx2, lock_idx2);
        __softboundcets_metadata_store(addr_idx2, key_idx1, lock_idx1);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

        __softboundcets_metadata_load(addr_idx1, &base_idx1, &bound_idx1,
                                      &key_idx1, &lock_idx1);
        __softboundcets_metadata_load(addr_idx2, &base_idx2, &bound_idx2,
                                      &key_idx2, &lock_idx2);

        __softboundcets_metadata_store(addr_idx1, base_idx2, bound_idx2,
                                       key_idx2, lock_idx2);
        __softboundcets_metadata_store(addr_idx2, base_idx1, bound_idx1,
                                       key_idx1, lock_idx1);

#endif
    }
}

#define exchange_elements(i, j)                                                \
    (exchange_elements_helper(base, element_size, (i), (j)))

#define MIN_QSORT_LIST_SIZE 32

__WEAK__
void my_qsort(void *base, size_t num_elements, size_t element_size,
              int (*comparer)(const void *, const void *)) {

    size_t i;

    for (i = 0; i < num_elements; i++) {
        int j;
        for (j = i - 1; j >= 0; j--) {
            if (element_less_than(j, j + 1))
                break;
            exchange_elements(j, j + 1);
        }
    }
    /* may be implement qsort here */
}

__WEAK_INLINE void softboundcets_qsort(void *base, size_t nmemb, size_t size,
                                       int (*compar)(const void *,
                                                     const void *)) {

    my_qsort(base, nmemb, size, compar);
}

//===----------------------------------------------------------------------===//
//                          obstack.h Wrappers
//===----------------------------------------------------------------------===//

#if defined(__linux__)

__WEAK_INLINE
void softboundcets__obstack_newchunk(struct obstack *obj, int b) {

    _obstack_newchunk(obj, b);
}

__WEAK_INLINE
int softboundcets__obstack_begin(struct obstack *obj, int a, int b,
                                 void *(foo)(long), void(bar)(void *)) {
    return _obstack_begin(obj, a, b, foo, bar);
}

__WEAK_INLINE
void softboundcets_obstack_free(struct obstack *obj, void *object) {
    obstack_free(obj, object);
}

//===----------------------------------------------------------------------===//
//                         langinfo.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
char *softboundcets_nl_langinfo(nl_item item) {

    char *ret_ptr = nl_langinfo(item);
    __softboundcets_store_return_metadata(ret_ptr,
                                          ret_ptr + strlen(ret_ptr) + 1, 1,
                                          __softboundcets_get_global_lock());

    return ret_ptr;
}

#endif

#if __SOFTBOUNDCETS_SPATIAL

// TODO those are written with only spatial safety in mind.
// Find out which generalize, make them available to configurations that include
// temporal safety.

//===----------------------------------------------------------------------===//
//                             GLIBC Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
int softboundcets_connect(int sockfd, const struct sockaddr *addr,
                          socklen_t addrlen) {
    return connect(sockfd, addr, addrlen);
}

__WEAK_INLINE
char *softboundcets_inet_ntoa(struct in_addr in) {
    char *ret_ptr = inet_ntoa(in);
    __softboundcets_store_return_metadata(ret_ptr, (void *)(ret_ptr + 18), 1,
                                          __softboundcets_get_global_lock());
    return ret_ptr;
}

__WEAK_INLINE
int softboundcets_initgroups(const char *user, gid_t group) {
    return initgroups(user, group);
}

__WEAK_INLINE
ssize_t softboundcets_sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return sendmsg(sockfd, msg, flags);
}

__WEAK_INLINE
ssize_t softboundcets_writev(int fd, const struct iovec *iov, int iovcnt) {
    return writev(fd, iov, iovcnt);
}

#ifdef _GNU_SOURCE
__WEAK_INLINE
int softboundcets_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                          int flags) {
    return accept4(sockfd, addr, addrlen, flags);
}
#endif

#ifdef _XOPEN_SOURCE
__WEAK_INLINE
char *softboundcets_crypt(const char *key, const char *salt) {
    char *ret_ptr = crypt(key, salt);
    __softboundcets_store_return_metadata(ret_ptr, (void *)(ret_ptr + 13), 1,
                                          __softboundcets_get_global_lock());
    return ret_ptr;
}
#endif

#ifdef _GNU_SOURCE
__WEAK_INLINE
char *softboundcets_crypt_r(const char *key, const char *salt,
                            struct crypt_data *data) {
    char *ret_ptr = crypt_r(key, salt, data);
    if (ret_ptr) {
        __softboundcets_propagate_metadata_shadow_stack_from(3, 0);
    } else {
        __softboundcets_store_null_return_metadata();
    }
    return ret_ptr;
}
#endif

//===----------------------------------------------------------------------===//
//                         Dynamic Linker wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
void *softboundcets_dlopen(const char *filename, int flag) {
    void *ret_ptr = dlopen(filename, flag);
    __softboundcets_generic_wide_bounds_return(ret_ptr);
    return ret_ptr;
}

__WEAK_INLINE
void *softboundcets_dlsym(void *handle, const char *symbol) {
    void *ret_ptr = dlsym(handle, symbol);
    __softboundcets_generic_wide_bounds_return(ret_ptr);
    return ret_ptr;
}

#ifdef _GNU_SOURCE
__WEAK_INLINE
int softboundcets_dladdr(void *addr, Dl_info *info) {
    int ret = dladdr(addr, info);

    void *addr1 = &info->dli_sname;
    void *addr2 = &info->dli_saddr;
    // dladdr returns nonzero on _success_
    if (ret) {
        __softboundcets_metadata_store_generic_wide_bounds(addr1);
        __softboundcets_metadata_store_generic_wide_bounds(addr2);
    } else {
        // dli_sname and dli_saddr are set to NULL, this should not be
        // accessed
        __softboundcets_metadata_store_generic_null_bounds(addr1);
        __softboundcets_metadata_store_generic_null_bounds(addr2);
    }
    return ret;
}
#endif

__WEAK_INLINE
int softboundcets_dlclose(void *handle) { return dlclose(handle); }

//----------------------------------------------------------------------------//

__WEAK_INLINE
int softboundcets_epoll_ctl(int epfd, int op, int fd,
                            struct epoll_event *event) {
    return epoll_ctl(epfd, op, fd, event);
}

__WEAK_INLINE
int softboundcets_epoll_wait(int epfd, struct epoll_event *events,
                             int maxevents, int timeout) {
    return epoll_wait(epfd, events, maxevents, timeout);
}

__WEAK_INLINE
FILE *softboundcets_freopen(const char *path, const char *mode, FILE *stream) {
    FILE *ret_ptr = freopen(path, mode, stream);
    if (ret_ptr) {
        __softboundcets_propagate_metadata_shadow_stack_from(3, 0);
    } else {
        __softboundcets_store_null_return_metadata();
    }
    return ret_ptr;
}

__WEAK_INLINE
key_t softboundcets_ftok(const char *pathname, int proj_id) {
    return ftok(pathname, proj_id);
}

__WEAK_INLINE
int softboundcets_gethostname(char *name, size_t len) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, name, len);
    return gethostname(name, len);
}

__WEAK_INLINE
int softboundcets_getaddrinfo(const char *node, const char *service,
                              const struct addrinfo *hints,
                              struct addrinfo **res) {
    int ret = getaddrinfo(node, service, hints, res);

    // getaddrinfo() returns 0 if it succeeds
    if (ret != 0) {
        return ret;
    }

    // Store the metadata for the first element of the linked list
    __mi_debug_printf("[getaddrinfo] Store metadata for the first element\n");
    __softboundcets_metadata_store(res, *res,
                                   ((char *)*res) + sizeof(struct addrinfo));

    struct addrinfo *next = *res;
    while (next) {
        // Store metadata information on the pointer in addrinfo

        // ai_addr is a pointer to a struct with no further indirection
        void *addr_of_addr = &(next->ai_addr);

        __mi_debug_printf("[getaddrinfo] Store ai_addr info\n");

        if (next->ai_addr) {
            __softboundcets_metadata_store(addr_of_addr, next->ai_addr,
                                           ((char *)next->ai_addr) +
                                               next->ai_addrlen);
        } else {
            __softboundcets_metadata_store_generic_null_bounds(addr_of_addr);
        }

        // ai_canonname is pointer to a name of unknown length
        void *addr_of_name = &(next->ai_canonname);

        __mi_debug_printf("[getaddrinfo] Store ai_canonname info\n");

        if (next->ai_canonname) {
            __softboundcets_metadata_store_generic_wide_bounds(addr_of_name);
        } else {
            __softboundcets_metadata_store_generic_null_bounds(addr_of_name);
        }

        // ai_next links to the next addrinfo struct
        void *addr_of_next_ptr = &(next->ai_next);

        __mi_debug_printf("[getaddrinfo] Store ai_next info\n");

        if (next->ai_next) {
            __softboundcets_metadata_store(addr_of_next_ptr, next->ai_next,
                                           ((char *)next->ai_next) +
                                               sizeof(struct addrinfo));
        } else {
            __softboundcets_metadata_store_generic_null_bounds(
                addr_of_next_ptr);
        }

        next = next->ai_next;
    }

    return ret;
}

__WEAK_INLINE
void softboundcets_freeaddrinfo(struct addrinfo *res) { freeaddrinfo(res); }

__WEAK_INLINE
pid_t softboundcets_wait3(int *wstatus, int options, struct rusage *rusage) {
    pid_t ret = wait3(wstatus, options, rusage);

    return ret;
}

__WEAK_INLINE
int softboundcets_setsockopt(int sockfd, int level, int optname,
                             const void *optval, socklen_t optlen) {

    if (optval) {
        __softboundcets_wrapper_check_shadow_stack_ptr(0, optval, optlen);
    }
    int ret = setsockopt(sockfd, level, optname, optval, optlen);
    return ret;
}

__WEAK_INLINE
int softboundcets_getsockname(int sockfd, struct sockaddr *addr,
                              socklen_t *addrlen) {

    if (addr) {
        __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)addr,
                                                       *addrlen);
    }

    int ret = getsockname(sockfd, addr, addrlen);
    return ret;
}

__WEAK_INLINE
int softboundcets_getnameinfo(const struct sockaddr *sa, socklen_t salen,
                              char *host, size_t hostlen, char *serv,
                              size_t servlen, int flags) {

    // From the man page:
    // The caller can specify that no hostname (or no service name) is
    // required by providing a NULL host (or serv) argument or a zero
    // hostlen (or servlen) argument.

    if (hostlen && host) {
        __softboundcets_wrapper_check_shadow_stack_ptr(1, host, hostlen);
    }

    if (serv && servlen) {
        __softboundcets_wrapper_check_shadow_stack_ptr(2, serv, servlen);
    }

    int ret = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
    return ret;
}

__WEAK_INLINE
void softboundcets_openlog(const char *ident, int option, int facility) {
    openlog(ident, option, facility);
}

__WEAK_INLINE
void softboundcets_closelog(void) { closelog(); }

__WEAK_INLINE
char *softboundcets_getpass(const char *prompt) {
    // From the man page:
    // This function is obsolete. Do not use it.
    char *ret = getpass(prompt);

    if (ret) {
        // The length seems to vary depending on the exact implementation.
        // libc4/5: 128 byte, glibc: unlimited, SUSv2: PASS_MAX in limits.h
        __softboundcets_generic_wide_bounds_return(ret);
    } else {
        __softboundcets_store_null_return_metadata();
    }

    return ret;
}

__WEAK_INLINE
int softboundcets_madvise(void *addr, size_t length, int advice) {
    __softboundcets_wrapper_check_shadow_stack_ptr(0, addr, length);
    int ret = madvise(addr, length, advice);
    return ret;
}

__WEAK_INLINE
int softboundcets_munmap(void *addr, size_t length) {
    return munmap(addr, length);
}

//===----------------------------------------------------------------------===//
//                             UUID Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
void softboundcets_uuid_generate(uuid_t out) { uuid_generate(out); }

#endif

//===----------------------------------------------------------------------===//
//                          sys/wait.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE
pid_t softboundcets_wait(int *status) {
#if __SOFTBOUNDCETS_WRAPPER_CHECKS
    if (status) {
        __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)status,
                                                       sizeof(*status));
    }
#endif
    return wait(status);
}

__WEAK_INLINE
pid_t softboundcets_waitpid(pid_t pid, int *status, int options) {
#if __SOFTBOUNDCETS_WRAPPER_CHECKS
    if (status) {
        __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)status,
                                                       sizeof(*status));
    }
#endif
    return waitpid(pid, status, options);
}

__WEAK_INLINE
int softboundcets_waitid(idtype_t idtype, id_t id, siginfo_t *infop,
                         int options) {

#if __SOFTBOUNDCETS_WRAPPER_CHECKS
    // https://man7.org/linux/man-pages/man2/wait.2.html (BUGS section):
    // According to POSIX.1-2008, an application calling waitid() must
    // ensure that infop points to a siginfo_t structure (i.e., that it
    // is a non-null pointer).  On Linux, if infop is NULL, waitid()
    // succeeds, and returns the process ID of the waited-for child.
    // Applications should avoid relying on this inconsistent,
    // nonstandard, and unnecessary feature.
    if (!infop) {
        __mi_fail_with_msg("Calling `waitid` with a null pointer is invalid.");
    }

    __softboundcets_wrapper_check_shadow_stack_ptr(0, (char *)infop,
                                                   sizeof(*infop));
#endif
    return waitid(idtype, id, infop, options);
}

//===----------------------------------------------------------------------===//
//                           setjmp.h Wrappers
//===----------------------------------------------------------------------===//

__WEAK_INLINE int softboundcets_setjmp(jmp_buf env) { return setjmp(env); }

__WEAK_INLINE __SOFTBOUNDCETS_NORETURN void softboundcets_longjmp(jmp_buf env,
                                                                  int val) {
    longjmp(env, val);
}

#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_C_SOURCE
__WEAK_INLINE int softboundcets_sigsetjmp(sigjmp_buf env, int savesigs) {
    return sigsetjmp(env, savesigs);
}

__WEAK_INLINE __SOFTBOUNDCETS_NORETURN void
softboundcets_siglongjmp(sigjmp_buf env, int val) {
    siglongjmp(env, val);
}
#endif
