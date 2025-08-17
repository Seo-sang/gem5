#include "mymap.h"

mymap::mymap() {
    bucket[BUCKET_SIZE].emplace_back(std::make_pair("", 0)); //end data
}

//candidate
uint32_t mymap::hash_function(const char* str, int n) const {
    uint32_t ret = 0;
    uint32_t p = 53;

    for(int i = 0; i < n; i++) {
        ret = (ret + (uint32_t)str[i] * (uint32_t)pow(p, i + 1)) % BUCKET_SIZE;
    }
    return ret;
}

void mymap::push(std::string input, llama_token id) {
    uint32_t hash_value = hash_function(input.c_str(), input.size());
    auto &bucket_entry = bucket[hash_value];

    bool chk = false;
    for(int i = 0; i < bucket_entry.size(); i++) {
        //string compare candidate
        if(bucket_entry[i].first == input) {
            chk = true;
            break;
        }
    }

    if(!chk) bucket_entry.emplace_back(make_pair(input, id));
}

mymap::iterator mymap::end() const{
    return iterator(bucket[BUCKET_SIZE].begin());
}

mymap::iterator mymap::find(const char* str) const{

    //int len = strlen(str);
    uint32_t hash_value = hash_function(str, strlen(str));
    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        //if(len != bucket_entry[i].first.size()) continue;

        //candidate
        if(!strcmp(str, bucket_entry[i].first.c_str())) {
            return iterator(bucket_entry.begin() + i);
        }
    }

    return iterator(bucket[BUCKET_SIZE].begin());
}

llama_token mymap::at(std::string input) const{
    llama_token ret = -1;

    uint32_t hash_value = hash_function(input.c_str(), input.size());
    auto &bucket_entry = bucket[hash_value];
    //int len = input.size();

    for(int i = 0; i < bucket_entry.size(); i++) {
//        if(len != bucket_entry[i].first.size()) continue;
        //string compare
        if(input == bucket_entry[i].first) {
            ret = bucket_entry[i].second;
            break;
        }
    }

    return ret;
}

llama_token mymap::at(const char* str) const{
    llama_token ret = -1;
    //int len = strlen(str);

    uint32_t hash_value = hash_function(str, strlen(str));
    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        //if(len != bucket_entry[i].first.size()) continue;

        //string compare
        if(!strcmp(str, bucket_entry[i].first.c_str())) {
            ret = bucket_entry[i].second;
            break;
        }
    }

    return ret;
}


int& mymap::operator[] (std::string input) {
    //int len = input.size();
    uint32_t hash_value = hash_function(input.c_str(), input.size());
    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        //if(len != bucket_entry[i].first.size()) continue;

        if(input == bucket_entry[i].first) {
            return bucket_entry[i].second;
        }
    }

    std::cout << "My: Out of bound exception\n";
    exit(1);
}


const std::pair<std::string, llama_token>& mymap::iterator::operator*() {
    return *current;
}

const std::pair<std::string, llama_token>& mymap::iterator::operator*() const {
    return *current;
}

bool mymap::iterator::operator !=(const iterator& other) {
    return current != other.current;
}

bool mymap::iterator::operator ==(const iterator& other) {
    return current == other.current;
}

bool mymap::iterator::operator ==(const iterator& other) const {
    return current == other.current;
}

void mymap::print() {
    std::cout << "print\n";
    for(int i = 0; i < BUCKET_SIZE; i++) {
        if(bucket[i].size() == 0) continue;
        for(auto it = bucket[i].begin(); it != bucket[i].end(); it++) {
            std::cout << it->first << ' ';
        }
        std::cout << std::endl;
    }
}
