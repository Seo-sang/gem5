#include <stdio.h>
#include <stdint.h>
#define VECTOR_SIZE 256


int  main() {
    //uint64_t arr[16] = {15, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    int64_t arr[16] = {
            0LL, 0x12LL, 0LL, 0x34LL, 0LL, 0LL, 0LL, 0x56LL,
            0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0x78LL,
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
        "vmv.v.i v0, 0xd \n\t"
        "vlshfte64.v v1, (t1), t2, v0.t \n\t"
        //"vlse64.v v1, (t1), %5 \n\t"
        //".word 0x1a737087 \n\t" //vlshfte64.v v1, (t1), t2\n\t" 0001_1010_0111_0011_0111_0000_1000_0111
        "addi t3, zero, 0x99 \n\t"
        "vmv.v.i v0, 0x2 \n\t"
        "vmerge.vxm v1, v1, t3, v0 \n\t"
        "vse64.v v1, (%6) \n\t"
            :
            : "r" (VECTOR_SIZE), "r" (arr), "r" (idx), "r" (value) , "r"(carr), "r"(stride), "r"(darr)
            : "t0", "v1", "v2", "v3", "v4", "v0"
    );
    printf("base address: %p\n", arr);

    printf("darr[0]: %#lx darr[1]: %#lx darr[2]: %#lx darr[3]: %#lx\n", darr[0], darr[1], darr[2], darr[3]);

}
