#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <cmath>

#define VECTOR_SIZE 256
#define BUCKET_SIZE 20001

typedef int32_t llama_token;

class vector_mymap {
public:
    struct iterator {
    private:
        std::vector<std::pair<std::string, llama_token>>::const_iterator current;
    public:

        iterator(std::vector<std::pair<std::string, llama_token>>::const_iterator current) : current(current) {}

        //iterator& operator[](std::string str);
        const std::pair<std::string, llama_token>& operator*();
        const std::pair<std::string, llama_token>& operator*() const;
        bool operator !=(const iterator& other);
        bool operator ==(const iterator& other);
        bool operator ==(const iterator& other) const;
    };


    vector_mymap();
    void push(std::string input, llama_token id);
    //uint32_t pow(uint32_t, int32_t) const;
    uint32_t hash_function(const char* str, int n) const;
    //uint32_t scalar_hash_function(const char* str, int n) const;
    vector_mymap::iterator end() const;
    vector_mymap::iterator find(const char* str) const;
    llama_token at(std::string input) const;
    llama_token at(const char* str) const;
    int& operator[] (std::string str);
    bool vstrcmp(const char*, const char*) const;
    void print();

    int cnt[9] = {0,};

private:
    std::vector<std::pair<std::string, llama_token>> bucket[BUCKET_SIZE + 1];

    const uint32_t pow_table[48] = {1, 53, 2809, 8870, 10087, 14585,
        12967, 7217, 2482, 11540, 11590, 14240,
        14683, 18161, 2485, 11699, 16, 848,
        4942, 1913, 1384, 13349, 7462, 15467,
        19711, 4631, 5431, 7829, 14917, 10562,
        19759, 7175, 256, 13568, 19069, 10607,
        2143, 13574, 19387, 7460, 15361, 14093,
        6892, 5258, 18661, 8984, 16129, 14795};

};
