#include "vector_mymap_template.h"


template class vector_mymap<std::string, int>;
template class vector_mymap<std::pair<std::string, std::string>, int>;

template<typename K, typename V>
vector_mymap<K, V>::vector_mymap() {
    if constexpr (std::is_same_v<K, std::string>) {
        bucket[BUCKET_SIZE].emplace_back(std::make_pair("", 0)); //end data
    }
    else {
        bucket[BUCKET_SIZE].emplace_back(std::make_pair(std::make_pair("",""), 0)); //end data
    }
}


template<typename K, typename V>
uint32_t vector_mymap<K,V>::pow(uint32_t p, int32_t n) const {
    uint32_t ret = 1;

    while(n--) {
        ret = (ret * p) % BUCKET_SIZE;
    }

    return ret;
}

template<typename K, typename V>
uint32_t vector_mymap<K,V>::scalar_hash_function(const char* str, int n) const {
    uint32_t ret = 0;
    uint32_t p = 53;

    for(int i = 0; i < n; i++) {
        ret = (ret + (uint32_t)str[i] * pow(p, i)) % BUCKET_SIZE;
        //std::cout << str[i] << ' ' << pow(p, i) << ' ' << ret << std::endl;
    }
    if(ret >= BUCKET_SIZE) {
        printf("error\n");
    }
    return ret;
}

//candidate
template<typename K, typename V>
uint32_t vector_mymap<K, V>::hash_function(const char* str, int len) const {
    uint32_t ret;

    const char* ptr1 = str;
    const uint32_t* ptr2 = pow_table;

    asm volatile(
        "li t5, 1 \n"
        "vsetvli t0, %4, e32, m1, tu, mu\n\t" //set vl
        "vmv.v.i v3, 0 \n\t" //intialize vector register
        "vmv.v.i v4, 0 \n\t" //intialize vector register
        "0: \t"
        "vsetvli t0, %0, e32, m1, tu, mu\n\t" //set vl
        "vlc8to32.v v1, (%1), t5 \n\t" //load string
        "vle32.v v2, (%2) \n\t" //load pow_table
        "vmacc.vv v3, v1, v2 \n\t" //multiply & accumulate
        "sub %0, %0, t0 \n\t" //loop cnt
        "add %1, %1, t0 \n\t"
        "slli t0, t0, 2 \n\t"
        "add %2, %2, t0 \n\t"
        "bnez %0, 0b \n\t" //if len != 0, again
        "vsetvli t0, %4, e32, m1, tu, mu\n\t" //set vl=8
        "vredsum.vs v5, v3, v4 \n\t" //reduction sum
        "vmv.x.s %3, v5 \n\t"
        : "+r"(len), "+r"(ptr1), "+r"(ptr2), "=r"(ret)
        : "r"(VECTOR_SIZE)
        : "t0", "v0", "v1", "v2", "v3", "v4", "v5"
    );

    ret %= BUCKET_SIZE;

    if(ret >= BUCKET_SIZE) {
        printf("error\n");
    }
    return ret;
}


template<typename K, typename V>
void vector_mymap<K, V>::push(K key, V value) {
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

            asm volatile(
                "mv a0, %1 \n\t"
                "mv a1, %2 \n\t"
                "jal ra, vstrcmp \n\t"
                "mv %0, a0 \n\t"
                : "=r"(strcmp_ret)
                : "r"(key.c_str()), "r"(bucket_entry[i].first.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                chk = true;
                break;
            }
        }
        else {

            asm volatile(
                "mv a0, %1 \n"
                "mv a1, %2 \n"
                "jal ra, vstrcmp \n"
                "bne a0, zero, 0f \n"
                "mv a0, %3 \n"
                "mv a1, %4 \n"
                "jal ra, vstrcmp \n"
                "0: \n"
                "mv %0, a0 \n"
                : "=r"(strcmp_ret)
                : "r"(key.first.c_str()), "r"(bucket_entry[i].first.first.c_str()), "r"(key.second.c_str()), "r"(bucket_entry[i].first.second.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                chk = true;
                break;
            }
            /*
            if(!vstrcmp(key.first.c_str(), bucket_entry[i].first.first.c_str()) && !vstrcmp(key.second.c_str(), bucket_entry[i].first.second.c_str())) {
                chk = true;
                break;
            }
                */
        }
    }

    if(!chk) bucket_entry.push_back(std::make_pair(key, value));
}

template<typename K, typename V>
typename vector_mymap<K, V>::iterator vector_mymap<K, V>::end() const{
    return iterator(bucket[BUCKET_SIZE].begin());
}

template<typename K, typename V>
typename vector_mymap<K, V>::iterator vector_mymap<K, V>::find(K key) const{

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

            asm volatile(
                "mv a0, %1 \n\t"
                "mv a1, %2 \n\t"
                "jal ra, vstrcmp \n\t"
                "mv %0, a0 \n\t"
                : "=r"(strcmp_ret)
                : "r"(key.c_str()), "r"(bucket_entry[i].first.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                return iterator(bucket_entry.begin() + i);
            }
            /*
            if(!vstrcmp(key.c_str(), bucket_entry[i].first.c_str())) {
                return iterator(bucket_entry.begin() + i);
            }
                */
        }
        else {

            asm volatile(
                "mv a0, %1 \n"
                "mv a1, %2 \n"
                "jal ra, vstrcmp \n"
                "bne a0, zero, 0f \n"
                "mv a0, %3 \n"
                "mv a1, %4 \n"
                "jal ra, vstrcmp \n"
                "0: \n"
                "mv %0, a0 \n"
                : "=r"(strcmp_ret)
                : "r"(key.first.c_str()), "r"(bucket_entry[i].first.first.c_str()), "r"(key.second.c_str()), "r"(bucket_entry[i].first.second.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                return iterator(bucket_entry.begin() + i);
            }
            /*
            if(!vstrcmp(key.first.c_str(), bucket_entry[i].first.first.c_str()) && !vstrcmp(key.second.c_str(), bucket_entry[i].first.second.c_str())) {
                return iterator(bucket_entry.begin() + i);
            }
                */
        }
    }

    return iterator(bucket[BUCKET_SIZE].begin());
}

