#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

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

#ifdef LLAMA3
struct llm_bigram_bpe {
    llm_symbol::index left;
    llm_symbol::index right;
	std::string text;
    int score;
    size_t size;
};
#else
struct llm_bigram_spm {
    llm_symbol::index left;
    llm_symbol::index right;
    float score;
    size_t size;
};
#endif

class my_pq {
private:
    //variables
	int64_t priority_arr[20000];
#ifdef LLAMA3
	struct llm_bigram_bpe* arr;
#else
	struct llm_bigram_spm* arr;
#endif
	int64_t idx_arr[20000];
	//struct llm_bigram_spm** ptr_arr;
	int maxSize = 20000;
	int64_t vl;
	int64_t arr_size;
	int64_t idx_size;

    //functions
	void vector_init();
	void print_elem();

public:
    //functions
	my_pq();
	my_pq(int maxSize);
    bool empty();
	int64_t size();
#ifdef LLAMA3
	void push(struct llm_bigram_bpe input);
#else
	void push(struct llm_bigram_spm input);
#endif
#ifdef LLAMA3
	struct llm_bigram_bpe top();
#else
	struct llm_bigram_spm top();
#endif
	void pop();
	struct elem get_elem(int n);
};
