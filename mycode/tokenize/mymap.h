#include <vector>
#include <cstdint>
#include <iostream>
#include <cmath>
#include <cstring>

#define BUCKET_SIZE 20001

typedef int32_t llama_token;

struct mymap {
public:
    struct iterator {
    private:
    public:
        std::vector<std::pair<std::string, llama_token>>::const_iterator current;

        iterator(std::vector<std::pair<std::string, llama_token>>::const_iterator current) : current(current) {}

        //iterator& operator[](std::string str);
        const std::pair<std::string, llama_token>& operator*();
        const std::pair<std::string, llama_token>& operator*() const;
        bool operator !=(const iterator& other);
        bool operator ==(const iterator& other);
        bool operator ==(const iterator& other) const;
    };


    mymap();
    void push(std::string input, llama_token id);
    uint32_t hash_function(const char* str, int n) const;
    mymap::iterator end() const;
    mymap::iterator find(const char* str) const;
    llama_token at(std::string input) const;
    llama_token at(const char* str) const;
    int& operator[] (std::string str);
    void print();
    //std::pair<std::string, llama_token>& operator*();

    int cnt[9] = {0,};
    int not_first = 0;

    std::vector<std::pair<std::string, llama_token>> bucket[BUCKET_SIZE + 1];

private:

};
