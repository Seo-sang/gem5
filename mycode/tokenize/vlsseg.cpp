#include <iostream>
#include <cstring>
#include <cstdint>
//#include "gem5/m5ops.h"

struct st {
    uint64_t a;
    uint64_t b;
    uint64_t c;
} arr[40];

int  main() {

    arr[0] = {0x1, 0x23, 0x45};
    arr[10] = {0x67, 0x89, 0xab};
    arr[20] = {0xcd, 0xef, 0x77};
    /*
    uint32_t arr[4100] __attribute__((aligned(4096)));
    arr[4092] = 0x1;
    arr[4093] = 0x23;
    arr[4094] = 0x45;
    arr[4095] = 0x67;
    arr[4096] = 0x89;
    arr[4097] = 0xab;
    arr[4098] = 0xcd;
    arr[4099] = 0xef;
    */
   uint64_t v1[4];
   uint64_t v2[4];
   uint64_t v3[4];

    struct st* ptr = arr;
    int stride = 240;

    asm volatile (
        "vsetivli t0, 4, e64, m1, tu, mu \n\t"
        "vlsseg3e64.v v1, (%[ptr]), %[stride] \n"
        "vse64.v v1, (%[v1]) \n\t"
        "vse64.v v2, (%[v2]) \n\t"
        "vse64.v v3, (%[v3]) \n\t"
            :
            : [ptr]"r"(ptr), [stride]"r"(stride), [v1]"r"(v1), [v2]"r"(v2), [v3]"r"(v3)
            : "t0", "v1"
    );
    printf("base address: %p\n", ptr);

    printf("v1[0]: %#x v1[1]: %#x v1[2]: %#x v1[3]: %#x\n", v1[0], v1[1], v1[2], v1[3]);
    printf("v2[0]: %#x v2[1]: %#x v2[2]: %#x v2[3]: %#x\n", v2[0], v2[1], v2[2], v2[3]);
    printf("v3[0]: %#x v3[1]: %#x v3[2]: %#x v3[3]: %#x\n", v3[0], v3[1], v3[2], v3[3]);

}
