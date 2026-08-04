#include <stdint.h>
// Manual EWRAM blocks for KH CoM EUR — memcpy auto-copied to 0x02038D48.
// Written in the exact ABI/style emitted by recomp (RtApi + uint32_t state).
// Two blocks: 0x02038D48 (setup + beq epilogue + dispatch to 0x6A) and
// 0x02038D6A (byte-copy loop inline + fall-through epilogue + bx r0 return).

typedef struct {
    uint32_t (*interp_one)(void*);
    uint32_t (*read8)(void*, uint32_t);
    uint32_t (*read16)(void*, uint32_t);
    uint32_t (*read32)(void*, uint32_t);
    void (*write8)(void*, uint32_t, uint32_t);
    void (*write16)(void*, uint32_t, uint32_t);
    void (*write32)(void*, uint32_t, uint32_t);
    void (*tick)(void*, uint32_t);
    uint32_t (*guard)(void*, uint32_t, uint32_t, uint64_t);
    uint32_t (*chain_gate)(void*);
} RtApi;

static inline void f_nz(uint32_t* c, uint32_t r) {
    c[16] = (c[16] & 0x3FFFFFFFu) | (r & 0x80000000u) | ((r == 0u) ? 0x40000000u : 0u);
}

static inline uint32_t op_adds(uint32_t* c, uint32_t a, uint32_t b, uint32_t cin) {
    uint64_t w = (uint64_t)a + (uint64_t)b + (uint64_t)cin;
    uint32_t r = (uint32_t)w;
    c[16] = (c[16] & 0x0FFFFFFFu)
        | (r & 0x80000000u) | ((r == 0u) ? 0x40000000u : 0u)
        | ((w >> 32) ? 0x20000000u : 0u)
        | (((((~(a ^ b)) & (a ^ r)) >> 31) & 1u) ? 0x10000000u : 0u);
    return r;
}

uint32_t b_02038d48_t(const RtApi* a, void* m) {
    uint32_t* c = (uint32_t*)m;
    uint32_t pc; (void)pc;
    // 02038d48 stmdb sp!, {r4, r5, lr}
    {
        uint32_t base = c[13];
        uint32_t addr = base - 12u;
        a->write32(m, addr & ~3u, c[4]); addr += 4u;
        a->write32(m, addr & ~3u, c[5]); addr += 4u;
        a->write32(m, addr & ~3u, c[14]); addr += 4u;
        c[13] = base - 12u;
    }
    a->tick(m, 7u);
    // 02038d4a adds r5, r0, #0
    { uint32_t r = op_adds(c, c[0], 0x0u, 0u); c[5] = r; }
    a->tick(m, 1u);
    // 02038d4c adds r4, r1, #0
    { uint32_t r = op_adds(c, c[1], 0x0u, 0u); c[4] = r; }
    a->tick(m, 1u);
    // 02038d4e adds r3, r2, #0
    { uint32_t r = op_adds(c, c[2], 0x0u, 0u); c[3] = r; }
    a->tick(m, 1u);
    // 02038d50 ldr r2, [pc, #0x2c] ; =[0x02038d80]
    {
        uint32_t base = 0x02038d80u;
        uint32_t ob = base + 0x0u;
        uint32_t addr = ob;
        c[2] = a->read32(m, addr & ~3u);
    }
    a->tick(m, 4u);
    // 02038d52 ldrh r0, [r2]
    c[0] = a->read16(m, c[2] & ~1u);
    a->tick(m, 4u);
    // 02038d54 ldr r1, [pc, #0x2c] ; =[0x02038d84]
    {
        uint32_t base = 0x02038d84u;
        uint32_t ob = base + 0x0u;
        uint32_t addr = ob;
        c[1] = a->read32(m, addr & ~3u);
    }
    a->tick(m, 4u);
    // 02038d56 ands r0, r0, r1
    { uint32_t r = c[0] & c[1]; c[0] = r; f_nz(c, r); }
    a->tick(m, 1u);
    // 02038d58 movs r1, #3
    { uint32_t r = 0x3u; c[1] = r; f_nz(c, r); }
    a->tick(m, 1u);
    // 02038d5a orrs r0, r0, r1
    { uint32_t r = c[0] | c[1]; c[0] = r; f_nz(c, r); }
    a->tick(m, 1u);
    // 02038d5c strh r0, [r2]
    a->write16(m, c[2], c[0]);
    a->tick(m, 4u);
    // 02038d5e subs r3, r3, #1
    { uint32_t r = op_adds(c, c[3], ~0x1u, 1u); c[3] = r; }
    a->tick(m, 1u);
    // 02038d60 movs r0, #1
    { uint32_t r = 0x1u; c[0] = r; f_nz(c, r); }
    a->tick(m, 1u);
    // 02038d62 rsbs r0, r0, #0  -> r0 = 0 - r0
    { uint32_t r = op_adds(c, 0x0u, ~c[0], 1u); c[0] = r; }
    a->tick(m, 1u);
    // 02038d64 cmp r3, r0
    { uint32_t r = op_adds(c, c[3], ~c[0], 1u); (void)r; }
    a->tick(m, 1u);
    // 02038d66 beq 0x02038d78
    { uint32_t s_ = c[16]; uint32_t cn = s_ >> 31, cz = (s_ >> 30) & 1u, cc = (s_ >> 29) & 1u, cv = (s_ >> 28) & 1u;
      if (cz) goto L_02038d78_epi; }
    // 02038d68 adds r1, r0, #0
    { uint32_t r = op_adds(c, c[0], 0x0u, 0u); c[1] = r; }
    a->tick(m, 1u);
    // dispatch to the loop block 0x02038D6A
    c[15] = 0x02038d6au;
    return 0x02038d6au;
L_02038d78_epi:;
    // 02038d78 ldmia sp!, {r4, r5}
    {
        uint32_t base = c[13];
        uint32_t addr = base + 0u;
        c[4] = a->read32(m, addr & ~3u); addr += 4u;
        c[5] = a->read32(m, addr & ~3u); addr += 4u;
        c[13] = base + 8u;
    }
    a->tick(m, 5u);
    // 02038d7a ldmia sp!, {r0}
    c[0] = a->read32(m, c[13] & ~3u);
    c[13] += 4u;
    a->tick(m, 3u);
    // 02038d7c bx r0
    { uint32_t r = c[0] & ~1u; c[15] = r; return r; }
}

