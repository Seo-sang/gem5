#include "process_seg.h"

void replace_all(std::string & s, const std::string & search, const std::string & replace) {
#ifdef DEBUG_ENABLE
    printf("--replace_all start\n");
#endif
    std::string result;
    for (size_t pos = 0; ; pos += search.length()) {
        auto new_pos = s.find(search, pos);
        if (new_pos == std::string::npos) {
            result += s.substr(pos, s.size() - pos);
            break;
        }
        result += s.substr(pos, new_pos - pos) + replace;
        pos = new_pos;
    }
    s = std::move(result);
#ifdef DEBUG_ENABLE
    printf("--replace_all end\n");
#endif
}

void llama_escape_whitespace(std::string & text) {
    replace_all(text, " ", "\xe2\x96\x81");
}

void llama_unescape_whitespace(std::string & word) {
    replace_all(word, "\xe2\x96\x81", " ");
}

llama_token llama_byte_to_token(const llama_vocab & vocab, uint8_t ch) {

#ifdef DEBUG_ENABLE
    printf("---llama_byte_to_token start\n");
#endif
    //GGML_ASSERT(llama_vocab_get_type(vocab) != LLAMA_VOCAB_TYPE_NONE);
    static const char * hex = "0123456789ABCDEF";
    const char buf[7] = { '<', '0', 'x', hex[ch >> 4], hex[ch & 15], '>', 0 };

    auto token = vocab.token_to_id.find(buf);

#ifdef DEBUG_ENABLE
    printf("---llama_byte_to_token end\n");
#endif
    if (token != vocab.token_to_id.end()) {
        return (*token).second;
    }
    // Try to fall back to just the byte as a string
    const char buf2[2] = { (char)ch, 0 };

    return vocab.token_to_id.at(buf2);
}

size_t utf8_len(char src) {
    const size_t lookup[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4 };
    uint8_t highbits = static_cast<uint8_t>(src) >> 4;
    return lookup[highbits];
}

struct llm_tokenizer_spm {
    llm_tokenizer_spm(const llama_vocab & vocab) : vocab(vocab) {}

    void store_model() {
        std::ofstream fout("token_priority.model");

        for(llama_vocab::token_data elem : vocab.id_to_token) {
            fout << elem.text << ' ' << elem.score << '\n';
        }
    }

    void tokenize(const std::string & text, std::vector<llama_vocab::id> & output) {
#ifdef DEBUG_ENABLE
        printf("---tokenize start\n");
#endif

        //store_model();

        // split string into utf8 chars
        int index = 0;
        size_t offs = 0;

        while (offs < text.size()) {
            llm_symbol sym;
            size_t len = utf8_len(text[offs]); //utf-8 표현에서의 길이 계산(앞 4bit로 판단 가능)
            sym.text = text.c_str() + offs;
            sym.n = std::min(len, text.size() - offs);
            offs += sym.n; //다음 offset 변경
            sym.prev = index - 1;
            sym.next = offs == text.size() ? -1 : index + 1;
            index++;
            symbols.emplace_back(sym);
        }


        // seed the work queue with all possible 2-character tokens.
        for (size_t i = 1; i < symbols.size(); ++i) {
            try_add_bigram(i - 1, i);
        }

        // keep substituting the highest frequency pairs for as long as we can.
        while (!my_work_queue.empty()) {

#ifdef DEBUG_ENABLE
            printf("cycle start\n");
            printf("queue size: %d\n", my_work_queue.size());
#endif
            auto bigram = my_work_queue.top(); //score가 낮은 것부터 먼저 뽑음
            my_work_queue.pop();

#ifdef DEBUG_ENABLE
            printf("tokenize | bigram.left: %d\n", bigram.left);
            printf("tokenize | bigram.right: %d\n", bigram.right);
#endif
            auto & left_sym = symbols[bigram.left];
            auto & right_sym = symbols[bigram.right];

            // if one of the symbols already got merged, skip it.
            if (left_sym.n == 0 || right_sym.n == 0 ||
                left_sym.n + right_sym.n != bigram.size) {
#ifdef DEBUG_ENABLE
            printf("cycle end\n");
#endif
                continue;
            }


            // merge the right sym into the left one
            left_sym.n += right_sym.n;
            right_sym.n = 0;

            // remove the right sym from the chain
            left_sym.next = right_sym.next;
            if (right_sym.next >= 0) {
                symbols[right_sym.next].prev = bigram.left;
            }

            // find more substitutions
            try_add_bigram(left_sym.prev, bigram.left);
            try_add_bigram(bigram.left, left_sym.next);
#ifdef DEBUG_ENABLE
            printf("cycle end\n");
#endif
        }

        for (int i = 0; i != -1; i = symbols[i].next) {
            auto & symbol = symbols[i];
            resegment(symbol, output);
        }

        /*
        std::ofstream outFile("token_length", std::ios::app);
        if(outFile.is_open()) {
            outFile << output.size() << std::endl;
            std::cout << "Token length is written to the file,\n";
        }

        outFile.close();
        */

#ifdef DEBUG_ENABLE
        printf("---tokenize end\n");
#endif
    }

private:
    void resegment(llm_symbol & symbol, std::vector<llama_vocab::id> & output) {
#ifdef DEBUG_ENABLE
    printf("----resegment start\n");
#endif
        auto text = std::string(symbol.text, symbol.n);

        //clock_t hash_start = clock();
        auto token = vocab.token_to_id.find(text.c_str());
        //clock_t hash_end = clock();

        //hash_time += (double)(hash_end - hash_start);

        // Do we need to support is_unused?
        if (token != vocab.token_to_id.end()) {
            output.push_back((*token).second);
            return;
        }

        const auto p = rev_merge.find(text);

        if (p == rev_merge.end()) { //vocab에 없는데 merge된 것도 아닌 경우 여러 char로 이루어진 symbol임
            // output any symbols that did not form tokens as bytes.
            output.reserve(output.size() + symbol.n);
            for (int j = 0; j < (int)symbol.n; ++j) {
                llama_vocab::id token_id = llama_byte_to_token(vocab, symbol.text[j]); //
                output.push_back(token_id);
            }
            return;
        }

        //merge된 것인데 vocab에 없는 경우
        //vocab에 있을 경우만 merge되는데
        //실행되나?
        resegment(symbols[p->second.first],  output);
        resegment(symbols[p->second.second], output);
#ifdef DEBUG_ENABLE
    printf("----resegment end\n");
#endif
    }

