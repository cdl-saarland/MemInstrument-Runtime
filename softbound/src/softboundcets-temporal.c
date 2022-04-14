#include "softboundcets-temporal.h"

#include "softboundcets-common.h"

#include "fail_function.h"

#include <assert.h>
#include <sys/mman.h>

lock_type __softboundcets_global_lock = 0;

__WEAK_INLINE lock_type __softboundcets_get_global_lock() {
    return __softboundcets_global_lock;
}

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_TEMPORAL

static const size_t __SOFTBOUNDCETS_N_TEMPORAL_ENTRIES =
    ((size_t)64 * (size_t)1024 * (size_t)1024);

static const size_t __SOFTBOUNDCETS_N_STACK_TEMPORAL_ENTRIES =
    ((size_t)1024 * (size_t)64);
static const size_t __SOFTBOUNDCETS_N_GLOBAL_LOCK_SIZE =
    ((size_t)1024 * (size_t)32);

// 256 Million simultaneous objects
static const size_t __SOFTBOUNDCETS_N_FREE_MAP_ENTRIES =
    ((size_t)32 * (size_t)1024 * (size_t)1024);

size_t *__softboundcets_free_map_table = NULL;

size_t *__softboundcets_lock_next_location = NULL;
size_t *__softboundcets_lock_new_location = NULL;
size_t __softboundcets_key_id_counter = 2;

size_t *__softboundcets_temporal_space_begin = 0;
size_t *__softboundcets_stack_temporal_space_begin = NULL;

//===------------------- Data structure initialization --------------------===//

__WEAK_INLINE void __softboundcets_temporal_initialize_datastructures(void) {

    size_t temporal_table_length =
        (__SOFTBOUNDCETS_N_TEMPORAL_ENTRIES) * sizeof(void *);

    __softboundcets_lock_new_location =
        mmap(0, temporal_table_length, PROT_READ | PROT_WRITE,
             SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);

    assert(__softboundcets_lock_new_location != (void *)-1);
    __softboundcets_temporal_space_begin =
        (size_t *)__softboundcets_lock_new_location;

    size_t stack_temporal_table_length =
        (__SOFTBOUNDCETS_N_STACK_TEMPORAL_ENTRIES) * sizeof(void *);
    __softboundcets_stack_temporal_space_begin =
        mmap(0, stack_temporal_table_length, PROT_READ | PROT_WRITE,
             SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);
    assert(__softboundcets_stack_temporal_space_begin != (void *)-1);

    size_t global_lock_size =
        (__SOFTBOUNDCETS_N_GLOBAL_LOCK_SIZE) * sizeof(void *);
    __softboundcets_global_lock =
        mmap(0, global_lock_size, PROT_READ | PROT_WRITE,
             SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);
    assert(__softboundcets_get_global_lock() != (lock_type)-1);
    //  __softboundcets_global_lock =  __softboundcets_lock_new_location++;
    *((size_t *)__softboundcets_global_lock) = 1;

#if __SOFTBOUNDCETS_FREE_MAP
    size_t length_free_map =
        (__SOFTBOUNDCETS_N_FREE_MAP_ENTRIES) * sizeof(size_t);
    __softboundcets_free_map_table =
        mmap(0, length_free_map, PROT_READ | PROT_WRITE,
             SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);
    assert(__softboundcets_free_map_table != (void *)-1);
#endif
}

//===----------------------------- Checks ---------------------------------===//

__WEAK_INLINE void
__softboundcets_temporal_dereference_check(lock_type pointer_lock,
                                           key_type key) {

#if 0
  /* URGENT: I should think about removing this condition check */
  if (!pointer_lock) {
    __mi_fail_with_msg("Temporal lock null\n");
  }

#endif

    key_type temp = *((key_type *)pointer_lock);

    if (temp != key) {
        __mi_printf("[TLDC] Key mismatch key = %zx, *lock=%zx, next_ptr =%zx\n",
                    key, temp, __softboundcets_lock_next_location);
        __mi_fail();
    }
}

//===-------------------- Shadow Stack Manipulation -----------------------===//

__WEAK_INLINE key_type __softboundcets_load_key_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __KEY_INDEX;
    key_type *key_ptr = __softboundcets_shadow_stack_ptr + count;
    key_type key = *key_ptr;
    return key;
}

__WEAK_INLINE lock_type __softboundcets_load_lock_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __LOCK_INDEX;
    shadow_stack_ptr_type lock_ptr = __softboundcets_shadow_stack_ptr + count;
    lock_type lock = *((lock_type *)lock_ptr);
    return lock;
}

__WEAK_INLINE void __softboundcets_store_key_shadow_stack(key_type key,
                                                          int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __KEY_INDEX;
    key_type *key_ptr = (__softboundcets_shadow_stack_ptr + count);

    *(key_ptr) = key;
}

