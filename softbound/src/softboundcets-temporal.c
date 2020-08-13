#include "softboundcets-temporal.h"

#include <assert.h>
#include <stdio.h>

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_TEMPORAL

extern size_t __softboundcets_key_id_counter;
extern size_t *__softboundcets_lock_next_location;
extern size_t *__softboundcets_lock_new_location;

//===----------------------------- Checks ---------------------------------===//

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
__WEAK_INLINE void
__softboundcets_temporal_load_dereference_check(void *pointer_lock, size_t key,
                                                void *base, void *bound) {
#else
__WEAK_INLINE void
__softboundcets_temporal_load_dereference_check(void *pointer_lock,
                                                size_t key) {
#endif

#if 0
  /* URGENT: I should think about removing this condition check */
  if(!pointer_lock){
    __softboundcets_printf("Temporal lock null\n");
    __softboundcets_abort();
    return;
  }

#endif

    size_t temp = *((size_t *)pointer_lock);

    if (temp != key) {
        __softboundcets_printf(
            "[TLDC] Key mismatch key = %zx, *lock=%zx, next_ptr =%zx\n", key,
            temp, __softboundcets_lock_next_location);
        __softboundcets_abort();
    }
}

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
__WEAK_INLINE void
__softboundcets_temporal_store_dereference_check(void *pointer_lock, size_t key,
                                                 void *base, void *bound) {
#else
__WEAK_INLINE void
__softboundcets_temporal_store_dereference_check(void *pointer_lock,
                                                 size_t key) {
#endif

#if 0
  if(!pointer_lock){
    __softboundcets_printf("lock null?");
    __softboundcets_abort();
  }
#endif

    size_t temp = *((size_t *)pointer_lock);

    if (temp != key) {
        __softboundcets_printf("[TSDC] Key mismatch, key = %zx, *lock=%zx\n",
                               key, temp);
        __softboundcets_abort();
    }
}

//===-------------------- Shadow Stack Manipulation -----------------------===//

__WEAK_INLINE size_t __softboundcets_load_key_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __KEY_INDEX;
    size_t *key_ptr = (__softboundcets_shadow_stack_ptr + count);
    size_t key = *key_ptr;
    return key;
}

__WEAK_INLINE void *__softboundcets_load_lock_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __LOCK_INDEX;
    size_t *lock_ptr = (__softboundcets_shadow_stack_ptr + count);
    void *lock = *((void **)lock_ptr);
    return lock;
}

__WEAK_INLINE void __softboundcets_store_key_shadow_stack(size_t key,
                                                          int arg_no) {
    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __KEY_INDEX;
    size_t *key_ptr = (__softboundcets_shadow_stack_ptr + count);

    *(key_ptr) = key;
}

__WEAK_INLINE void __softboundcets_store_lock_shadow_stack(void *lock,
                                                           int arg_no) {
    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __LOCK_INDEX;
    void **lock_ptr = (void **)(__softboundcets_shadow_stack_ptr + count);

    *(lock_ptr) = lock;
}

//===----------------------- Allocation related  --------------------------===//

__WEAK_INLINE void __softboundcets_stack_memory_deallocation(size_t ptr_key) {

#ifndef __SOFTBOUNDCETS_CONSTANT_STACK_KEY_LOCK

    __softboundcets_stack_temporal_space_begin--;
    *(__softboundcets_stack_temporal_space_begin) = 0;

#endif

    return;
}

__WEAK_INLINE void __softboundcets_memory_deallocation(void *ptr_lock,
                                                       size_t ptr_key) {

    __softboundcets_debug_printf(
        "[Hdealloc] pointer_lock = %p, *pointer_lock=%zx\n", ptr_lock,
        *((size_t *)ptr_lock));

#if 0
  if(!ptr_lock)
    return;
#endif

    *((size_t *)ptr_lock) = 0;
    *((void **)ptr_lock) = __softboundcets_lock_next_location;
    __softboundcets_lock_next_location = ptr_lock;
}

