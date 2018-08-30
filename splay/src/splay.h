#pragma once
#include <stddef.h>
#include <stdint.h>

void __setup_splay(void);

void __splay_insertGlobalsFromBinary(void);

void __splay_check_inbounds_named(void *witness, void *ptr, char *name);

void __splay_check_inbounds(void *witness, void *ptr);

void __splay_check_dereference_named(void *witness, void *ptr, size_t sz,
                                     char *name);

void __splay_check_dereference(void *witness, void *ptr, size_t sz);

uintptr_t __splay_get_lower(void *witness);

void *__splay_get_lower_as_ptr(void *witness);

void *__splay_get_base(void *witness);

uintptr_t __splay_get_upper(void *witness);

void *__splay_get_upper_as_ptr(void *witness);

uintptr_t __splay_get_maxbyteoffset(void *witness);

void __splay_alloc_or_merge_with_msg(void *ptr, size_t sz, const char *msg);

void __splay_alloc_or_merge(void *ptr, size_t sz);

void __splay_alloc_with_msg(void *ptr, size_t sz, const char *msg);

void __splay_alloc(void *ptr, size_t sz);

void __splay_alloc_or_replace_with_msg(void *ptr, size_t sz, const char *msg);

void __splay_alloc_or_replace(void *ptr, size_t sz);

void __splay_free_with_msg(void *ptr, const char *msg);

void __splay_free(void *ptr);
