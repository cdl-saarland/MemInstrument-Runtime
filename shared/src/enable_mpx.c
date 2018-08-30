#include "config.h"

#if ENABLE_MPX

#define _GNU_SOURCE

#include <cpuid.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>

#define MPX_L1_SIZE ((1UL << NUM_L1_BITS) * sizeof(void *))

#define NUM_L1_BITS 28
#define NUM_L2_BITS 17
#define NUM_IGN_BITS 3
#define MPX_L1_ADDR_MASK 0xfffffffffffff000ULL
#define MPX_L2_ADDR_MASK 0xfffffffffffffff8ULL
#define MPX_L2_VALID_MASK 0x0000000000000001ULL

#define REG_IP_IDX REG_RIP
#define REX_PREFIX "0x48, "

#define XSAVE_OFFSET_IN_FPMEM 0

#define MPX_ENABLE_BIT_NO 0
#define BNDPRESERVE_BIT_NO 1

struct xsave_hdr_struct {
    uint64_t xstate_bv;
    uint64_t reserved1[2];
    uint64_t reserved2[5];
} __attribute__((packed));

struct bndregs_struct {
    uint64_t bndregs[8];
} __attribute__((packed));

struct bndcsr_struct {
    uint64_t cfg_reg_u;
    uint64_t status_reg;
} __attribute__((packed));

struct xsave_struct {
    uint8_t fpu_sse[512];
    struct xsave_hdr_struct xsave_hdr;
    uint8_t ymm[256];
    uint8_t lwp[128];
    struct bndregs_struct bndregs;
    struct bndcsr_struct bndcsr;
} __attribute__((packed));

/* Following vars are initialized at process startup only
   and thus are considered to be thread safe.  */
static void *l1base = NULL;
static int bndpreserve;
static int enable = 1;

static inline void xrstor_state(struct xsave_struct *fx, uint64_t mask) {
    uint32_t lmask = mask;
    uint32_t hmask = mask >> 32;

    asm volatile(".byte " REX_PREFIX "0x0f,0xae,0x2f\n\t"
                 :
                 : "D"(fx), "m"(*fx), "a"(lmask), "d"(hmask)
                 : "memory");
}

void mpx_handler(int signum, siginfo_t *info, void *ucontext) {
    (void)signum;
    (void)info;
    ucontext_t *uctxt = ucontext;
    greg_t trapno = uctxt->uc_mcontext.gregs[REG_TRAPNO];
    greg_t ip = uctxt->uc_mcontext.gregs[REG_IP_IDX];

    if (trapno == 5) {
        printf("Bound violation exception at %#010llx\n", ip);
        exit(1);
    } else {
        printf("Unrecognized SIGSEGV trap\n");
        exit(1);
    }
}

void install_handler() {
    struct sigaction mpx_action;
    memset(&mpx_action, 0, sizeof(struct sigaction));

    mpx_action.sa_flags |= SA_SIGINFO;
    mpx_action.sa_sigaction = mpx_handler;

    sigprocmask(SIG_SETMASK, 0, &mpx_action.sa_mask);

    sigaction(SIGSEGV, &mpx_action, NULL);
}

void enable_mpx(void) {
    l1base = mmap(NULL, MPX_L1_SIZE, PROT_READ | PROT_WRITE,
                  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    uint8_t __attribute__((__aligned__(64))) buffer[4096];
    struct xsave_struct *xsave_buf = (struct xsave_struct *)buffer;

    memset(buffer, 0, sizeof(buffer));
    xrstor_state(xsave_buf, 0x18);

    xsave_buf->xsave_hdr.xstate_bv = 0x10;
    xsave_buf->bndcsr.cfg_reg_u = (unsigned long)l1base;
    xsave_buf->bndcsr.cfg_reg_u |= enable << MPX_ENABLE_BIT_NO;
    xsave_buf->bndcsr.cfg_reg_u |= bndpreserve << BNDPRESERVE_BIT_NO;
    xsave_buf->bndcsr.status_reg = 0;

    xrstor_state(xsave_buf, 0x10);

    munmap(l1base, MPX_L1_SIZE);

    install_handler();
}

#endif
