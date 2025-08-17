#include "vector_pq_static_opt.h"

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
        printf("[%d]: score: %#lx, pos: %ld, idx: %d\n", i, (int32_t)arr[idx_arr[i]].score, arr[idx_arr[i]].left, idx_arr[i]);
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
    idx_size = 0;
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
    idx_size = 0;
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

    int32_t score = input.score;
    //uint32_t pos = input.left;
    //priority_arr[0]++;
    arr_size++;
    idx_size++;

    int64_t new_priority = ((int64_t)score << 32) + (0xffffffff - input.left);
    int32_t idx = arr_size;
    int64_t new_idx = idx_size;

    /*
    printf("----------\n");
    printf("pus data\n");
    printf("score: %#x\n", score);
    printf("pos: %d\n", input.left);
    printf("priority: %#lx\n", new_priority);
    printf("----------\n");
    */

    arr[new_idx] = input;


    if(arr_size > 1 && priority_arr[idx/2] > new_priority) { //if inserted node has highest priority
        priority_arr[idx] = new_priority;
        idx_arr[idx] = new_idx;
        return;
    }

    int64_t mask;
    asm volatile(
        "vsetivli t0, 4, e64, m1, tu, mu\n\t" //set vector length
        //"vmv.v.i v0, 0 \n\t" //initialize
        :
        :
        : "t0"

    );
    while(idx > 0) {
        if(idx == 1) {
            priority_arr[1] = new_priority;
            idx_arr[1] = new_idx;
            break;
        }
        asm volatile(
            "vlshfte64.v v1, (%1), %2 \n\t" //priority load
            "vmsgeu.vx v0, v1, %3 \n\t" //set mask priority
            "vmv.x.s %0, v0 \n\t" //check result
            : "=r"(mask)
            : "r"(priority_arr), "r"(idx), "r"(new_priority)
            : "v0", "v1", "t0"
        );


        mask = mask & 0xf;
        //int64_t tmp_mask;
        //printf("mask : %#lx\n", mask);
        if(mask == 0xf) { //if new node is lowest priority
            //printf("mask is 0xf \n");
            priority_arr[idx] = new_priority;
            idx_arr[idx] = new_idx;
            break;
        }
        else if(mask == 0LL) { //if new node is highest priority
            //printf("mask is 0LL \n");
            //***** priority *****
            asm volatile(
                "vlshfte64.v v2, (%2), %1 \n\t" //idx load
                "vsshfte64.v v1, (%0), %1 \n\t" //priority store
                "vsshfte64.v v2, (%2), %1 \n\t" //idx store
                :
                : "r"(priority_arr), "r"(idx), "r"(idx_arr)
                : "v1", "v2"
            );
            idx >>= vl;
        }
        else { // 0 <= pos < vl
            //printf("mask is 0 <= pos < vl \n");
            asm volatile(
                "xori t1, %0, 0xf \n\t"
                "vmv.v.x v0, t1 \n\t" //load mask
                "vlshfte64.v v4, (%5), %2, v0.t \n\t" //idx load
                "vmv.v.x v0, %0 \n\t" //load mask
                "addi t0, %0, -1 \n\t" //set mask
                "not t0, t0 \n\t"
                "vslideup.vi v1, v1, 1, v0.t \n\t" //slide all values
                "vslideup.vi v4, v4, 1, v0.t \n\t" //slide all values
                "vmv.v.x v0, t0 \n\t"
                "vmerge.vxm v1, v1, %3, v0 \n\t"
                "vmerge.vxm v4, v4, %6, v0 \n\t"
                "slli t0, %0, 1 \n\t"
                "not t0, t0 \n\t"
                "vmv.v.x v0, t0 \n\t" //load mask
                "vsshfte64.v v1, (%1), %2, v0.t \n\t" //priority store
                "vsshfte64.v v4, (%5), %2, v0.t \n\t" //idx store
                :
                : "r"(mask), "r"(priority_arr), "r"(idx), "r"(new_priority), "r"(VECTOR_SIZE), "r"(idx_arr), "r"(new_idx)
                : "t0", "v0", "v1", "v2", "v3", "v4", "v5"
            );
            /*  previous code
                //***** priority *****
                //"vsetivli t0, 4, e64, m1, tu, mu\n\t" //set vector length
                //"mv t1, %0 \n\t" //copy mask
                "xori t1, %0, 0xf \n\t"
                "vmv.v.x v0, t1 \n\t" //load mask
                //"vlshfte64.v v1, (%1), %2, v0.t \n\t" //priority load
                "vlshfte64.v v4, (%5), %2, v0.t \n\t" //idx load
                "vmv.v.v v2, v1 \n\t" //copy priority
                "vmv.v.v v5, v4 \n\t" //copy pointer
                "vmv.v.x v0, %0 \n\t" //load mask
                "addi t0, %0, -1 \n\t" //set mask
                "not t0, t0 \n\t"
                "vslideup.vi v2, v1, 1, v0.t \n\t" //slide all values
                "vslideup.vi v5, v4, 1, v0.t \n\t" //slide all values
                "vmv.v.x v0, t0 \n\t"
                "vmerge.vxm v2, v2, %3, v0 \n\t"
                "vmerge.vxm v5, v5, %6, v0 \n\t"
                "slli t0, %0, 1 \n\t"
                "not t0, t0 \n\t"
                "vmv.v.x v0, t0 \n\t" //load mask
                "vsshfte64.v v2, (%1), %2, v0.t \n\t" //priority store
                "vsshfte64.v v5, (%5), %2, v0.t \n\t" //idx store
                :
                : "r"(mask), "r"(priority_arr), "r"(idx), "r"(new_priority), "r"(VECTOR_SIZE), "r"(idx_arr), "r"(new_idx)
                : "t0", "v0", "v1", "v2", "v3", "v4", "v5"
            */
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
    return arr[idx_arr[1]];
}

void my_pq::pop() {
#ifdef DEBUG_ENABLE
    printf("----pop_start\n");
#endif
    if(arr_size == 0) return;
    //vector_init();
    //priority_arr[0]--;

    priority_arr[1] = priority_arr[arr_size];
    idx_arr[1] = idx_arr[arr_size];
    arr_size--;

    int cur = 1;
    while(cur * 2 <= arr_size) {
        int child = cur * 2;

        if(child < arr_size) {
            if(priority_arr[child] < priority_arr[child + 1]) child++;
        }

        if(priority_arr[cur] < priority_arr[child]) {
            int64_t tmp1 = priority_arr[cur];
            int64_t tmp2 = idx_arr[cur];

            priority_arr[cur] = priority_arr[child];
            idx_arr[cur] = idx_arr[child];

            priority_arr[child] = tmp1;
            idx_arr[child] = tmp2;

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

struct elem my_pq::get_elem(int n) {
    struct elem ret;

    ret.score = priority_arr[n] >> 32;
    ret.pos = 0xffffffff - (priority_arr[n] & 0xffff);

    return ret;
}
