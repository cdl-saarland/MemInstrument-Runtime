#include <stdint.h>

/**
 *  Allocate a new stat table and return its ID.
 *  Calls to accessor functions from one source have to use the same ID.
 */
uint64_t __mi_stat_init(uint64_t num);

void __mi_stat_init_entry(uint64_t tableID, uint64_t idx, const char *text);

void __mi_stat_inc(uint64_t tableID, uint64_t idx);

void __mi_print_stats(void);