__WEAK_INLINE void __softboundcets_store_lock_shadow_stack(lock_type lock,
                                                           int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __LOCK_INDEX;
    lock_type *lock_ptr = (void **)(__softboundcets_shadow_stack_ptr + count);

    *(lock_ptr) = lock;
}

//===----------------------- Allocation related  --------------------------===//

__WEAK_INLINE void __softboundcets_stack_memory_deallocation(key_type ptr_key) {

#ifndef __SOFTBOUNDCETS_CONSTANT_STACK_KEY_LOCK

    __softboundcets_stack_temporal_space_begin--;
    *(__softboundcets_stack_temporal_space_begin) = 0;

#endif

    return;
}

// TODO: this function is currently unused (and might contain undefined
// behavior?)
__WEAK_INLINE void __softboundcets_memory_deallocation(lock_type ptr_lock,
                                                       key_type ptr_key) {

    __mi_debug_printf("[Hdealloc] pointer_lock = %p, *pointer_lock=%zx\n",
                      ptr_lock, *((size_t *)ptr_lock));

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

#ifdef MIRT_DEBUG
        // TODO not sure if all of this is only to be done when debugging
        __mi_debug_printf("[lock_allocate] new_lock_location=%p\n",
                          __softboundcets_lock_new_location);

        if (__softboundcets_lock_new_location >
            __softboundcets_temporal_space_begin +
                __SOFTBOUNDCETS_N_TEMPORAL_ENTRIES) {
            __mi_fail_with_msg(
                "[lock_allocate] out of temporal free entries \n");
        }
#endif

        return __softboundcets_lock_new_location++;
    } else {

        temp = __softboundcets_lock_next_location;

        __mi_debug_printf("[lock_allocate] next_lock_location=%p\n", temp);

        __softboundcets_lock_next_location =
            *((void **)__softboundcets_lock_next_location);
        return temp;
    }
}

__WEAK_INLINE void __softboundcets_stack_memory_allocation(lock_type *ptr_lock,
                                                           key_type *ptr_key) {

#ifdef __SOFTBOUNDCETS_CONSTANT_STACK_KEY_LOCK
    *ptr_key = 1;
    *((size_t **)ptr_lock) = __softboundcets_get_global_lock();
#else
    key_type temp_id = __softboundcets_key_id_counter++;
    *((size_t **)ptr_lock) =
        (size_t *)__softboundcets_stack_temporal_space_begin++;
    *ptr_key = temp_id;
    **((size_t **)ptr_lock) = temp_id;
#endif
}

__WEAK_INLINE void __softboundcets_memory_allocation(void *ptr,
                                                     lock_type *ptr_lock,
                                                     key_type *ptr_key) {

    key_type temp_id = __softboundcets_key_id_counter++;

    *((key_type **)ptr_lock) =
        (key_type *)__softboundcets_allocate_lock_location();
    *ptr_key = temp_id;
    **((key_type **)ptr_lock) = temp_id;

#if __SOFTBOUNDCETS_FREE_MAP
    __softboundcets_add_to_free_map(temp_id, ptr);
#endif
    __softboundcets_allocation_secondary_trie_allocate(ptr);

    __mi_debug_printf("[mem_alloc] lock = %p, ptr_key = %p, key = %zx\n",
                      ptr_lock, ptr_key, temp_id);
}

//===---------------------- Free map operations  --------------------------===//

__WEAK_INLINE void __softboundcets_add_to_free_map(key_type ptr_key,
                                                   void *ptr) {

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

            __mi_debug_printf("entry_ptr=%zx, ptr=%zx, key=%zx\n", entry_ptr,
                              ptr, ptr_key);
            *entry_ptr = (size_t)(ptr);
            return;
        }
        if (counter >= (__SOFTBOUNDCETS_N_FREE_MAP_ENTRIES)) {
            __mi_fail();
        }
        counter++;
    }
    return;
}

__WEAK_INLINE void __softboundcets_check_remove_from_free_map(key_type ptr_key,
                                                              void *ptr) {

#if !__SOFTBOUNDCETS_FREE_MAP
    return;
#endif

#if 0
  if (ptr_key == 1) {
    __mi_fail_with_msg("freeing a global key\n");
  }
#endif

    key_type counter = 0;
    while (1) {
        key_type index =
            (ptr_key + counter) % __SOFTBOUNDCETS_N_FREE_MAP_ENTRIES;
        key_type *entry_ptr = &__softboundcets_free_map_table[index];
        key_type tag = *entry_ptr;

        if (tag == 0) {
            __mi_fail_with_msg("free map does not have the key\n");
        }

        if (tag == (key_type)ptr) {
            *entry_ptr = 2;
            return;
        }

        if (counter >= __SOFTBOUNDCETS_N_FREE_MAP_ENTRIES) {
            __mi_fail_with_msg("free map out of entries\n");
        }
        counter++;
    }
    return;
}

#endif
