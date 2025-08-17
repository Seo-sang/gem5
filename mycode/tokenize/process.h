#include <queue>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <map>
#include <unordered_map>
#include <time.h>
#include <forward_list>
#include "gem5/m5ops.h"


#ifdef VECTOR_ENABLE
    #include "vector_mymap.h"
    #include "vector_pq_static_opt.h"
#else
    #include "mymap.h"
    #include "my_pq.h"
#endif


enum llama_vocab_type {
    LLAMA_VOCAB_TYPE_NONE = 0, // For models without vocab
    LLAMA_VOCAB_TYPE_SPM  = 1, // LLaMA tokenizer based on byte-level BPE with byte fallback
    LLAMA_VOCAB_TYPE_BPE  = 2, // GPT-2 tokenizer based on byte-level BPE
    LLAMA_VOCAB_TYPE_WPM  = 3, // BERT tokenizer based on WordPiece
    LLAMA_VOCAB_TYPE_UGM  = 4, // T5 tokenizer based on Unigram
};

// pre-tokenization types
enum llama_vocab_pre_type {
    LLAMA_VOCAB_PRE_TYPE_DEFAULT        = 0,
    LLAMA_VOCAB_PRE_TYPE_LLAMA3         = 1,
    LLAMA_VOCAB_PRE_TYPE_DEEPSEEK_LLM   = 2,
    LLAMA_VOCAB_PRE_TYPE_DEEPSEEK_CODER = 3,
    LLAMA_VOCAB_PRE_TYPE_FALCON         = 4,
    LLAMA_VOCAB_PRE_TYPE_MPT            = 5,
    LLAMA_VOCAB_PRE_TYPE_STARCODER      = 6,
    LLAMA_VOCAB_PRE_TYPE_GPT2           = 7,
    LLAMA_VOCAB_PRE_TYPE_REFACT         = 8,
    LLAMA_VOCAB_PRE_TYPE_COMMAND_R      = 9,
    LLAMA_VOCAB_PRE_TYPE_STABLELM2      = 10,
    LLAMA_VOCAB_PRE_TYPE_QWEN2          = 11,
    LLAMA_VOCAB_PRE_TYPE_OLMO           = 12,
    LLAMA_VOCAB_PRE_TYPE_DBRX           = 13,
    LLAMA_VOCAB_PRE_TYPE_SMAUG          = 14,
    LLAMA_VOCAB_PRE_TYPE_PORO           = 15,
    LLAMA_VOCAB_PRE_TYPE_CHATGLM3       = 16,
    LLAMA_VOCAB_PRE_TYPE_CHATGLM4       = 17,
    LLAMA_VOCAB_PRE_TYPE_VIKING         = 18,
    LLAMA_VOCAB_PRE_TYPE_JAIS           = 19,
};


enum llama_token_attr {
    LLAMA_TOKEN_ATTR_UNDEFINED    = 0,
    LLAMA_TOKEN_ATTR_UNKNOWN      = 1 << 0,
    LLAMA_TOKEN_ATTR_UNUSED       = 1 << 1,
    LLAMA_TOKEN_ATTR_NORMAL       = 1 << 2,
    LLAMA_TOKEN_ATTR_CONTROL      = 1 << 3,  // SPECIAL?
    LLAMA_TOKEN_ATTR_USER_DEFINED = 1 << 4,
    LLAMA_TOKEN_ATTR_BYTE         = 1 << 5,
    LLAMA_TOKEN_ATTR_NORMALIZED   = 1 << 6,
    LLAMA_TOKEN_ATTR_LSTRIP       = 1 << 7,
    LLAMA_TOKEN_ATTR_RSTRIP       = 1 << 8,
    LLAMA_TOKEN_ATTR_SINGLE_WORD  = 1 << 9,
};



struct llama_vocab {
    using id    = int32_t;
    using token = std::string;
    using tattr = llama_token_attr;

    struct token_data {
        token text;
        float score;
        tattr attr;
    };

    enum llama_vocab_type     type     = LLAMA_VOCAB_TYPE_SPM;
    enum llama_vocab_pre_type type_pre = LLAMA_VOCAB_PRE_TYPE_DEFAULT;

