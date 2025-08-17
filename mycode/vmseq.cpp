#include <stdio.h>
#include <stdint.h>
#define VECTOR_SIZE 256


int  main() {
    //uint64_t arr[16] = {15, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    char *arr1 = "hello";
    char *arr2 = "heooo";
    int result;

    asm volatile (
        "vsetvli t0, %1, e8, m1, tu, mu \n\t"
        "mv t1, %2 \n\t"
        "mv t2, %3 \n\t"
        "vle8.v v1, (t1) \n\t"
        "vle8.v v2, (t2) \n\t"
        "vmseq.vv v3, v1, v2, v0.t \n\t"
        "vmv.x.s %0, v3 \n\t"
            : "+r"(result)
            : "r" (VECTOR_SIZE), "r" (arr1), "r" (arr2)
            : "t0", "v1", "v2", "v3", "v4", "v0"
    );

    printf("mask result: %#x\n", result);

}
