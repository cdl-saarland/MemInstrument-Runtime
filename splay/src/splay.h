#pragma once

void __splay_check_inbounds(void* witness, void* ptr);
void __splay_check_inbounds_named(void* witness, void* ptr);

void __splay_check_dereference(void* witness, void* ptr, size_t sz);

void __splay_check_dereference_named(void* witness, void* ptr, size_t sz, char* name);

uintptr_t __splay_get_lower(void* witness);

uintptr_t __splay_get_upper(void* witness);

void __splay_alloc(void* ptr, size_t sz);

void __splay_alloc_or_merge(void* ptr, size_t sz);

void __splay_alloc_or_replace(void* ptr, size_t sz);

void __splay_free(void* ptr);
