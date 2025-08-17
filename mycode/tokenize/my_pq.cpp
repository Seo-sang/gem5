#include "my_pq.h"

void my_pq::print_elem() {
    printf("---------print elements---------\n");
    for(int i = 1; i <= size(); i++) {
    }
    printf("--------------------------------\n\n");
}

my_pq::my_pq(int maxSize) : maxSize(maxSize) {
    arr_size = 0;
#ifdef LLAMA3
    arr = (struct llm_bigram_bpe*) malloc(sizeof(struct llm_bigram_bpe) * (maxSize + 1));
#else
    arr = (struct llm_bigram_spm*) malloc(sizeof(struct llm_bigram_spm) * (maxSize + 1));
#endif
}

my_pq::my_pq() {
    arr_size = 0;
#ifdef LLAMA3
    arr = (struct llm_bigram_bpe*) malloc(sizeof(struct llm_bigram_bpe) * (maxSize + 1));
#else
    arr = (struct llm_bigram_spm*) malloc(sizeof(struct llm_bigram_spm) * (maxSize + 1));
#endif
}

bool my_pq::empty() {
    return arr_size == 0;
}

int64_t my_pq::size() {
    return arr_size;
}


#ifdef LLAMA3
void my_pq::push(struct llm_bigram_bpe input) {
#else
void my_pq::push(struct llm_bigram_spm input) {
#endif
    // printf("my push\n");

    if(size() == maxSize) return;
    arr_size++;

    //new node generation
    arr[arr_size] = input;

    int32_t idx = arr_size;

    while(idx > 1) {
#ifdef LLAMA3
        struct llm_bigram_bpe curr = arr[idx];
        struct llm_bigram_bpe parent = arr[idx / 2];
#else
        struct llm_bigram_spm curr = arr[idx];
        struct llm_bigram_spm parent = arr[idx / 2];
#endif

        if((curr.score > parent.score) || (curr.score == parent.score && curr.left < parent.left)) { //new node has higher priority
#ifdef LLAMA3
        struct llm_bigram_bpe tmp = arr[idx / 2];
#else
        struct llm_bigram_spm tmp = arr[idx / 2];
#endif
            arr[idx / 2] = arr[idx];
            arr[idx] = tmp;
        }
        else {
            break;
        }
        idx /= 2;
    }
    // print_elem();
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
    // printf("my pop\n");

    if(arr_size == 0) return;

    arr[1] = arr[arr_size];
    arr_size--;

    int cur = 1;
    while(cur * 2 <= arr_size) {
        int child = cur * 2;

        if(child < arr_size) {
#ifdef LLAMA3
            struct llm_bigram_bpe left_node = arr[child];
            struct llm_bigram_bpe right_node = arr[child + 1];
#else
            struct llm_bigram_spm left_node = arr[child];
            struct llm_bigram_spm right_node = arr[child + 1];
#endif

            if(left_node.score < right_node.score || (left_node.score == right_node.score && left_node.left > right_node.left)) child++;
        }
#ifdef LLAMA3
            struct llm_bigram_bpe parent_node = arr[cur];
            struct llm_bigram_bpe child_node = arr[child];
#else
            struct llm_bigram_spm parent_node = arr[cur];
            struct llm_bigram_spm child_node = arr[child];
#endif

        if(parent_node.score < child_node.score || (parent_node.score == child_node.score && parent_node.left > child_node.left)) {
#ifdef LLAMA3
            struct llm_bigram_bpe tmp = arr[cur];
#else
            struct llm_bigram_spm tmp = arr[cur];
#endif
            arr[cur] = arr[child];
            arr[child] = tmp;

            cur = child;
        }
        else {
            break;
        }
    }

    // print_elem();
}