    void try_add_bigram(int left, int right) {
#ifdef DEBUG_ENABLE
    //printf("----try_add_bigram start\n");
#endif
        if (left == -1 || right == -1) {
            return;
        }

        const std::string text = std::string(symbols[left].text, symbols[left].n + symbols[right].n); //2개의 char를 이어붙여서 text 생성

        //clock_t hash_start = clock();
        auto token = vocab.token_to_id.find(text.c_str()); //vocab에 text가 있는지 확인
        //clock_t hash_end = clock();

        //hash_time += (double)(hash_end - hash_start);

        if (token == vocab.token_to_id.end()) { //없으면 리턴
#ifdef DEBUG_ENABLE
            //printf("----try_add_bigram end\n");
#endif
            return;
        }

        if (static_cast<size_t>((*token).second) >= vocab.id_to_token.size()) { //위치가 vocab 크기를 넘어가도 리턴
            return;
        }

        const auto & tok_data = vocab.id_to_token[(*token).second];

//#ifdef VECTOR_ENABLE
        llm_bigram_spm bigram;
//#else
        //scalar_bigram_spm bigram;
//#endif
        bigram.left  = left;
        bigram.right = right;
        bigram.score = tok_data.score;
        bigram.size  = text.size();

#ifdef DEBUG_ENABLE
        printf("try_add_bigram | bigram.left: %d\n", bigram.left);
        printf("try_add_bigram | bigram.right: %d\n", bigram.right);
#endif
        //max_token_len = std::max(max_token_len, (int)bigram.size);

        //clock_t push_start = clock();
        my_work_queue.push(bigram);
        //clock_t push_end = clock();

        //push_time += (double)(push_end - push_start);

        // Do we need to support is_unused?
        rev_merge[text] = std::make_pair(left, right); //bigram으로 merge 여부 확인

#ifdef DEBUG_ENABLE
   // printf("----try_add_bigram end\n");
#endif
    }

    const llama_vocab & vocab;

    std::vector<llm_symbol> symbols;


    //llm_bigram_spm::queue work_queue;
//#ifdef VECTOR_ENABLE
    class my_pq my_work_queue; //use my vector priority queue
//#else
    //scalar_bigram_spm::queue my_work_queue;
//#endif

    std::map<std::string, std::pair<int, int>> rev_merge;

    int max_token_len = 0;

    double hash_time = 0;
    double push_time = 0;

};