    int max_token_len = 0; // used for optimizing longest token search

#ifdef VECTOR_ENABLE
    vector_mymap token_to_id;
#else
    mymap token_to_id;
#endif

    std::vector<token_data>       id_to_token;

    std::vector<id>    cache_special_tokens;
    std::vector<token> cache_token_to_piece; // llama_token_to_piece(special = true);

    std::map<std::pair<std::string, std::string>, int> bpe_ranks;

    // default LLaMA special tokens
    id special_bos_id  = 1;
    id special_eos_id  = 2;
    id special_unk_id  = 0;
    id special_sep_id  = -1;
    id special_pad_id  = -1;
    id special_cls_id  = -1;
    id special_mask_id = -1;

    id linefeed_id       = 13;
    id special_prefix_id = -1;
    id special_suffix_id = -1;
    id special_middle_id = -1;
    id special_eot_id    = -1; // TODO: move above after "eos_id", and here add "file separator" token

    // tokenizer flags
    bool tokenizer_add_space_prefix = false;
    bool tokenizer_add_bos          = false;
    bool tokenizer_add_eos          = false;
    bool tokenizer_ignore_merges    = false;
    bool tokenizer_clean_spaces     = false;  // clean_up_tokenization_spaces
    bool tokenizer_remove_extra_whitespaces   = false;
    bool tokenizer_escape_whitespaces         = true;
    bool tokenizer_treat_whitespace_as_suffix = false;

    int find_bpe_rank(const std::string & token_left, const std::string & token_right) const;

    std::vector<char> precompiled_charsmap;

};
/*
struct scalar_bigram_spm {
    struct comparator {
        bool operator()(scalar_bigram_spm & l, scalar_bigram_spm & r) {
            return (l.score < r.score) || (l.score == r.score && l.left > r.left);
        }
    };
    using queue_storage = std::vector<scalar_bigram_spm>;
    using queue = std::priority_queue<scalar_bigram_spm, queue_storage, comparator>;
    llm_symbol::index left;
    llm_symbol::index right;
    float score;
    size_t size;
};
*/

typedef enum FRAGMENT_BUFFER_VARIANT_TYPE {
    FRAGMENT_BUFFER_VARIANT_TYPE_TOKEN,
    FRAGMENT_BUFFER_VARIANT_TYPE_RAW_TEXT
} FRAGMENT_BUFFER_VARIANT_TYPE;

struct fragment_buffer_variant {
    fragment_buffer_variant(llama_vocab::id _token)
    :
        type(FRAGMENT_BUFFER_VARIANT_TYPE_TOKEN),
        token(_token),
        raw_text(_dummy),
        offset(0),
        length(0) {}

    fragment_buffer_variant(const std::string & _raw_text, int64_t _offset, int64_t _length)
    :
        type(FRAGMENT_BUFFER_VARIANT_TYPE_RAW_TEXT),
        token((llama_vocab::id) - 1),
        raw_text(_raw_text),
        offset(_offset),
        length(_length){
            /*GGML_ASSERT(_offset >= 0);
            GGML_ASSERT(_length >= 1);
            GGML_ASSERT(offset + length <= raw_text.length());*/
        }

    const FRAGMENT_BUFFER_VARIANT_TYPE type;
    const llama_vocab::id token;
    const std::string _dummy;
    const std::string & raw_text;
    const uint64_t offset;
    const uint64_t length;
};


int32_t llama_token_to_piece(const struct llama_vocab &vocab, llama_token token, char * buf, int32_t length, int32_t lstrip, bool special);
std::string llama_token_to_piece(const struct llama_vocab &vocab, llama_token token, bool special = true);
std::vector<llama_vocab::id> llama_tokenize_internal(const llama_vocab & vocab, std::string raw_text, bool add_special, bool parse_special);
size_t utf8_len(char src);
llama_token llama_byte_to_token(const llama_vocab & vocab, uint8_t ch);
void replace_all(std::string & s, const std::string & search, const std::string & replace);
void llama_escape_whitespace(std::string & text);
void llama_unescape_whitespace(std::string & word);
uint8_t llama_token_to_byte(const llama_vocab& vocab, llama_token id);
void tokenizer_st_partition(const llama_vocab & vocab, std::forward_list<fragment_buffer_variant> & buffer);
