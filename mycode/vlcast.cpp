#include <stdio.h>
#include <stdint.h>
#define VECTOR_SIZE 256


int  main() {
    //uint64_t arr[16] = {15, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    char arr[8] = {0x1, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    int32_t darr[8] = {0, };
    int stride = 1;


    asm volatile (
        "vsetvli t0, %0, e32, m1, tu, mu \n\t"
        //"vlse32.v v1, (%1), %2\n\t"
        //".word 0x3ac76087 \n\t"
        "vlc8to32.v v1, (%1), %2 \n\t"
        "vse32.v v1, (%3) \n\t"
            :
            : "r" (VECTOR_SIZE), "r" (arr), "r" (stride), "r" (darr)
            : "t0", "v1"
    );
    printf("base address: %p\n", arr);

    printf("darr[0]: %#x darr[1]: %#x darr[2]: %#x darr[3]: %#x\n", darr[0], darr[1], darr[2], darr[3]);
    printf("darr[4]: %#x darr[5]: %#x darr[6]: %#x darr[7]: %#x\n", darr[4], darr[5], darr[6], darr[7]);

}
