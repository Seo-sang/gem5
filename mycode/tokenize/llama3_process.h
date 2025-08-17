#include <queue>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <string>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <time.h>
#include <forward_list>
#include <cassert>
#include <regex>
#include <locale>
#include <codecvt>

#include "unicode-data.h"
#include "gem5/m5ops.h"


#ifdef VECTOR_ENABLE
   #include "vector_mymap_template.h"
    #include "vector_pq_static_opt.h"
#else
    #include "mymap_template.h"
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

    enum llama_vocab_type     type     = LLAMA_VOCAB_TYPE_BPE;
    enum llama_vocab_pre_type type_pre = LLAMA_VOCAB_PRE_TYPE_LLAMA3;

    int max_token_len = 0; // used for optimizing longest token search

#ifdef VECTOR_ENABLE
    vector_mymap<std::string, int> token_to_id;
#else
    mymap<std::string, int> token_to_id;
#endif

    std::vector<token_data>       id_to_token;

    std::vector<id>    cache_special_tokens;
    std::vector<token> cache_token_to_piece; // llama_token_to_piece(special = true);
#ifdef VECTOR_ENABLE
    vector_mymap<std::pair<std::string, std::string>, int> bpe_ranks;
#else
    mymap<std::pair<std::string, std::string>, int> bpe_ranks;
#endif

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

    std::vector<char> precompiled_charsmap;

    int find_bpe_rank(const std::string & token_left, const std::string & token_right) const {

        auto it = bpe_ranks.find(std::make_pair(token_left, token_right));
        if (it == bpe_ranks.end()) {
            return -1;
        }

        return (*it).second;
    }

};

struct codepoint_flags {
    enum {
        UNDEFINED       = 0x0001,
        NUMBER          = 0x0002,  // regex: \p{N}
        LETTER          = 0x0004,  // regex: \p{L}
        SEPARATOR       = 0x0008,  // regex: \p{Z}
        ACCENT_MARK     = 0x0010,  // regex: \p{M}
        PUNCTUATION     = 0x0020,  // regex: \p{P}
        SYMBOL          = 0x0040,  // regex: \p{S}
        CONTROL         = 0x0080,  // regex: \p{C}
        MASK_CATEGORIES = 0x00FF,
    };

    // codepoint type
    uint16_t is_undefined   : 1;
    uint16_t is_number      : 1;  // regex: \p{N}
    uint16_t is_letter      : 1;  // regex: \p{L}
    uint16_t is_separator   : 1;  // regex: \p{Z}
    uint16_t is_accent_mark : 1;  // regex: \p{M}
    uint16_t is_punctuation : 1;  // regex: \p{P}
    uint16_t is_symbol      : 1;  // regex: \p{S}
    uint16_t is_control     : 1;  // regex: \p{C}
    // helper flags
    uint16_t is_whitespace  : 1;  // regex: \s
    uint16_t is_lowercase   : 1;
    uint16_t is_uppercase   : 1;
    uint16_t is_nfd         : 1;

    // decode from uint16
    inline codepoint_flags(const uint16_t flags=0) {
        *reinterpret_cast<uint16_t*>(this) = flags;
    }

    inline uint16_t as_uint() const {
        return *reinterpret_cast<const uint16_t*>(this);
    }

    inline uint16_t category_flag() const {
        return this->as_uint() & MASK_CATEGORIES;
    }
};

/*
struct llm_bigram_bpe {
    struct comparator {
        bool operator()(const llm_bigram_bpe & l, const llm_bigram_bpe & r) const {
            return l.rank > r.rank || (l.rank == r.rank && l.left > r.left);
        }
    };

    using queue_storage = std::vector<llm_bigram_bpe>;
    using queue = std::priority_queue<llm_bigram_bpe, queue_storage, comparator>;
    llm_symbol::index left;
    llm_symbol::index right;
    std::string text;
    int rank;
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

std::vector<llama_vocab::id> llama3_tokenize_internal(const llama_vocab & vocab, std::string raw_text, bool add_special, bool parse_special);
std::vector<std::string> unicode_regex_split(const std::string & text, const std::vector<std::string> & regex_exprs);
size_t utf8_len(char src);
std::vector<uint32_t> unicode_cpts_from_utf8(const std::string & utf8);
void tokenizer_st_partition(const llama_vocab & vocab, std::forward_list<fragment_buffer_variant> & buffer);
codepoint_flags unicode_cpt_flags(const uint32_t cp);
codepoint_flags unicode_cpt_flags(const std::string & utf8);
static std::vector<size_t> unicode_regex_split_custom(const std::string & text, const std::string & regex_expr, const std::vector<size_t> & offsets);
static std::vector<size_t> unicode_regex_split_stl(const std::wstring & wtext, const std::wstring & regex_expr, const std::vector<size_t> & offsets);
std::string unicode_cpt_to_utf8(uint32_t cp);
static inline std::wstring unicode_wstring_from_utf8(const std::string & s);
static std::vector<std::string> unicode_byte_encoding_process(const std::vector<std::string> & bpe_words);
uint32_t unicode_cpt_from_utf8(const std::string & utf8, size_t & offset);
static std::vector<codepoint_flags> unicode_cpt_flags_array();
static std::vector<size_t> unicode_regex_split_custom_llama3(const std::string & text, const std::vector<size_t> & offsets);
uint32_t unicode_tolower(uint32_t cp);
std::string unicode_byte_to_utf8(uint8_t byte);
static std::unordered_map<uint8_t, std::string> unicode_byte_to_utf8_map();
std::wstring string_to_wstring(const std::string& str);
std::string llama_token_to_piece(const struct llama_vocab &vocab, llama_token token, bool special = true);
int32_t llama_token_to_piece(const struct llama_vocab &vocab, llama_token token, char * buf, int32_t length, int32_t lstrip, bool special);
void llama_unescape_whitespace(std::string & word);
uint8_t llama_token_to_byte(const llama_vocab& vocab, llama_token id);
void replace_all(std::string & s, const std::string & search, const std::string & replace);
