#include "mymap_template.h"


template class mymap<std::string, int>;
template class mymap<std::pair<std::string, std::string>, int>;

template<typename K, typename V>
mymap<K, V>::mymap() {
    if constexpr (std::is_same_v<K, std::string>) {
        bucket[BUCKET_SIZE].emplace_back(std::make_pair("", 0)); //end data
    }
    else {
        bucket[BUCKET_SIZE].emplace_back(std::make_pair(std::make_pair("",""), 0)); //end data
    }
}


//candidate
template<typename K, typename V>
uint32_t mymap<K, V>::hash_function(const char* str, int n) const {
    uint32_t ret = 0;
    uint32_t p = 53;

    for(int i = 0; i < n; i++) {
        ret = (ret + (uint32_t)str[i] * (uint32_t)pow(p, i + 1)) % BUCKET_SIZE;
    }
    if(ret >= BUCKET_SIZE) {
        printf("error\n");
    }
    return ret;
}


template<typename K, typename V>
void mymap<K, V>::push(K key, V value) {
    int32_t strcmp_ret;
    uint32_t hash_value;
    if constexpr(std::is_same_v<K, std::string>) {
        hash_value = hash_function(key.c_str(), key.size());
    }
    else {
        uint32_t str1_hash = hash_function(key.first.c_str(), key.first.size());
        uint32_t str2_hash = hash_function(key.second.c_str(), key.second.size());
        hash_value = (str1_hash + 0x9e3779b9 + str2_hash) % BUCKET_SIZE;
    }
    auto &bucket_entry = bucket[hash_value];

    bool chk = false;
    for(int i = 0; i < bucket_entry.size(); i++) {
        if constexpr(std::is_same_v<K, std::string>) {
            if(key == bucket_entry[i].first) {
                chk = true;
                break;
            }
        }
        else {
            if(key.first == bucket_entry[i].first.first && key.second == bucket_entry[i].first.second) {
                chk = true;
                break;
            }
        }
    }

    if(!chk) bucket_entry.push_back(std::make_pair(key, value));
}

template<typename K, typename V>
typename mymap<K, V>::iterator mymap<K, V>::end() const{
    return iterator(bucket[BUCKET_SIZE].begin());
}

template<typename K, typename V>
typename mymap<K, V>::iterator mymap<K, V>::find(K key) const{

    int32_t strcmp_ret;
    uint32_t hash_value;

    if constexpr(std::is_same_v<K, std::string>) {
        hash_value = hash_function(key.c_str(), key.size());
    }
    else {
        uint32_t str1_hash = hash_function(key.first.c_str(), key.first.size());
        uint32_t str2_hash = hash_function(key.second.c_str(), key.second.size());
        hash_value = (str1_hash + 0x9e3779b9 + str2_hash) % BUCKET_SIZE;
    }

    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {

        if constexpr(std::is_same_v<K, std::string>) {

            if(key == bucket_entry[i].first) {
                return iterator(bucket_entry.begin() + i);
            }
        }
        else {
            if(key.first == bucket_entry[i].first.first && key.second == bucket_entry[i].first.second) {
                return iterator(bucket_entry.begin() + i);
            }

        }
    }

    return iterator(bucket[BUCKET_SIZE].begin());
}

template<typename K, typename V>
llama_token mymap<K, V>::at(K key) const{
    llama_token ret = -1;

    int32_t strcmp_ret;

    uint32_t hash_value;
    if constexpr(std::is_same_v<K, std::string>) {
        hash_value = hash_function(key.c_str(), key.size());
    }
    else {
        uint32_t str1_hash = hash_function(key.first.c_str(), key.first.size());
        uint32_t str2_hash = hash_function(key.second.c_str(), key.second.size());
        hash_value = (str1_hash + 0x9e3779b9 + str2_hash) % BUCKET_SIZE;
    }

    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        //string compare


        if constexpr(std::is_same_v<K, std::string>) {

            if(key == bucket_entry[i].first) {
                ret = bucket_entry[i].second;
                break;
            }
        }
        else {
            if(key.first == bucket_entry[i].first.first && key.second == bucket_entry[i].first.second) {
                ret = bucket_entry[i].second;
                break;
            }

        }
    }

    return ret;
}
/*
llama_token vector_mymap::at(K key) const{
    llama_token ret = -1;


    uint32_t hash_value;
    if constexpr(std::is_same_v<K, std::string>) {
        hash_value = hash_function(key.c_str(), key.size());
    }
    else {
        uint32_t str1_hash = hash_function(key.first.c_str(), key.first.size());
        uint32_t str2_hash = hash_function(key.second.c_str(), key.second.size());
        hash_value = str1_hash + 0x9e3779b9 + str2_hash;
    }

    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        //string compare

        if(vstrcmp(str, bucket_entry[i].first.c_str())) {
            ret = bucket_entry[i].second;
            break;
        }
    }

    return ret;
}
*/


template<typename K, typename V>
int& mymap<K, V>::operator[] (K key) {
    int32_t strcmp_ret;
    uint32_t hash_value;
    if constexpr(std::is_same_v<K, std::string>) {
        hash_value = hash_function(key.c_str(), key.size());
    }
    else {
        uint32_t str1_hash = hash_function(key.first.c_str(), key.first.size());
        uint32_t str2_hash = hash_function(key.second.c_str(), key.second.size());
        hash_value = (str1_hash + 0x9e3779b9 + str2_hash) % BUCKET_SIZE;
    }
    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {


        if constexpr(std::is_same_v<K, std::string>) {

            if(key == bucket_entry[i].first) {
                return bucket_entry[i].second;
            }
        }
        else {

            if(key.first == bucket_entry[i].first.first && key.second == bucket_entry[i].first.second) {
                return bucket_entry[i].second;
            }
        }
    }

    std::cout << "My: Out of bound exception\n";
    exit(1);
}


template<typename K, typename V>
const std::pair<K, V>& mymap<K, V>::iterator::operator*() {
    return *current;
}

template<typename K, typename V>
const std::pair<K, V>& mymap<K, V>::iterator::operator*() const {
    return *current;
}


template<typename K, typename V>
bool mymap<K, V>::iterator::operator !=(const iterator& other) {
    return current != other.current;
}

template<typename K, typename V>
bool mymap<K, V>::iterator::operator ==(const iterator& other) {
    return current == other.current;
}

template<typename K, typename V>
bool mymap<K, V>::iterator::operator ==(const iterator& other) const {
    return current == other.current;
}


template<typename K, typename V>
void mymap<K, V>::print() {
    /*
    std::cout << "print\n";
    for(int i = 0; i < BUCKET_SIZE; i++) {
        if(bucket[i].size() == 0) continue;
        for(auto it = bucket[i].begin(); it != bucket[i].end(); it++) {
            std::cout << it->first << ' ';
        }
        std::cout << std::endl;
    }
*/
}
