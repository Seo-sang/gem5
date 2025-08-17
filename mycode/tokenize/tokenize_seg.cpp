//#include "common.h"
// #include "llama.h"

#ifdef LLAMA3
    #include "llama3_process.h"
#else
    #include "process_seg.h"
#endif

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

static void print_usage_information(const char * argv0, FILE * stream) {
    fprintf(stream, "usage: %s [options]\n\n", argv0);
    fprintf(stream, "The tokenize program tokenizes a prompt using a given model,\n");
    fprintf(stream, "and prints the resulting tokens to standard output.\n\n");
    fprintf(stream, "It needs a model file, a prompt, and optionally other flags\n");
    fprintf(stream, "to control the behavior of the tokenizer.\n\n");
    fprintf(stream, "    The possible options are:\n");
    fprintf(stream, "\n");
    fprintf(stream, "    -h, --help                           print this help and exit\n");
    fprintf(stream, "    -m MODEL_PATH, --model MODEL_PATH    path to model.\n");
    fprintf(stream, "    --ids                                if given, only print numerical token IDs, and not token strings.\n");
    fprintf(stream, "                                         The output format looks like [1, 2, 3], i.e. parseable by Python.\n");
    fprintf(stream, "    -f PROMPT_FNAME, --file PROMPT_FNAME read prompt from a file.\n");
    fprintf(stream, "    -p PROMPT, --prompt PROMPT           read prompt from the argument.\n");
    fprintf(stream, "    --stdin                              read prompt from standard input.\n");
    fprintf(stream, "    --no-bos                             do not ever add a BOS token to the prompt, even if normally the model uses a BOS token.\n");
    fprintf(stream, "    --log-disable                        disable logs. Makes stderr quiet when loading the model.\n");
    fprintf(stream, "    --show-count                         print the total number of tokens.\n");
}


static std::string read_prompt_from_file(const char * filepath, bool & success) {
    success = false;

    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        fprintf(stderr, "%s: could not open file '%s' for reading: %s\n", __func__, filepath, strerror(errno));
        return std::string();
    }
    // do not assume the file is seekable (e.g. /dev/stdin)
    std::stringstream buffer;
    buffer << in.rdbuf();
    if (in.fail()) {
        fprintf(stderr, "%s: could not read the entire file '%s': %s\n", __func__, filepath, strerror(errno));
        return std::string();
    }

    success = true;
    return buffer.str();
}

//
// Function: ingest_args(...) -> vector<string>
//
//  Takes argc and argv arguments, and converts them to a vector of UTF-8 encoded
//  strings, as an STL vector<string>.
//
//  In particular, it handles character encoding shenanigans on Windows.
//
// Note: raw_argc and raw_argv are not actually read at all on Windows.
//       On Windows we call GetCommandLineW to get the arguments in wchar_t
//       format, ignoring the regular argc/argv arguments to main().
//
// TODO: potential opportunity to roll common stuff into common/console.cpp
//       in relation to Windows wchar_t shenanigans.
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

#ifdef LLAMA3

void llama3_load_specials(const char *path, struct llama_vocab *vocab) {
    std::ifstream fin(std::string(path) + std::string("/llama3_special_tokens.txt"));
    int tmp;
    fin >> tmp;
    vocab->type = (enum llama_vocab_type)tmp;
    fin >> tmp;
    vocab->type_pre = (enum llama_vocab_pre_type)tmp;
    fin >> vocab->max_token_len;

    //default special tokens
    fin >> vocab->special_bos_id;
    fin >> vocab->special_eos_id;
    fin >> vocab->special_unk_id;
    fin >> vocab->special_sep_id;
    fin >> vocab->special_pad_id;
    fin >> vocab->special_cls_id;
    fin >> vocab->special_mask_id;
    fin >> vocab->linefeed_id;
    fin >> vocab->special_prefix_id;
    fin >> vocab->special_suffix_id;
    fin >> vocab->special_middle_id;
    fin >> vocab->special_eot_id;

    //flags
    fin >> vocab->tokenizer_add_space_prefix;
    fin >> vocab->tokenizer_add_bos;
    fin >> vocab->tokenizer_add_eos;
    fin >> vocab->tokenizer_ignore_merges;
    fin >> vocab->tokenizer_clean_spaces;
    fin >> vocab->tokenizer_remove_extra_whitespaces;
    fin >> vocab->tokenizer_escape_whitespaces;
    fin >> vocab->tokenizer_treat_whitespace_as_suffix;

    fin.close();
}

void llama3_load_id_to_token(const char * path, struct llama_vocab* vocab) {
    std::ifstream fin(std::string(path)+ std::string("/llama3_id_to_token.txt"), std::ios::binary);

    //load total tokens
    int num;
    fin >> num;

    char buffer[260];
    int len;

    struct llama_vocab::token_data data;

    while(num--) {
        memset(buffer, 0, sizeof(buffer));

        //utf8 length 읽기
        fin >> len;
        fin.seekg(1, std::ios::cur); //공백제거

        //data읽기
        fin.read(buffer, len); //data 읽기
        fin.seekg(1, std::ios::cur); //공백제거

        //utf8 to string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wstr = converter.from_bytes(buffer);
        data.text = converter.to_bytes(wstr);

        float score_tmp;
        //id 읽기
        fin >> score_tmp;
        data.score = score_tmp;

        int tmp;
        fin >> tmp;
        data.attr = (enum llama_token_attr)tmp;

        vocab->id_to_token.push_back(data);
        vocab->token_to_id.push(data.text, vocab->id_to_token.size() - 1);
    }

    fin.close();
}
#endif

