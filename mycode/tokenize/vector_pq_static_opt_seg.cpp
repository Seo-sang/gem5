#include "vector_pq_static_opt_seg.h"

void my_pq::vector_init() {
    asm volatile(
        "vsetvli %0, %1, e64, m1, tu, mu\n\t" //set vector length
        : "=r"(vl)
        : "r"(VECTOR_SIZE)
    );
}

void my_pq::print_elem() {
    printf("---------print elements---------\n");
    for(int i = 1; i <= size(); i++) {
        printf("[%d]: score: %#lx, left: %ld, right: %ld, size: %ld\n", i, arr[i].score, arr[i].left, arr[i].right, arr[i].size);
        //printf("[%d]: priority: %#lx\n", i, priority_arr[i]);
        //printf("[%d]: priority: %#lx, pointer: %p\n", i, priority_arr[i], ptr_arr[i]);
    }
    printf("--------------------------------\n");
}

my_pq::my_pq() {
    vector_init();
    //priority_arr = (int64_t*)malloc(sizeof(int64_t) * (maxSize + 1));
#ifdef LLAMA3
    arr = (struct llm_bigram_bpe*) malloc(sizeof(struct llm_bigram_bpe) * (maxSize + 1));
#else
    arr = (struct llm_bigram_spm*) malloc(sizeof(struct llm_bigram_spm) * (maxSize + 1));
#endif
    arr_size = 0;
    //idx_arr = (int64_t*)malloc(sizeof(int64_t) * (maxSize + 1));
    //priority_arr[0] = 0;
}


my_pq::my_pq(int maxSize) : maxSize(maxSize) {
    vector_init();
    //priority_arr = (int64_t*)malloc(sizeof(int64_t) * (maxSize + 1));
#ifdef LLAMA3
    arr = (struct llm_bigram_bpe*) malloc(sizeof(struct llm_bigram_bpe) * (maxSize + 1));
#else
    arr = (struct llm_bigram_spm*) malloc(sizeof(struct llm_bigram_spm) * (maxSize + 1));
#endif
    //idx_arr = (int64_t*)malloc(sizeof(int64_t) * (maxSize + 1));
    arr_size = 0;
    //priority_arr[0] = 0;
}

bool my_pq::empty() {
    return arr_size == 0LL;
}

int64_t my_pq::size() {
    return arr_size;
}

#ifdef LLAMA3
void my_pq::push(struct llm_bigram_bpe input) {
#else
void my_pq::push(struct llm_bigram_spm input) {
#endif

#ifdef DEBUG_ENABLE
    printf("----push start\n");
#endif
    // vector_init();
    if(arr_size == maxSize) return;

    arr_size++;

    //int64_t new_priority;
    int64_t new_priority = ((int64_t)input.score << 32) + input.left;
    //int64_t new_priority = ((int64_t)input.priority.field.score << 32) + (0xffffffff - input.priority.field.left);
    int32_t idx = arr_size;

    //arr[arr_size] = input;

    /*

    printf("----------\n");
    printf("push data\n");
    printf("score: %#lx, left: %ld, right: %ld, size: %ld\n", input.score, input.left, input.right, input.size);
    printf("----------\n");
    */

    /*
    if(arr_size > 1 && (arr[idx/2].score > input.score || (arr[idx/2].score == input.score) && (arr[idx/2].left < input.left)) ) { //if inserted node has highest priority
        return;
    }
        */

    int64_t mask;

    asm volatile(
        "vsetivli t0, 4, e64, m1, tu, mu \n" //set vector length
        :
        :
        : "t0"
    );

    while(idx > 0) {
        if(idx == 1) {
            arr[1] = input;
            break;
        }
        asm volatile(
           // "vsetivli t0, 4, e64, m1, tu, mu \n"
            "vlshftseg3e64.v v1, (%[arr]), %[idx] \n\t" //shift segment load
            "vmsgeu.vx v0, v1, %[new_priority] \n\t" //set mask priority
            "vmv.x.s %[mask], v0 \n\t" //check result
            : [mask]"=r"(mask)
            : [arr]"r"(arr), [idx]"r"(idx), [new_priority]"r"(new_priority)
            : "v0", "v1"
        );

        mask = mask & 0xf;
        if(mask == 0xf) { //if new node is lowest priority
            arr[idx] = input;
            break;
        }
        else if(mask == 0LL) { //if new node is highest priority
            asm volatile(
               // "vsetivli t0, 4, e64, m1, tu, mu \n"
                "vsshftseg3e64.v v1, (%[arr]), %[idx] \n" //shift segment store
                :
                : [arr]"r"(arr), [idx]"r"(idx)
                : "v1"
            );
            idx >>= vl;
        }
        else { // 0 <= pos < vl
            asm volatile(
              //  "vsetivli t0, 4, e64, m1, tu, mu \n"
                "vmv.v.x v0, %[mask] \n\t" //load mask
                "addi t0, %[mask], -1 \n\t" //set mask
                "not t0, t0 \n\t"
                "vslideup.vi v1, v1, 1, v0.t \n\t" //slide all values
                "vslideup.vi v2, v2, 1, v0.t \n\t" //slide all values
                "vslideup.vi v3, v3, 1, v0.t \n\t" //slide all values
                "vmv.v.x v0, t0 \n\t"
                "vmerge.vxm v1, v1, %[new_priority], v0 \n\t"
                "vmerge.vxm v2, v2, %[right], v0 \n\t"
                "vmerge.vxm v3, v3, %[size], v0 \n\t"
                "slli t1, %[mask], 1 \n\t"
                "not t1, t1 \n\t"
                "vmv.v.x v0, t1 \n\t" //load mask
                "vsshftseg3e64.v v1, (%[arr]), %[idx], v0.t \n\t" //shift segment store
                :
                : [mask]"r"(mask), [arr]"r"(arr), [idx]"r"(idx), [new_priority]"r"(new_priority), [right]"r"(input.right), [size]"r"(input.size)
                : "t0", "v0", "v1", "v2", "v3", "t1"
            );
            break;
        }
    }
    //print_elem();
#ifdef DEBUG_ENABLE
    printf("----push_end\n");
#endif
}

#ifdef LLAMA3
struct llm_bigram_bpe my_pq::top() {
#else
struct llm_bigram_spm my_pq::top() {
#endif
    //printf("top executed, size: %d\n", size());
    return arr[1];
}

void my_pq::pop() {
#ifdef DEBUG_ENABLE
    printf("----pop_start\n");
#endif
    if(arr_size == 0) return;
    //vector_init();
    //priority_arr[0]--;
    arr[1] = arr[arr_size];

    arr_size--;

    int cur = 1;
    while(cur * 2 <= arr_size) {
        int child = cur * 2;

        if(child < arr_size) {
            if(arr[child].score < arr[child + 1].score) child++;
        }

        if(arr[cur].score < arr[child].score) {
            struct llm_bigram_spm tmp = arr[cur];
            arr[cur] = arr[child];
            arr[child] = tmp;

            cur = child;
        }
        else {
            break;
        }


    }
#ifdef DEBUG_ENABLE
    printf("----pop_end\n");
#endif
   //print_elem();
}
/*
struct elem my_pq::get_elem(int n) {
    struct elem ret;

    ret.score = priority_arr[n] >> 32;
    ret.pos = 0xffffffff - (priority_arr[n] & 0xffff);

    return ret;
}
    */
