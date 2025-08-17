#include <stdio.h>
#include <stdint.h>
#define VECTOR_SIZE 256


int  main() {
    //uint64_t arr[16] = {15, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    int64_t arr[16] = {
            0x12LL, 0x34LL, 0x56LL, 0x78LL,
    };
    int64_t darr[16] = {0, };

    char carr[6] = {'a', 'b', 'c', 'd',};
    //uint64_t load_value[15] = {0, };
    int64_t value = (90LL << 32) + 11;
    int stride = 8;
    int idx = 15;


    asm volatile (
        "mv t1, %1 \n\t"
        "mv t2, %2 \n\t"
        "vsetvli t0, %0, e64, m1, tu, mu \n\t"
        "vle64.v v1, (t1) \n\t"
        // "vse64.v v1, (%6) \n\t"
        "vmv.v.i v0, 0xd \n\t"
        "vsshfte64.v v1, (%6), t2, v0.t \n\t"
            :
            : "r" (VECTOR_SIZE), "r" (arr), "r" (idx), "r" (value) , "r"(carr), "r"(stride), "r"(darr)
            : "t0", "v1", "v2", "v3", "v4", "v0"
    );
    printf("base address: %p\n", arr);

    //printf("darr[15]: %#lx darr[7]: %#lx darr[3]: %#lx darr[1]: %#lx\n", darr[3], darr[2], darr[1], darr[0]);
    printf("darr[15]: %#lx darr[7]: %#lx darr[3]: %#lx darr[1]: %#lx\n", darr[15], darr[7], darr[3], darr[1]);

}
