#include <stdint.h>

uint64_t __mi_stat_init(uint64_t num);

void __mi_stat_init_entry(uint64_t tabid, uint64_t idx, const char* text);

void __mi_stat_inc(uint64_t tabid, uint64_t idx);

void __mi_print_stats(void);

void bar(void)
{
    uint64_t id = __mi_stat_init(2);
    __mi_stat_init_entry(id, 0, "bar0");
    __mi_stat_init_entry(id, 1, "bar1");
    __mi_stat_inc(id, 0);
    __mi_stat_inc(id, 0);
    __mi_stat_inc(id, 1);
}

