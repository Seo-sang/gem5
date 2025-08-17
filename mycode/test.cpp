//#include "mmio.h"
//#include <stdlib.h>
#include <iostream>
#include <cstdint>

#define VECTOR_SIZE 256


int main() {
    /*
    uint64_t a[400];
    a[0] = 0x12;
    a[2] = 0x34;
    a[4] = 0x56;
    a[6] = 0x78;
    */
    char b[32] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    uint64_t c[16];
    uint64_t result;
    uint64_t vl;
    uint64_t stride = 16;
    char d[32] = {0,};

    asm volatile(
        "vsetvli %1, %2, e8, m1, ta, ma \n\t"
        "vle8.v v1, (%4) \n\t"
        "vse8.v v1, (%6) \n\t"
        : "=r"(result), "=r"(vl)
        : "r"(VECTOR_SIZE), "r"(stride), "r"(b), "r"(c), "r"(d)
        : "t0", "v1", "v2", "a0"
    );

    printf("b address: %p\n", b);
    printf("d address: %p\n", d);

    printf("d[0]: %#x, d[1]: %#x, d[2]: %#x, d[3]: %#x\n", d[0], d[1], d[2], d[3]);
}