void load_specials(const char *path, struct llama_vocab *vocab) {
    std::ifstream fin(std::string(path) + std::string("/special_tokens.txt"));
    int tmp;
    fin >> tmp;
    vocab->type = (enum llama_vocab_type)tmp;
    fin >> tmp;
    vocab->type_pre = (enum llama_vocab_pre_type)tmp;
    fin >> vocab->max_token_len;

    //default special tokens
    fin >> vocab->special_bos_id;
    fin >> vocab->special_eos_id;
    fin >> vocab->special_unk_id;
    fin >> vocab->special_sep_id;
    fin >> vocab->special_pad_id;
    fin >> vocab->special_cls_id;
    fin >> vocab->special_mask_id;
    fin >> vocab->linefeed_id;
    fin >> vocab->special_prefix_id;
    fin >> vocab->special_suffix_id;
    fin >> vocab->special_middle_id;
    fin >> vocab->special_eot_id;

    //flags
    fin >> vocab->tokenizer_add_space_prefix;
    fin >> vocab->tokenizer_add_bos;
    fin >> vocab->tokenizer_add_eos;
    fin >> vocab->tokenizer_ignore_merges;
    fin >> vocab->tokenizer_clean_spaces;
    fin >> vocab->tokenizer_remove_extra_whitespaces;
    fin >> vocab->tokenizer_escape_whitespaces;
    fin >> vocab->tokenizer_treat_whitespace_as_suffix;

    fin.close();
}

/*
void llama3_load_cache_special_tokens(const char *path, struct llama_vocab *vocab) {
    std::ifstream fin(std::string(path) + std::string("llama3_cache_special_tokens.txt"));
    int len;
    fin >> len;

    int tmp;
    while(len--) {
        fin >> tmp;
        vocab->cache_special_tokens.push_back(tmp);
    }

    fin.close();
}

void llama3_load_cache_token_to_piece(const char *path, struct llama_vocab *vocab) {
    std::ifstream fin(std::string(path) + std::string("llama3_cache_token_to_piece.txt"));
    int len;
    fin >> len;

    int tmp;
    while(len--) {
        fin >> tmp;
        vocab->cache_special_tokens.push_back(tmp);
    }

    fin.close();
}
*/
#ifdef LLAMA3
void llama3_load_bpe_ranks(const char *path, struct llama_vocab *vocab) {
    std::ifstream fin(std::string(path) + std::string("/llama3_bpe_ranks.txt"));
    int len;
    fin >> len;
    char buffer[260];

    int tmp1, tmp2, tmp3;
    while(len--) {
        memset(buffer, 0, sizeof(buffer));
        fin >> tmp1;
        fin.seekg(1, std::ios::cur); //공백제거

        fin.read(buffer, tmp1);
        fin.seekg(1, std::ios::cur); //공백제거

        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wstr1 = converter.from_bytes(buffer);
        std::string str1 = converter.to_bytes(wstr1);
        // printf("str1: %s\n", buffer);

        memset(buffer, 0, sizeof(buffer));

        fin >> tmp2;
        fin.seekg(1, std::ios::cur); //공백제거
        fin.read(buffer, tmp2);
        // printf("str2: %s\n", buffer);


        std::wstring wstr2 = converter.from_bytes(buffer);
        std::string str2 = converter.to_bytes(wstr2);

        fin >> tmp3;
        // printf("tmp3: %d\n", tmp3);

        vocab->bpe_ranks.push(std::make_pair(str1, str2), tmp3);
    }

    fin.close();
}
#endif

void load_id_to_token(const char * path, struct llama_vocab* vocab) {
    std::ifstream fin(std::string(path)+ std::string("/id_to_token.txt"), std::ios::binary);

    //load total tokens
    int num;
    fin >> num;

    char buffer[49];
    int len;

    struct llama_vocab::token_data data;

    while(num--) {
        memset(buffer, 0, sizeof(buffer));

        //utf8 length 읽기
        fin >> len;
        fin.seekg(1, std::ios::cur); //공백제거

        //data읽기
        fin.read(buffer, len); //data 읽기
        fin.seekg(1, std::ios::cur); //공백제거

        //utf8 to string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wstr = converter.from_bytes(buffer);
        data.text = converter.to_bytes(wstr);

        //printf("len: %d\n", len);
        //printf("data: %s\n", buffer);
        //printf("text: %s\n", data.text.c_str());
        float score_tmp;
        //id 읽기
        fin >> score_tmp;
        data.score = score_tmp + 32000;
        /*
        if(num > 31000) {
            printf("text: %s\n", data.text.c_str());
            printf("score: %f\n", data.score);
        }
            */

        int tmp;
        fin >> tmp;
        data.attr = (enum llama_token_attr)tmp;

        vocab->id_to_token.push_back(data);
        vocab->token_to_id.push(data.text, vocab->id_to_token.size() - 1);
        //printf("%d text: %s\n", vocab->id_to_token.size() - 1, vocab->id_to_token[vocab->id_to_token.size() - 1]);
    }

    fin.close();
}