int32_t llama_token_to_piece(const struct llama_vocab &vocab, llama_token token, char * buf, int32_t length, int32_t lstrip, bool special) {
    // ref: https://github.com/ggerganov/llama.cpp/pull/7587#discussion_r1620983843
    static const int attr_special = LLAMA_TOKEN_ATTR_UNKNOWN | LLAMA_TOKEN_ATTR_CONTROL;
    const llama_token_attr attr = vocab.id_to_token[token].attr;
    if (!special && (attr & attr_special)) {
        return 0;
    }

    // copy piece chars to output text buffer
    // skip up to 'lstrip' leading spaces before copying
    auto _try_copy = [=] (const char * token, size_t size) -> int32_t {
        for (int32_t i = 0; i < lstrip && size && *token == ' '; ++i) {
            token++;
            size--;
        }
        if (length < (int32_t)size) {
            return (int32_t) -size;
        }
        memcpy(buf, token, size);
        return (int32_t) size;
    };

    // if we have a cache - use it
    {
        const auto & cache = vocab.cache_token_to_piece;

        if (!cache.empty()) {
            const auto & result = cache.at(token);
            return _try_copy(result.data(), result.size());
        }
    }

    if (0 <= token && token < vocab.id_to_token.size()) {
        const std::string & token_text = vocab.id_to_token[token].text;

        if (attr & (attr_special | LLAMA_TOKEN_ATTR_USER_DEFINED)) {
            return _try_copy(token_text.data(), token_text.size());
        } else if (attr & LLAMA_TOKEN_ATTR_NORMAL) {
            std::string result = token_text;
            llama_unescape_whitespace(result);
            return _try_copy(result.data(), result.size());
        } else if (attr & LLAMA_TOKEN_ATTR_BYTE) {
            char byte = (char) llama_token_to_byte(vocab, token);
            return _try_copy((char*) &byte, 1);
        }
    }
    return 0;
}


std::string llama_token_to_piece(const struct llama_vocab &vocab, llama_token token, bool special) {
#ifdef DEBUG_ENABLE
    printf("--llama_token_to_piece start\n");
#endif
    std::string piece;
    piece.resize(piece.capacity());  // using string internal cache, 15 bytes + '\n'
    const int n_chars = llama_token_to_piece(vocab, token, &piece[0], piece.size(), 0, special);
    if (n_chars < 0) {
        piece.resize(-n_chars);
        int check = llama_token_to_piece(vocab, token, &piece[0], piece.size(), 0, special);
        //GGML_ASSERT(check == -n_chars);
    }
    else {
        piece.resize(n_chars);
    }

#ifdef DEBUG_ENABLE
    printf("--llama_token_to_piece end\n");
#endif
    return piece;
}


std::vector<llama_vocab::id> llama_tokenize_internal(const llama_vocab & vocab, std::string raw_text, bool add_special, bool parse_special) {

#ifdef DEBUG_ENABLE
    printf("-llama_tokenize_internal start\n");
#endif
    //store_vocab(vocab);

    std::vector<llama_vocab::id> output;
    std::forward_list<fragment_buffer_variant> fragment_buffer;

    //초기 fragment_buffer는 입력 sentence
    if (!raw_text.empty()) {
        fragment_buffer.emplace_front(raw_text, 0, raw_text.length());
        if (parse_special) tokenizer_st_partition(vocab, fragment_buffer);
    }

    // OG tokenizer behavior:
    //
    // tokenizer.encode('', add_special_tokens=True)  returns [1]
    // tokenizer.encode('', add_special_tokens=False) returns []

    bool is_prev_special = true;  // prefix with space if first token

    if (add_special && vocab.tokenizer_add_bos) {
        //GGML_ASSERT(vocab.special_bos_id != -1);
        output.push_back(vocab.special_bos_id);
        is_prev_special = true;
    }


    llm_tokenizer_spm tokenizer(vocab);
    for (const auto & fragment : fragment_buffer) {
        //printf("--------------------\n");
        //printf("fragment: %s\n", fragment.raw_text.c_str());

        if (fragment.type == FRAGMENT_BUFFER_VARIANT_TYPE_RAW_TEXT) {
            //printf("type: raw_text\n");
            auto raw_text = fragment.raw_text.substr(fragment.offset, fragment.length);

            // prefix with space if previous is special
            if (vocab.tokenizer_add_space_prefix && is_prev_special) {
                raw_text = " " + raw_text;
            }
            llama_escape_whitespace(raw_text); //whitespace -> '_'(under score, visible space)로 변경
                //printf("raw_text: %s\n", raw_text.c_str());
            m5_reset_stats(0, 0);
            //m5_work_begin(0, 0);
            tokenizer.tokenize(raw_text, output);
            //m5_work_end(0, 0);
            m5_dump_stats(0, 0);
            is_prev_special = false;
        } else { // if (fragment.type == FRAGMENT_BUFFER_VARIANT_TYPE_TOKEN)
            //printf("type: token\n");
            output.push_back(fragment.token);
            is_prev_special = true;
        }
    }

    if (add_special && vocab.tokenizer_add_bos && output.size() >= 2 && output[1] == vocab.special_bos_id) {
        /*
        LLAMA_LOG_WARN(
            "%s: Added a BOS token to the prompt as specified by the model but the prompt "
            "also starts with a BOS token. So now the final prompt starts with 2 BOS tokens. "
            "Are you sure this is what you want?\n", __FUNCTION__);
            */
    }

    if (add_special && vocab.tokenizer_add_eos) {
        //GGML_ASSERT(vocab.special_eos_id != -1);
        output.push_back(vocab.special_eos_id);
    }
#ifdef DEBUG_ENABLE
    printf("-llama_tokenize_internal end\n");
#endif
    return output;
}

