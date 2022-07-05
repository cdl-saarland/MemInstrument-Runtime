#ifndef SOFTBOUNDCETS_COMMON_H
#define SOFTBOUNDCETS_COMMON_H

#include "softboundcets_defines.h"

// 2^23 entries each will be 8 bytes each
static const size_t __SOFTBOUNDCETS_TRIE_PRIMARY_TABLE_ENTRIES =
    ((size_t)8 * (size_t)1024 * (size_t)1024);

static const size_t __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES =
    ((size_t)128 * (size_t)32);

// each secondary entry has 2^ 22 entries
static const size_t __SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES =
    ((size_t)4 * (size_t)1024 * (size_t)1024);

/* Layout of the shadow stack

  1) size of the previous stack frame
  2) size of the current stack frame
  3) base/bound/key/lock of each argument

  Allocation: read the current stack frames size, increment the
  shadow_stack_ptr by current_size + 2, store the previous size into
  the new prev value, calculate the allocation size and store in the
  new current stack size field; Deallocation: read the previous size,
  and decrement the shadow_stack_ptr */

void __softboundcets_allocate_shadow_stack_space(int num_pointer_args);

void __softboundcets_deallocate_shadow_stack_space();

__softboundcets_trie_entry_t *__softboundcets_trie_allocate();

/* Metadata store parameterized by the mode of checking */

#if __SOFTBOUNDCETS_SPATIAL

void __softboundcets_metadata_store(const void *addr_of_ptr, void *base,
                                    void *bound);

#elif __SOFTBOUNDCETS_TEMPORAL

void __softboundcets_metadata_store(const void *addr_of_ptr, key_type key,
                                    lock_type lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

void __softboundcets_metadata_store(const void *addr_of_ptr, void *base,
                                    void *bound, key_type key, lock_type lock);

#endif

#if __SOFTBOUNDCETS_SPATIAL

void __softboundcets_metadata_load(const void *addr_of_ptr, void **base,
                                   void **bound);

#elif __SOFTBOUNDCETS_TEMPORAL

void __softboundcets_metadata_load(const void *addr_of_ptr, key_type *key,
                                   lock_type *lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

void __softboundcets_metadata_load(const void *addr_of_ptr, void **base,
                                   void **bound, key_type *key,
                                   lock_type *lock);

#endif

void __softboundcets_copy_metadata(void *dest, const void *from, size_t size);

/******************************************************************************/

void __softboundcets_allocation_secondary_trie_allocate_range(void *initial_ptr,
                                                              size_t size);
void __softboundcets_allocation_secondary_trie_allocate(void *addr_of_ptr);

//===----------------------------------------------------------------------===//
//                    VarArg Proxy Object Handling
//===----------------------------------------------------------------------===//

// Vaarg handlers in C are initialized by calling va_start and can afterwards
// be used by calling va_arg. Every va_arg call reads "the next" argument handed
// over to the function. In order to provide the metadata to check for the
// validity of vaargs, we generate a proxy object that keeps track of the
// metadata that belongs to the next pointer argument. We create the proxy upon
// the call to va_start. Upon a va_arg call that loads a _pointer_, the proxy
// object can be asked for the metadata for this pointer. The internal state of
// the proxy object will update itself upon this request to point to the data
// for the next pointer argument. Finally, upon va_end, the proxy object is
// deleted.

// Implementation details:
// The proxy object encapsulates a pointer to the shadow stack. The proxy state
// is updated whenever pointer bounds are requested through the proxy. It then
// jumps to the next shadow stack entry. Note that after reading the last
// pointer value from the shadow stack, the proxy object points one past the
// last valid shadow stack location. The proxy object itself never modifies the
// shadow stack (similar to va_arg which only reads).
// If a call hands over a va_list argument, the proxy object is passed to the
// callee through the shadow stack, just as other metadata. However, the shadow
// stack entry of a proxy argument is smaller than regular entries which have
// size __SOFTBOUNDCETS_METADATA_NUM_FIELDS. Hence, we provide special functions
// to load proxy objects from/ store them onto the shadow stack.

// Note that this is not designed to detect the misuse of vaarg functions.
// We cannot identify if the caller hands over different types or fewer
// arguments than what the callee expects. However, if pointers are involved in
// this mixup, it is likely that an (not necessarily on point) error will occur
// at some later point during the execution.

// The vararg proxy requires less space on the shadow stack than regular
// entries, these values are written to the unused entries in debug mode to make
// it easy to spot them in the debug output (e.g., if they are read, the shadow
// stack is broken or used the wrong way).
// Use a similar indicator for in-memory information.
#define INVALID_STACK_VALUE 0x42
#define INVALID_MEM_VALUE 0x24

// Create a vaarg proxy object.
// The arg_no needs to be the index of the last pointer in the called functions
// signature plus one (0 if no pointer is used).
shadow_stack_ptr_type *__softboundcets_allocate_va_arg_proxy(int arg_no);

// Get the metadata for the next va arg.
// This updates the state of the va_arg_proxy to point to the next vaarg.
#if __SOFTBOUNDCETS_SPATIAL
__METADATA_INLINE void
__softboundcets_next_va_arg_metadata(shadow_stack_ptr_type *va_arg_proxy,
                                     void **base, void **bound);
#elif __SOFTBOUNDCETS_TEMPORAL
__METADATA_INLINE void
__softboundcets_next_va_arg_metadata(shadow_stack_ptr_type *va_arg_proxy,
                                     key_type *key, lock_type *lock);
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void
__softboundcets_next_va_arg_metadata(shadow_stack_ptr_type *va_arg_proxy,
                                     void **base, void **bound, key_type *key,
                                     lock_type *lock);
#endif

// (Deep-)Copy the given proxy object, such that the incoming proxy and the new
// one both refer to the same vaarg. They can be used independently afterwards
// and do not influence each others state. Intended to be used upon va_copy
// calls.
shadow_stack_ptr_type *
__softboundcets_copy_va_arg_proxy(shadow_stack_ptr_type *to_copy);

// Delete the given va proxy.
void __softboundcets_free_va_arg_proxy(shadow_stack_ptr_type *va_arg_proxy);

// Vaarg proxies do not have the same layout as the regular metadata, such that
// special functions are required to store them to/load them from the shadow
// stack or memory.

// Load a vaarg proxy from the shadow stack
__WEAK_INLINE shadow_stack_ptr_type *
__softboundcets_load_proxy_shadow_stack(int arg_no);

// Store a vaarg proxy to the shadow stack
__WEAK_INLINE void
__softboundcets_store_proxy_shadow_stack(shadow_stack_ptr_type *proxy,
                                         int arg_no);

// Store metadata for the address the proxy object is stored to
void __softboundcets_proxy_metadata_store(const void *addr_of_ptr,
                                          shadow_stack_ptr_type *proxy);

// Load metadata for the address the proxy object is loaded from
void __softboundcets_proxy_metadata_load(const void *addr_of_ptr,
                                         shadow_stack_ptr_type **proxy);

/******************************************************************************/

// Store metadata about the `environ` variable
void __softboundcets_update_environment_metadata();

#ifdef MIRT_STATISTICS
// If statistics should be selected, provide a function to incremenet the
// external checks counter
void __softboundcets_inc_external_check(void);
#endif

#endif // SOFTBOUNDCETS_COMMON_H
