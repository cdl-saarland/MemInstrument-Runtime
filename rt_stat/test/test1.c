#include <stdint.h>

uint64_t __mi_stat_init(uint64_t num);

void __mi_stat_init_entry(uint64_t tabid, uint64_t idx, const char* text);

void __mi_stat_inc(uint64_t tabid, uint64_t idx);

void __mi_print_stats(void);

void bar(void);

int main(void)
{
    uint64_t id = __mi_stat_init(4);
    bar();

    __mi_stat_init_entry(id, 0, "foo0");
    __mi_stat_init_entry(id, 1, "foo1");
    __mi_stat_init_entry(id, 2, "foo2");
    __mi_stat_init_entry(id, 3, "foo3");
    __mi_stat_inc(id, 1);
    __mi_stat_inc(id, 1);
    __mi_stat_inc(id, 1);
    __mi_stat_inc(id, 2);
    __mi_stat_inc(id, 3);

    __mi_print_stats();
}
