#include "vector_pq.h"

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
        printf("[%d]: priority: %#lx, pointer: %p\n", i, priority_arr[i], ptr_arr[i]);
    }
    printf("--------------------------------\n\n");
}

my_pq::my_pq() {
    vector_init();
    priority_arr = (int64_t*)malloc(sizeof(int64_t) * (maxSize + 1));
    ptr_arr = (struct llm_bigram_spm**) malloc(sizeof(struct llm_bigram_spm*) * (maxSize + 1));
    priority_arr[0] = 0;
}


my_pq::my_pq(int maxSize) : maxSize(maxSize) {
    vector_init();
    priority_arr = (int64_t*)malloc(sizeof(int64_t) * (maxSize + 1));
    ptr_arr = (struct llm_bigram_spm**) malloc(sizeof(struct llm_bigram_spm*) * (maxSize + 1));
    priority_arr[0] = 0;
}

bool my_pq::empty() {
    return priority_arr[0] == 0LL;
}

int64_t my_pq::size() {
    return priority_arr[0];
}

void my_pq::push(struct llm_bigram_spm input) {
#ifdef DEBUG_ENABLE
    printf("----push start\n");
#endif
    int32_t score = input.score;
    uint32_t pos = input.left;
    vector_init();
    if(size() == maxSize) return;
    priority_arr[0]++;
    int64_t new_priority = ((int64_t)score << 32) + (0xffffffff - pos);
    //int64_t new_priority = ((int64_t)score << 32) + pos;
    //printf("new node | score: %#x, %d pos: %#x \n", score, score, pos);

    //new node generation
    struct llm_bigram_spm* new_ptr = (struct llm_bigram_spm*)malloc(sizeof(struct llm_bigram_spm));
    new_ptr->left = input.left;
    new_ptr->right = input.right;
    new_ptr->score = input.score;
    new_ptr->size = input.size;

    int32_t idx = size();
    int64_t mask;
    while(idx > 0) {
        if(idx == 1) {
            // printf("idx == 1\n");
            priority_arr[1] = new_priority;
            ptr_arr[1] = new_ptr;
            break;
        }
        asm volatile(
            "vmv.v.i v0, 1 \n\t" //initialize
            "vlshfte64.v v1, (%1), %2 \n\t" //priority load
            "vmsge.vx v0, v1, %3 \n\t" //set mask priority
            "vmv.x.s %0, v0 \n\t" //check result
            : "=r"(mask)
            : "r"(priority_arr), "r"(idx), "r"(new_priority), "r"(VECTOR_SIZE)
            : "v0", "v1", "v6"
        );
        mask = mask & 0xf;
        // printf("mask : %#lx\n", mask);
        if(mask == 0xf) { //if new node is lowest priority
            // printf("new node is the lowest priority\n");
            priority_arr[idx] = new_priority;
            ptr_arr[idx] = new_ptr;
            break;
        }
        else if(mask == 0LL) { //if new node is highest priority
            // printf("new node is the highest priority\n");
            /***** priority *****/
            asm volatile(
                //"vlshfte64.v v1, (%0), %1 \n\t" //priority load
                "vsshfte64.v v1, (%0), %1 \n\t" //priority store
                "vlshfte64.v v2, (%2), %1 \n\t" //ptr load
                "vsshfte64.v v2, (%2), %1 \n\t" //ptr store
                :
                : "r"(priority_arr), "r"(idx), "r"(ptr_arr)
                : "v1", "v2"
            );
            idx >>= vl;
        }
        else { // 0 <= pos < vl
            asm volatile(
                /***** priority *****/
                "mv t1, %0 \n\t" //copy mask
                "xori t1, t1, -1 \n\t"
                "vmv.v.x v0, t1 \n\t" //load mask
                "vlshfte64.v v1, (%1), %2, v0.t \n\t" //priority load
                "vlshfte64.v v4, (%5), %2, v0.t \n\t" //pointer load
                "vmv.v.v v2, v1 \n\t" //copy priority
                "vmv.v.v v5, v4 \n\t" //copy pointer
                "vmv.v.x v0, %0 \n\t" //load mask
                "vslideup.vi v2, v1, 1, v0.t \n\t" //slide all values
                "vslideup.vi v5, v4, 1, v0.t \n\t" //slide all values
                "addi t0, %0, -1 \n\t" //set mask
                "not t0, t0 \n\t"
                "vmv.v.x v0, t0 \n\t"
                "vmerge.vxm v2, v2, %3, v0 \n\t"
                "vmerge.vxm v5, v5, %6, v0 \n\t"
                "slli t0, %0, 1 \n\t"
                "not t0, t0 \n\t"
                "vmv.v.x v0, t0 \n\t" //load mask
                "vsshfte64.v v2, (%1), %2, v0.t \n\t" //priority store
                "vsshfte64.v v5, (%5), %2, v0.t \n\t" //pointer store
                :
                : "r"(mask), "r"(priority_arr), "r"(idx), "r"(new_priority), "r"(VECTOR_SIZE), "r"(ptr_arr), "r"(new_ptr)
                : "t0", "v0", "v1", "v2", "v3"
            );

            break;
        }
    }
    // print_elem();
#ifdef DEBUG_ENABLE
    printf("----push_end\n");
#endif
}

struct llm_bigram_spm my_pq::top() {
    //printf("top executed, size: %d\n", size());
    return *ptr_arr[1];
}

void my_pq::pop() {
#ifdef DEBUG_ENABLE
    printf("----pop_start\n");
#endif
    if(size() == 0) return;
    vector_init();
    priority_arr[0]--;
    int32_t cnt = size();
    free(ptr_arr[1]);
    //struct llm_bigram_spm* free_ptr = ptr_arr[1];

    priority_arr[1] = priority_arr[cnt+1];
    ptr_arr[1] = ptr_arr[cnt+1];

    int cur = 1;
    while(cur * 2 <= cnt) {
        int child = cur * 2;

        if(child < cnt) {
            struct elem left_elem = get_elem(child);
            struct elem right_elem = get_elem(child + 1);

            if(left_elem.score < right_elem.score || (left_elem.score == right_elem.score && left_elem.pos > right_elem.pos)) child++;
        }

        struct elem parent_elem = get_elem(cur);
        struct elem child_elem = get_elem(child);

        if(parent_elem.score < child_elem.score || (parent_elem.score == child_elem.score && parent_elem.pos > child_elem.pos)) {
            int64_t tmp1 = priority_arr[cur];
            priority_arr[cur] = priority_arr[child];
            priority_arr[child] = tmp1;

            struct llm_bigram_spm* tmp2 = ptr_arr[cur];
            ptr_arr[cur] = ptr_arr[child];
            ptr_arr[child] = tmp2;

            cur = child;
        }
        else {
            break;
        }
    }
    //free(free_ptr);
#ifdef DEBUG_ENABLE
    printf("----pop_end\n");
#endif
    // print_elem();
}

struct elem my_pq::get_elem(int n) {
    struct elem ret;

    ret.score = priority_arr[n] >> 32;
    ret.pos = 0xffffffff - (priority_arr[n] & 0xffff);
    ret.ptr = ptr_arr[n];

    return ret;
}
