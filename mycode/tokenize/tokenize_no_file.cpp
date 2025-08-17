//#include "common.h"
// #include "llama.h"

#include "process.h"
#include "tokens.h"

#include <cstring>
#include <time.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <locale>
#include <codecvt>
/*
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>   // For CommandLineToArgvW
#endif
*/
static std::vector<std::string> ingest_args(int raw_argc, char ** raw_argv) {
    std::vector<std::string> argv;

    int argc = raw_argc;
    for (int i = 0; i < argc; ++i) {
        argv.push_back(raw_argv[i]);
    }
    return argv;
}

//
// Function: write_utf8_cstr_to_stdout(const char *) -> <writes to stdout>
//
// writes a string to standard output; taking into account that on Windows
// to display correctly you have to use special handling. Works even if the
// user has not set a unicode code page on a Windows cmd.exe.
//
// In case of invalid UTF-8, invalid_utf8 is set to true on Windows, and something
// a human-readable is written instead.
//
// On non-Windows systems, simply printfs() the string.
static void write_utf8_cstr_to_stdout(const char * str, bool & invalid_utf8) {
        invalid_utf8 = false;

#if defined(_WIN32)
        // Are we in a console?
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;

        // According to Microsoft docs:
        // "WriteConsole fails if it is used with a standard handle that is redirected to a file."
        // Also according to the docs, you can use GetConsoleMode to check for that.
        if (hConsole == INVALID_HANDLE_VALUE || !GetConsoleMode(hConsole, &dwMode)) {
            printf("%s", str);
            return;
        }

        // MultiByteToWideChar reports an error if str is empty, don't report
        // them as invalid_utf8.
        if (*str == 0) {
            return;
        }
        int length_needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, strlen(str), NULL, 0);
        if (length_needed == 0) {
            DWORD err = GetLastError();
            if (err == ERROR_NO_UNICODE_TRANSLATION) {
                invalid_utf8 = true;
                int len = strlen(str);
                printf("<");
                for (int i = 0; i < len; ++i) {
                    if (i > 0) {
                        printf(" ");
                    }
                    printf("%02x", (uint8_t) str[i]);
                }
                printf(">");
                return;
            }
            //GGML_ASSERT(false && "MultiByteToWideChar() failed in an unexpected way.");
        }

        LPWSTR wstr = (LPWSTR) calloc(length_needed+1, sizeof(*wstr));
        //GGML_ASSERT(wstr);

        MultiByteToWideChar(CP_UTF8, 0, str, strlen(str), wstr, length_needed);
        WriteConsoleW(hConsole, wstr, length_needed, NULL, NULL);

        free(wstr);
#else
        // TODO: reporting invalid_utf8 would be useful on non-Windows too.
        // printf will silently just write bad unicode.
        printf("%s", str);
#endif
}


void load_specials(struct llama_vocab *vocab) {
    //std::ifstream fin(std::string(path) + std::string("special_tokens.txt"));
    int tmp = 0;
    //fin >> tmp;

    vocab->type = (enum llama_vocab_type)0;//(enum llama_vocab_type)tmp;
    //fin >> tmp;
    vocab->type_pre = (enum llama_vocab_pre_type)0;//(enum llama_vocab_pre_type)tmp;
    //fin >> vocab->max_token_len;

    //default special tokens

    vocab->special_bos_id = 1;
    vocab->special_eos_id = 2;
    vocab->special_unk_id = 0;
    vocab->special_sep_id = -1;
    vocab->special_pad_id = -1;
    vocab->special_cls_id = -1;
    vocab->special_mask_id = -1;
    vocab->linefeed_id = 1;
    vocab->special_prefix_id = 3;
    vocab->special_suffix_id = -1;
    vocab->special_middle_id = -1;
    vocab->special_eot_id = -1;

    vocab->tokenizer_add_space_prefix = 0;
    vocab->tokenizer_add_bos = 0;
    vocab->tokenizer_add_eos = 0;
    vocab->tokenizer_ignore_merges = 0;
    vocab->tokenizer_clean_spaces = 0;
    vocab->tokenizer_remove_extra_whitespaces = 0;
    vocab->tokenizer_escape_whitespaces = 1;
    vocab->tokenizer_treat_whitespace_as_suffix = 0;
}


void load_id_to_token(struct llama_vocab* vocab) {

    int num = 0;

    char buffer[49];
    int len;

    struct llama_vocab::token_data data;

    while(num < 32000) {\
        data.text = vocabulary[num].str;
        data.score = vocabulary[num].score;
        data.attr = (enum llama_token_attr)vocabulary[num].attr;

        vocab->id_to_token.push_back(data);
        vocab->token_to_id.push(data.text, vocab->id_to_token.size() - 1);

        num++;
    }

}


void load_vocab(struct llama_vocab *vocab) {
    load_specials(vocab);
    load_id_to_token(vocab);
}

int main(int raw_argc, char* raw_argv[]) {
    //raw_argc = 5;
    //char* raw_argv[] = {" ", "-m", "Llama-2-7B-Chat-GGUF/llama-2-7b-chat.Q8_0.gguf", "-f", "prompts/test.txt"};
    const std::vector<std::string> argv = ingest_args(raw_argc, raw_argv);
    //const std::vector<std::string> argv = {"", "-f", "./prompt.txt"};
    const int argc = argv.size();


    //////
    // Read out all the command line arguments.
    //////

    // variables where to put any arguments we see.
    bool printing_ids = false;
    //bool no_bos = false;
    //bool disable_logging = false;

    //////
    // Sanity check the command line arguments.

    std::string prompt = "Unlike SIMD datapaths in VLIW cores, a Saturn-like short-vector design does not require the high instruction throughput of VLIW fetch. A short-vector machine’s more aggressive capability for dynamic instruction scheduling also diminishes the need for precisely scheduled microarchitecture-aware code, compared to VLIW-SIMD designs.";

    struct llama_vocab vocab;

    load_vocab(&vocab);

    //vocab.token_to_id.print();

    // read entire prompt from stdin?

    //const bool model_wants_add_bos = llama_should_add_bos_token(model);
    //const bool add_bos = model_wants_add_bos && !no_bos;
    const bool add_bos = true;

    //printf("add_bos: %d\n", add_bos);

    //clock_t token_start = clock();


    std::vector<llama_token> tokens;
    tokens = llama_tokenize_internal(vocab, prompt, add_bos, true);

    //write_vocab(model);

//    clock_t token_end = clock();
    //printf("Total time: %lf\n", (double)(token_end - token_start) / CLOCKS_PER_SEC);


    if (printing_ids) {
        printf("[");
    }

    for (int i = 0; i < (int) tokens.size(); i++) {
        if (printing_ids) {
            if (i > 0) {
                printf(", ");
            }
            printf("%d", tokens[i]);
        } else {
            bool invalid_utf8 = false;
            printf("%6d -> '", tokens[i]);
            write_utf8_cstr_to_stdout(llama_token_to_piece(vocab, tokens[i]).c_str(), invalid_utf8);
            if (invalid_utf8) {
                printf("' (utf-8 decode failure)\n");
            } else {
                printf("'\n");
            }
        }
    }

    if (printing_ids) {
        printf("]\n");
    }


    return 0;
}
