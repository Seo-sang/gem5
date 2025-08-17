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
	int32_t left: 32;
	int32_t score: 32;
	/*
	union
	{
		struct {
			int32_t score : 32;
			llm_symbol::index left : 32;
		} field;
		int64_t total;
	} priority;
	 */
    llm_symbol::index right : 64;
    size_t size : 64;
};
#endif

class my_pq {
private:
#ifdef LLAMA3
	struct llm_bigram_bpe* arr;
#else
	struct llm_bigram_spm* arr;
#endif
	int maxSize = 20000;
	int64_t arr_size = 0;
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
	//struct elem get_elem(int n);
};
