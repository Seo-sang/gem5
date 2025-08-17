#include <stdio.h>
#include <stdint.h>
#define VECTOR_SIZE 256


int  main() {
    char *str1 = "abcdefghijkaabcdefghijkaabcdefghijkaabcdefghijkaabcdefghijka";
    char *str2 = "abcdefghijkaabcdefghijkaabcdefghijkaabcdefghijkaabcdefghijka";
    int32_t result;

    asm volatile (
        "vsetvli t0, %1, e8, m1, tu, mu \n\t"
        "vle8.v v1, (%2) \n\t"
        "vle8.v v2, (%3) \n\t"
        ".word 0xfa208057 \n\t"//"vstrcmp.vv v0, v1, v2 \n\t"
        "vmv.x.s %0, v0\n\t"

            : "=r"(result)
            : "r" (VECTOR_SIZE), "r" (str1), "r" (str2)
            : "t0", "v0", "v1", "v2"
    );

    printf("result: %d\n", result);

}
