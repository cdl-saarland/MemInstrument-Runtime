#pragma once

extern size_t sizes[];

uintptr_t _ptr_index(void *ptr);
uintptr_t _ptr_base(void *ptr);
uintptr_t _ptr_size(void *ptr);
void __check_deref(void *ptr, uintptr_t base, uintptr_t size);