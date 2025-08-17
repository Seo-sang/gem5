#include <iostream>
#include <cstring>
#include <cstdint>
#include "gem5/m5ops.h"

#define VECTOR_SIZE 256


struct llm_bigram_spm {
    int32_t score : 32;// = 0x11;
    int32_t left: 32;// = 0x22;

	// union {
	// 	struct {
	// 		int32_t score : 32;
	// 		int32_t left : 32;
	// 	} field;
	// 	int64_t total;
	// } priority;

    int64_t right : 64;// = 0x9999999999999999;
    int64_t size : 64;// = 0x8888888888888888;
} arr[20];


int main() {

    struct llm_bigram_spm* ptr = arr;
    int idx = 15;
    int64_t a[10];

    m5_reset_stats(0, 0);

    char arr8[10];

    asm volatile(
        "vsetivli t0,8, e32, tu, mu \n"
        "vlc8to32.v v1, (%[arr8]) \n"
        //"vsshftseg3e64.v v1, (%[ptr]), %[idx], v0.t \n"
        //"vlshftseg3e64.v v1, (%[ptr]), %[idx] \n"
        //"vle64.v v1, (%[a]) \n"
        //"ld t0, (%[a]) \n"
        //"ld t0, 8(%[a]) \n"
        //"ld t0, 16(%[a]) \n"
        //"ld t0, 24(%[a]) \n"
        //"vlseg3e64.v v1, (t1) \n"
        //"vse64.v v0, (%[v0]) \n"
        //"vse64.v v1, (%[v1]) \n"
        //"vse64.v v2, (%[v2]) \n"
        //"vse64.v v3, (%[v3]) \n"
        //"vse64.v v4, (%[v4]) \n"
        :
        : [ptr]"r"(ptr), [idx]"r"(idx), [a]"r"(a), [arr8]"r"(arr8)
        : "a0", "a1"
    );

    m5_dump_stats(0, 0);
}