uint32_t b_02038d6a_t(const RtApi* a, void* m) {
    uint32_t* c = (uint32_t*)m;
    uint32_t pc; (void)pc;
L_02038d6a_t:;
    // 02038d6a ldrb r0, [r5]
    c[0] = a->read8(m, c[5]);
    a->tick(m, 4u);
    // 02038d6c strb r0, [r4]
    a->write8(m, c[4], c[0]);
    a->tick(m, 4u);
    // 02038d6e adds r5, r5, #1
    { uint32_t r = op_adds(c, c[5], 0x1u, 0u); c[5] = r; }
    a->tick(m, 1u);
    // 02038d70 adds r4, r4, #1
    { uint32_t r = op_adds(c, c[4], 0x1u, 0u); c[4] = r; }
    a->tick(m, 1u);
    // 02038d72 subs r3, r3, #1
    { uint32_t r = op_adds(c, c[3], ~0x1u, 1u); c[3] = r; }
    a->tick(m, 1u);
    // 02038d74 cmp r3, r1
    { uint32_t r = op_adds(c, c[3], ~c[1], 1u); (void)r; }
    a->tick(m, 1u);
    // 02038d76 bne 0x02038d6a  (loop inline)
    { uint32_t s_ = c[16]; uint32_t cn = s_ >> 31, cz = (s_ >> 30) & 1u, cc = (s_ >> 29) & 1u, cv = (s_ >> 28) & 1u;
      if (!cz) goto L_02038d6a_t; }
    // 02038d78 ldmia sp!, {r4, r5}
    {
        uint32_t base = c[13];
        uint32_t addr = base + 0u;
        c[4] = a->read32(m, addr & ~3u); addr += 4u;
        c[5] = a->read32(m, addr & ~3u); addr += 4u;
        c[13] = base + 8u;
    }
    a->tick(m, 5u);
    // 02038d7a ldmia sp!, {r0}
    c[0] = a->read32(m, c[13] & ~3u);
    c[13] += 4u;
    a->tick(m, 3u);
    // 02038d7c bx r0
    { uint32_t r = c[0] & ~1u; c[15] = r; return r; }
}