#ifdef LLAMA3
void load_llama3_vocab(const char *path, struct llama_vocab *vocab) {
    llama3_load_specials(path, vocab);
    llama3_load_id_to_token(path, vocab);
    llama3_load_bpe_ranks(path, vocab);
}
#endif
void load_vocab(const char *path, struct llama_vocab *vocab) {
    load_specials(path, vocab);
    load_id_to_token(path, vocab);
}

int main(int raw_argc, char* raw_argv[]) {
    //raw_argc = 5;
    //char* raw_argv[] = {" ", "-m", "Llama-2-7B-Chat-GGUF/llama-2-7b-chat.Q8_0.gguf", "-f", "prompts/test.txt"};
    const std::vector<std::string> argv = ingest_args(raw_argc, raw_argv);
    //const std::vector<std::string> argv = {"", "-f", "./prompt.txt"};
    const int argc = argv.size();

    if (argc <= 1) {
        print_usage_information(argv[0].c_str(), stderr);
        return 1;
    }

    //////
    // Read out all the command line arguments.
    //////

    // variables where to put any arguments we see.
    bool printing_ids = false;
    //bool no_bos = false;
    //bool disable_logging = false;
    bool show_token_count = false;
    const char * model_path = NULL;
    const char * prompt_path = NULL;
    const char * prompt_file = NULL;
    const char * prompt_arg = NULL;
    const char * option_path = NULL;

    // track which arguments were explicitly given
    // used for sanity checking down the line
    bool model_path_set = false;
    bool prompt_path_set = false;
    bool prompt_set = false;
    bool stdin_set = false;

    int iarg = 1;
    for (; iarg < argc; ++iarg) {
        std::string arg{argv[iarg]};
        if (arg == "-h" || arg == "--help") {
            print_usage_information(argv[0].c_str(), stdout);
            return 0;
        }
        else if (arg == "--ids") {
            printing_ids = true;
        }
        else if (arg == "-m" || arg == "--model") {
            if (model_path_set) {
                fprintf(stderr, "Error: -m or --model specified multiple times.\n");
                return 1;
            }
            model_path = argv[++iarg].c_str();
            model_path_set = true;
        }
        else if (arg == "--no-bos") {
            //no_bos = true;
        }
        else if (arg == "-p" || arg == "--path") {
            if (prompt_set) {
                fprintf(stderr, "Error: -p or --path specified multiple times.\n");
                return 1;
            }
            option_path = argv[++iarg].c_str();
            prompt_set = true;
        }
        else if (arg == "-f" || arg == "--file") {
            if (prompt_path_set) {
                fprintf(stderr, "Error: -f or --file specified multiple times.\n");
                return 1;
            }
            prompt_file = argv[++iarg].c_str();
        }
        else if (arg == "--stdin") {
            stdin_set = true;
        }
        else if (arg == "--log-disable") {
            //disable_logging = true;
        }
        else if (arg == "--show-count") {
            show_token_count = true;
        }
        else {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[iarg].c_str());
            return 1;
        }
    }
    std::string path = ((std::string(option_path) + std::string("/")) + std::string(prompt_file));
    prompt_path = path.c_str();
    prompt_path_set = true;

    //////
    // Sanity check the command line arguments.
    //////

    if (prompt_path_set && prompt_path == NULL) {
        fprintf(stderr, "Error: --file requires an argument.\n");
        return 1;
    }
    if (!prompt_set) {
        fprintf(stderr, "Error: --prompt requires an argument.\n");
        return 1;
    }

    std::string prompt;
    if (prompt_path_set) {
        bool success = false;
        prompt = read_prompt_from_file(prompt_path, success);
        if (!success) {
            return 1;
        }
    } else if (prompt_set) {
        prompt = prompt_arg;
    } else {
        //GGML_ASSERT(stdin_set);
        // we read stdin *after* loading model (early exit if model cannot
        // be loaded, which can be a nicer user experience)
    }


    struct llama_vocab vocab;
#ifdef LLAMA3
    load_llama3_vocab(option_path, &vocab);
#else
    load_vocab(option_path, &vocab);
#endif


    //vocab.token_to_id.print();

    // read entire prompt from stdin?

    //const bool model_wants_add_bos = llama_should_add_bos_token(model);
    //const bool add_bos = model_wants_add_bos && !no_bos;
    const bool add_bos = true;

    //printf("add_bos: %d\n", add_bos);

    //clock_t token_start = clock();


    std::vector<llama_token> tokens;
#ifdef LLAMA3
    tokens = llama3_tokenize_internal(vocab, prompt, add_bos, true);
#else
    tokens = llama_tokenize_internal(vocab, prompt, add_bos, true);
#endif


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