__WEAK_INLINE void *__softboundcets_allocate_lock_location() {

    void *temp = NULL;
    if (__softboundcets_lock_next_location == NULL) {

#if __SOFTBOUNDCETS_DEBUG
        // TODO not sure if all of this is only to be done when debugging
        __softboundcets_printf("[lock_allocate] new_lock_location=%p\n",
                               __softboundcets_lock_new_location);

        if (__softboundcets_lock_new_location >
            __softboundcets_temporal_space_begin +
                __SOFTBOUNDCETS_N_TEMPORAL_ENTRIES) {
            __softboundcets_printf(
                "[lock_allocate] out of temporal free entries \n");
            __softboundcets_abort();
        }
#endif

        return __softboundcets_lock_new_location++;
    } else {

        temp = __softboundcets_lock_next_location;

        __softboundcets_debug_printf("[lock_allocate] next_lock_location=%p\n",
                                     temp);

        __softboundcets_lock_next_location =
            *((void **)__softboundcets_lock_next_location);
        return temp;
    }
}

__WEAK_INLINE void __softboundcets_stack_memory_allocation(void **ptr_lock,
                                                           size_t *ptr_key) {

#ifdef __SOFTBOUNDCETS_CONSTANT_STACK_KEY_LOCK
    *((size_t *)ptr_key) = 1;
    *((size_t **)ptr_lock) = __softboundcets_global_lock;
#else
    size_t temp_id = __softboundcets_key_id_counter++;
    *((size_t **)ptr_lock) =
        (size_t *)__softboundcets_stack_temporal_space_begin++;
    *((size_t *)ptr_key) = temp_id;
    **((size_t **)ptr_lock) = temp_id;
#endif
}

__WEAK_INLINE void __softboundcets_memory_allocation(void *ptr, void **ptr_lock,
                                                     size_t *ptr_key) {

    size_t temp_id = __softboundcets_key_id_counter++;

    *((size_t **)ptr_lock) = (size_t *)__softboundcets_allocate_lock_location();
    *((size_t *)ptr_key) = temp_id;
    **((size_t **)ptr_lock) = temp_id;

    __softboundcets_add_to_free_map(temp_id, ptr);
    __softboundcets_allocation_secondary_trie_allocate(ptr);

    __softboundcets_debug_printf(
        "[mem_alloc] lock = %p, ptr_key = %p, key = %zx\n", ptr_lock, ptr_key,
        temp_id);
}

__WEAK_INLINE void *__softboundcets_get_global_lock() {
    return __softboundcets_global_lock;
}

__WEAK_INLINE void __softboundcets_add_to_free_map(size_t ptr_key, void *ptr) {

#if !__SOFTBOUNDCETS_FREE_MAP
    return;
#endif

    assert(ptr != NULL);

    size_t counter = 0;
    while (1) {
        size_t index = (ptr_key + counter) % __SOFTBOUNDCETS_N_FREE_MAP_ENTRIES;
        size_t *entry_ptr = &__softboundcets_free_map_table[index];
        size_t tag = *entry_ptr;

        if (tag == 0 || tag == 2) {

            __softboundcets_debug_printf("entry_ptr=%zx, ptr=%zx, key=%zx\n",
                                         entry_ptr, ptr, ptr_key);
            *entry_ptr = (size_t)(ptr);
            return;
        }
        if (counter >= (__SOFTBOUNDCETS_N_FREE_MAP_ENTRIES)) {
#ifndef __NOSIM_CHECKS
            __softboundcets_abort();
#else
            break;
#endif
        }
        counter++;
    }
    return;
}

__WEAK_INLINE void __softboundcets_check_remove_from_free_map(size_t ptr_key,
                                                              void *ptr) {

#if !__SOFTBOUNDCETS_FREE_MAP
    return;
#endif

#if 0
  if(ptr_key == 1){
    __softboundcets_printf(("freeing a global key\n");
    __softboundcets_abort();
  }
#endif

    size_t counter = 0;
    while (1) {
        size_t index = (ptr_key + counter) % __SOFTBOUNDCETS_N_FREE_MAP_ENTRIES;
        size_t *entry_ptr = &__softboundcets_free_map_table[index];
        size_t tag = *entry_ptr;

        if (tag == 0) {
#ifndef __NOSIM_CHECKS
            __softboundcets_printf("free map does not have the key\n");
            __softboundcets_abort();
#else
            break;
#endif
        }

        if (tag == (size_t)ptr) {
            *entry_ptr = 2;
            return;
        }

        if (counter >= __SOFTBOUNDCETS_N_FREE_MAP_ENTRIES) {
#ifndef __NOSIM_CHECKS
            __softboundcets_printf("free map out of entries\n");
            __softboundcets_abort();
#else
            break;
#endif
        }
        counter++;
    }
    return;
}

#endif