uint8_t llama_token_to_byte(const llama_vocab& vocab, llama_token id) {
    const auto & token_data = vocab.id_to_token.at(id);
    auto buf = token_data.text.substr(3, 2);
    return strtol(buf.c_str(), NULL, 16);
}

void tokenizer_st_partition(const llama_vocab & vocab, std::forward_list<fragment_buffer_variant> & buffer) {
    // for each special token
    for (const llama_vocab::id special_id : vocab.cache_special_tokens) {
        const auto & data = vocab.id_to_token[special_id];
        const auto & special_token = data.text;

        // for each text fragment
        std::forward_list<fragment_buffer_variant>::iterator it = buffer.begin();
        while (it != buffer.end()) {
            auto & fragment = (*it);

            // if a fragment is text ( not yet processed )
            if (fragment.type == FRAGMENT_BUFFER_VARIANT_TYPE_RAW_TEXT) {
                auto & raw_text = fragment.raw_text;

                auto raw_text_base_offset = fragment.offset;
                auto raw_text_base_length = fragment.length;

                // loop over the text
                while (true) {
                    // find the first occurrence of a given special token in this fragment
                    //  passing offset argument only limit the "search area" but match coordinates
                    //  are still relative to the source full raw_text
                    auto match = raw_text.find(special_token, raw_text_base_offset);

                    // no occurrences found, stop processing this fragment for a given special token
                    if (match == std::string::npos) break;

                    // check if match is within bounds of offset <-> length
                    if (match + special_token.length() > raw_text_base_offset + raw_text_base_length) break;

                    auto source = std::distance(buffer.begin(), it);

                    // if match is further than base offset
                    //  then we have some text to the left of it
                    if (match > raw_text_base_offset) {
                        // left
                        const int64_t left_reminder_offset = raw_text_base_offset + 0;
                        int64_t left_reminder_length = match - raw_text_base_offset;

                        if (data.attr & LLAMA_TOKEN_ATTR_LSTRIP) {
                            while (left_reminder_length > 0 && isspace(raw_text[left_reminder_offset + left_reminder_length - 1])) {
                                left_reminder_length--;
                            }
                        }

                        if (left_reminder_length > 0) {
                            buffer.emplace_after(it, raw_text, left_reminder_offset, left_reminder_length);
                            it++;
                        }
                    }

                    // special token
                    buffer.emplace_after(it, special_id);
                    it++;

                    // right
                    if (match + special_token.length() < raw_text_base_offset + raw_text_base_length) {
                        int64_t right_reminder_offset = match + special_token.length();
                        int64_t right_reminder_length = raw_text_base_length - ((match - raw_text_base_offset) + special_token.length());

                        if (data.attr & LLAMA_TOKEN_ATTR_RSTRIP) {
                            while (right_reminder_length > 0 && isspace(raw_text[right_reminder_offset])) {
                                right_reminder_offset++;
                                right_reminder_length--;
                            }
                        }

                        if (right_reminder_length > 0) {
                            buffer.emplace_after(it, raw_text, right_reminder_offset, right_reminder_length);
                            it++;
                        }

                        if (source == 0) {
                            buffer.erase_after(buffer.before_begin());
                        } else {
                            buffer.erase_after(std::next(buffer.begin(), (source-1)));
                        }

                        // repeat for the right side
                        raw_text_base_offset = right_reminder_offset;
                        raw_text_base_length = right_reminder_length;

                    } else {
                        if (source == 0) {
                            buffer.erase_after(buffer.before_begin());
                        } else {
                            buffer.erase_after(std::next(buffer.begin(), (source-1)));
                        }
                        break;
                    }
                }
            }
            it++;
        }
    }
}