template<typename K, typename V>
llama_token vector_mymap<K, V>::at(K key) const{
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

            asm volatile(
                "mv a0, %1 \n\t"
                "mv a1, %2 \n\t"
                "jal ra, vstrcmp \n\t"
                "mv %0, a0 \n\t"
                : "=r"(strcmp_ret)
                : "r"(key.c_str()), "r"(bucket_entry[i].first.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                ret = bucket_entry[i].second;
                break;
            }
            /*
            if(!vstrcmp(key.c_str(), bucket_entry[i].first.c_str())) {
                ret = bucket_entry[i].second;
                break;
            }
                */
        }
        else {

            asm volatile(
                "mv a0, %1 \n"
                "mv a1, %2 \n"
                "jal ra, vstrcmp \n"
                "bne a0, zero, 0f \n"
                "mv a0, %3 \n"
                "mv a1, %4 \n"
                "jal ra, vstrcmp \n"
                "0: \n"
                "mv %0, a0 \n"
                : "=r"(strcmp_ret)
                : "r"(key.first.c_str()), "r"(bucket_entry[i].first.first.c_str()), "r"(key.second.c_str()), "r"(bucket_entry[i].first.second.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                ret = bucket_entry[i].second;
                break;
            }
            /*
            if(!vstrcmp(key.first.c_str(), bucket_entry[i].first.first.c_str()) && !vstrcmp(key.second.c_str(), bucket_entry[i].first.second.c_str())) {
                ret = bucket_entry[i].second;
                break;
            }
                */
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
int& vector_mymap<K, V>::operator[] (K key) {
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

            asm volatile(
                "mv a0, %1 \n\t"
                "mv a1, %2 \n\t"
                "jal ra, vstrcmp \n\t"
                "mv %0, a0 \n\t"
                : "=r"(strcmp_ret)
                : "r"(key.c_str()), "r"(bucket_entry[i].first.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                return bucket_entry[i].second;
            }
            /*
            if(!vstrcmp(key.c_str(), bucket_entry[i].first.c_str())) {
                return bucket_entry[i].second;
                break;
            }
                */
        }
        else {

            asm volatile(
                "mv a0, %1 \n"
                "mv a1, %2 \n"
                "jal ra, vstrcmp \n"
                "bne a0, zero, 0f \n"
                "mv a0, %3 \n"
                "mv a1, %4 \n"
                "jal ra, vstrcmp \n"
                "0: \n"
                "mv %0, a0 \n"
                : "=r"(strcmp_ret)
                : "r"(key.first.c_str()), "r"(bucket_entry[i].first.first.c_str()), "r"(key.second.c_str()), "r"(bucket_entry[i].first.second.c_str())
                : "a0", "a1"
            );
            if(!strcmp_ret) {
                return bucket_entry[i].second;
            }
            /*
            if(!vstrcmp(key.first.c_str(), bucket_entry[i].first.first.c_str()) && !vstrcmp(key.second.c_str(), bucket_entry[i].first.second.c_str())) {
                return bucket_entry[i].second;
                break;
            }
                */
        }
    }

    std::cout << "My: Out of bound exception\n";
    exit(1);
}


template<typename K, typename V>
const std::pair<K, V>& vector_mymap<K, V>::iterator::operator*() {
    return *current;
}

template<typename K, typename V>
const std::pair<K, V>& vector_mymap<K, V>::iterator::operator*() const {
    return *current;
}


template<typename K, typename V>
bool vector_mymap<K, V>::iterator::operator !=(const iterator& other) {
    return current != other.current;
}

template<typename K, typename V>
bool vector_mymap<K, V>::iterator::operator ==(const iterator& other) {
    return current == other.current;
}

template<typename K, typename V>
bool vector_mymap<K, V>::iterator::operator ==(const iterator& other) const {
    return current == other.current;
}


template<typename K, typename V>
void vector_mymap<K, V>::print() {
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

template<typename K, typename V>
bool vector_mymap<K, V>::vstrcmp(const char* str1, const char* str2) const {
    //printf("vstrcmp_start\n");
    bool ret;
    asm volatile(
        "li t2, 1 \n\t"
        "vsetvli t0, %3, e8, m1, tu, mu \n\t" //set vector configuration
        "0: \n\t"
        //"vstrcmp_loop: \n\t"
        "vle8.v v1, (%1) \n\t" //load str1
        "vle8.v v2, (%2) \n\t" //load str2
        "add %1, %1, t0 \n\t" // move mem read point(str1)
        "add %2, %2, t0 \n\t" // move mem read point(str2)
        "vstrcmp.vv v3, v1, v2 \n\t" //vector string compare
        "vmv.x.s %0, v3 \n\t"
        "beq %0, zero, 1f \n\t" //끝까지 같으면 종료
        //"beq t1, zero, exit \n\t" //끝까지 같으면 종료
        "beq %0, t2, 0b \n\t" //아직 끝까지 안읽었으면 더 읽기
        //"beq t1, t2, vstrcmp_loop \n\t" //아직 끝까지 안읽었으면 더 읽기
        //"exit: \n\t"
        "1: \n\t"
        : "+r"(ret), "+r"(str1), "+r"(str2)
        : "r"(VECTOR_SIZE)
        : "v1", "v2", "t0", "t1", "t2"
    );

    return ret;

}
