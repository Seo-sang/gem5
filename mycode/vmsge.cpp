#include <stdio.h>
#include <stdint.h>
#define VECTOR_SIZE 256


int  main() {
    int64_t arr[16] = {
            0LL, 0x3b9adf80fffffffeLL, 0LL, 0x3b9acf80fffffffe, 0LL, 0LL, 0LL, 0x3b9abf80fffffffe,
            0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0x78LL,
    };
    int64_t darr[16] = {0, };

    //uint64_t load_value[15] = {0, };
    int64_t value = 0x3b9aff80fffffffe;
    int stride = 8;
    int idx = 2;
    int mask = 0;


    asm volatile (
        "mv t1, %2 \n\t"
        "mv t2, %3 \n\t"
        "vsetvli t0, %1, e64, m1, tu, mu \n\t"
        "vlshfte64.v v1, (t1), t2 \n\t"
        "vse64.v v1, (%5) \n\t"
        "vmsgeu.vx v0, v1, %4 \n\t"
        "vmv.x.s %0, v0 \n\t"
            : "=r"(mask)
            : "r" (VECTOR_SIZE), "r" (arr), "r" (idx), "r" (value), "r"(darr)
            : "t0", "v1", "v2", "v3", "v4", "v0"
    );
    printf("base address: %p\n", arr);

    printf("darr[0]: %#lx darr[1]: %#lx darr[2]: %#lx darr[3]: %#lx\n", darr[0], darr[1], darr[2], darr[3]);
    printf("mask: %#lx\n", mask);
}
