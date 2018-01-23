void enable_mpx(void);

__attribute__((constructor))
static void initmpx(void) {
    enable_mpx();
}
