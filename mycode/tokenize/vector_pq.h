#include <cstdint>
#include <cstdio>
#include <cstdlib>

#define VECTOR_SIZE 256

struct elem {
	int32_t score;
	uint32_t pos;
	void* ptr;
};


struct llm_symbol {
    using index = int;
    index prev;
    index next;
    const char * text;
    size_t n;
};

struct llm_bigram_spm {
    llm_symbol::index left;
    llm_symbol::index right;
    float score;
    size_t size;
};

class my_pq {
private:
    //variables
	int64_t* priority_arr;
	struct llm_bigram_spm** ptr_arr;
	int maxSize = 20000;
	int64_t vl;

    //functions
	void vector_init();
	void print_elem();

public:

    //functions
	my_pq();
	my_pq(int maxSize);
    bool empty();
	int64_t size();
	void push(struct llm_bigram_spm input);
	struct llm_bigram_spm top();
	void pop();
	struct elem get_elem(int n);
};
