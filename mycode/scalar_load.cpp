#include <cstdint>
#define VECTOR_SIZE 256

int main() {
    int64_t arr[50] = {0, };
    int idx = 48;

    asm volatile(
        "vsetvli t0, %0, e32, m1, tu, mu \n\t"
        "mv t0, %2 \n\t"
        "ld t0, (%1) \n\t"
        :
        : "r"(VECTOR_SIZE), "r"(arr), "r"(idx)
        : "t0"
    );

    return 0;
}
