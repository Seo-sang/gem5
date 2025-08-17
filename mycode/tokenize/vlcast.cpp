#include <stdio.h>
#include <stdint.h>
#define VECTOR_SIZE 256


int  main() {
    //uint64_t arr[16] = {15, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    char arr[4100] __attribute__((aligned(4096)));
    arr[4092] = 0x1;
    arr[4093] = 0x23;
    arr[4094] = 0x45;
    arr[4095] = 0x67;
    arr[4096] = 0x89;
    arr[4097] = 0xab;
    arr[4098] = 0xcd;
    arr[4099] = 0xef;
    //char arr[8] = {0x1, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    int32_t darr[8] = {0, };

    char* ptr = arr + 4092;

    asm volatile (
        "vsetvli t0, %0, e32, m1, tu, mu \n\t"
        "mv t1, %1 \n\t"
        "mv t2, %2 \n\t"
        "vmv.v.i v1, 0xf\n\t"
        "vlc8to32.v v1, (t1) \n\t" //vlc8to32.v v1, (t1) \n\t: 00001_0000_0000_0011_0110_0000_1000_0111
        "vse32.v v1, (%2) \n\t"
            :
            : "r" (VECTOR_SIZE), "r" (ptr), "r" (darr)
            : "t0", "v1"
    );
    printf("base address: %p\n", ptr);

    printf("darr[0]: %#x darr[1]: %#x darr[2]: %#x darr[3]: %#x\n", darr[0], darr[1], darr[2], darr[3]);
    printf("darr[4]: %#x darr[5]: %#x darr[6]: %#x darr[7]: %#x\n", darr[4], darr[5], darr[6], darr[7]);

}
