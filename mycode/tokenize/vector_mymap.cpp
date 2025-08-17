#include "vector_mymap.h"

vector_mymap::vector_mymap() {
    bucket[BUCKET_SIZE].emplace_back(std::make_pair("", 0)); //end data
}

/*
uint32_t vector_mymap::hash_function(const char* str, int n) const {
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
    */



uint32_t vector_mymap::hash_function(const char* str, int32_t len) const {
    uint32_t ret;
    /*
    static const uint32_t pow_table[48] = {1, 53, 2809, 8870, 10087, 14585,
                            12967, 7217, 2482, 11540, 11590, 14240,
                            14683, 18161, 2485, 11699, 16, 848,
                            4942, 1913, 1384, 13349, 7462, 15467,
                            19711, 4631, 5431, 7829, 14917, 10562,
                            19759, 7175, 256, 13568, 19069, 10607,
                            2143, 13574, 19387, 7460, 15361, 14093,
                            6892, 5258, 18661, 8984, 16129, 14795};
                            */

    const char* ptr1 = str;
    const uint32_t* ptr2 = pow_table;
    asm volatile(
        //"li t5, 1\n\t"
        "vsetivli t0, 8, e32, m1, tu, mu\n\t" //set vl
        "vmv.v.i v3, 0 \n\t" //intialize vector register
        "vmv.v.i v4, 0 \n\t" //intialize vector register
        "hash_loop: \n\t"
        "vsetvli t0, %0, e32, m1, tu, mu\n" //set vl
        "vlc8to32.v v1, (%1) \n" //load string
        "vle32.v v2, (%2) \n\t" //load pow_table
        "vmacc.vv v3, v1, v2 \n\t" //multiply & accumulate
        "sub %0, %0, t0 \n\t" //loop cnt
        "add %1, %1, t0 \n\t"
        "addi %2, %2, 64 \n\t" //VLEN-byte 이동
        //"slli t0, t0, 2 \n\t"
        "bnez %0, hash_loop \n\t" //if len != 0, again
        "vsetivli t0, 8, e32, m1, tu, mu\n\t" //set vl=8
        "vredsum.vs v5, v3, v4 \n\t" //reduction sum
        "vmv.x.s %3, v5 \n\t"
        : "+r"(len), "+r"(ptr1), "+r"(ptr2), "=r"(ret)
        : "r"(VECTOR_SIZE)
        : "t0", "v0", "v1", "v2", "v3", "v4", "v5", "t5"
    );

    ret %= BUCKET_SIZE;

    return ret;
}



void vector_mymap::push(std::string input, llama_token id) {
    int32_t strcmp_ret;
    uint32_t hash_value = hash_function(input.c_str(), input.size());
    auto &bucket_entry = bucket[hash_value];

    bool chk = false;
    for(int i = 0; i < bucket_entry.size(); i++) {

        /*
        if(!strcmp(input.c_str(), bucket_entry[i].first.c_str())) {
            chk = true;
            break;
        }

        //string compare candidate
        if(vstrcmp(input.c_str(), bucket_entry[i].first.c_str())) {
            chk = true;
            break;
        }
            */
        asm volatile(
            "mv a0, %1 \n\t"
            "mv a1, %2 \n\t"
            "jal ra, vstrcmp \n\t"
            "mv %0, a0 \n\t"
            : "=r"(strcmp_ret)
            : "r"(input.c_str()), "r"(bucket_entry[i].first.c_str())
            : "a0", "a1"
        );
        if(!strcmp_ret) {
            chk = true;
            break;
        }

    }

    if(!chk) bucket_entry.emplace_back(make_pair(input, id));
}

vector_mymap::iterator vector_mymap::end() const{
    return iterator(bucket[BUCKET_SIZE].begin());
}

vector_mymap::iterator vector_mymap::find(const char* str) const{

    int32_t strcmp_ret;
    uint32_t hash_value = hash_function(str, strlen(str));

    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {


            /*

        if(!strcmp(str, bucket_entry[i].first.c_str())) {
            return iterator(bucket_entry.begin() + i);
        }

        if(vstrcmp(str, bucket_entry[i].first.c_str())) {
            return iterator(bucket_entry.begin() + i);
        }

        */

        asm volatile(
            "mv a0, %1 \n\t"
            "mv a1, %2 \n\t"
            "jal ra, vstrcmp \n\t"
            "mv %0, a0 \n\t"
            : "=r"(strcmp_ret)
            : "r"(str), "r"(bucket_entry[i].first.c_str())
            : "a0", "a1"
        );
        if(!strcmp_ret) {
            return iterator(bucket_entry.begin() + i);
        }

    }

    return iterator(bucket[BUCKET_SIZE].begin());
}

llama_token vector_mymap::at(std::string input) const{
    llama_token ret = -1;
    int32_t strcmp_ret;

    uint32_t hash_value = hash_function(input.c_str(), input.size());
    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        //string compare

        /*

        if(!strcmp(input.c_str(), bucket_entry[i].first.c_str())) {
            ret = bucket_entry[i].second;
            break;
        }


        if(vstrcmp(input.c_str(), bucket_entry[i].first.c_str())) {
            ret = bucket_entry[i].second;
            break;
        }

        */
        asm volatile(
            "mv a0, %1 \n\t"
            "mv a1, %2 \n\t"
            "jal ra, vstrcmp \n\t"
            "mv %0, a0 \n\t"
            : "=r"(strcmp_ret)
            : "r"(input.c_str()), "r"(bucket_entry[i].first.c_str())
            : "a0", "a1"
        );
        if(!strcmp_ret) {
            ret = bucket_entry[i].second;
            break;
        }

    }

    return ret;
}

llama_token vector_mymap::at(const char* str) const{
    llama_token ret = -1;
    int32_t strcmp_ret;

    uint32_t hash_value = hash_function(str, strlen(str));
    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        //string compare

         /*
        if(!strcmp(str, bucket_entry[i].first.c_str())) {
            ret = bucket_entry[i].second;
            break;
        }

        if(vstrcmp(str, bucket_entry[i].first.c_str())) {
            ret = bucket_entry[i].second;
            break;
        }
            */
        asm volatile(
            "mv a0, %1 \n\t"
            "mv a1, %2 \n\t"
            "jal ra, vstrcmp \n\t"
            "mv %0, a0 \n\t"
            : "=r"(strcmp_ret)
            : "r"(str), "r"(bucket_entry[i].first.c_str())
            : "a0", "a1"
        );
        if(!strcmp_ret) {
            ret = bucket_entry[i].second;
            break;
        }
    }

    return ret;
}


int& vector_mymap::operator[] (std::string input) {
    int32_t strcmp_ret;

    uint32_t hash_value = hash_function(input.c_str(), input.size());
    auto &bucket_entry = bucket[hash_value];

    for(int i = 0; i < bucket_entry.size(); i++) {
        /*
        if(!strcmp(input.c_str(), bucket_entry[i].first.c_str())) {
            return bucket_entry[i].second;
        }



        if(vstrcmp(input.c_str(), bucket_entry[i].first.c_str())) {
            return bucket_entry[i].second;
        }


            */

        asm volatile(
            "mv a0, %1 \n \t"
            "mv a1, %2 \n\t"
            "jal ra, vstrcmp \n\t"
            "mv %0, a0 \n\t"
            : "=r"(strcmp_ret)
            : "r"(input.c_str()), "r"(bucket_entry[i].first.c_str())
            : "a0", "a1"
        );
        if(!strcmp_ret) {
            return bucket_entry[i].second;
            break;
        }

    }

    std::cout << "My: Out of bound exception\n";
    exit(1);
}


const std::pair<std::string, llama_token>& vector_mymap::iterator::operator*() {
    return *current;
}

const std::pair<std::string, llama_token>& vector_mymap::iterator::operator*() const {
    return *current;
}


bool vector_mymap::iterator::operator !=(const iterator& other) {
    return current != other.current;
}

bool vector_mymap::iterator::operator ==(const iterator& other) {
    return current == other.current;
}

bool vector_mymap::iterator::operator ==(const iterator& other) const {
    return current == other.current;
}


void vector_mymap::print() {
    std::cout << "print\n";
    for(int i = 0; i < BUCKET_SIZE; i++) {
        if(bucket[i].size() == 0) continue;
        for(auto it = bucket[i].begin(); it != bucket[i].end(); it++) {
            std::cout << it->first << ' ';
        }
        std::cout << std::endl;
    }
}
/*
bool vector_mymap::vstrcmp(const char* str1, const char* str2) const {
    //printf("vstrcmp_start\n");
    bool ret;
    asm volatile(
        "li %0, 1 \n\t"
        //"li t2, 1 \n\t"
        "vsetvli t0, %3, e8, m1, tu, mu \n\t" //set vector configuration
        "vstrcmp_loop: \n\t"
        "vle8.v v1, (%1) \n\t" //load str1
        "vle8.v v2, (%2) \n\t" //load str2
        "vstrcmp.vv v3, v1, v2 \n\t" //vector string compare
        "vmv.x.s t1, v3 \n\t"
        "beq t1, zero, exit \n\t" //끝까지 같으면 종료
        "add %1, %1, t0 \n\t" // move mem read point(str1)
        "add %2, %2, t0 \n\t" // move mem read point(str2)
        "beq t1, %0, vstrcmp_loop \n\t" //아직 끝까지 안읽었으면 더 읽기
        "li %0, 0 \n\t"
        "exit: \n\t"
        : "+r"(ret), "+r"(str1), "+r"(str2)
        : "r"(VECTOR_SIZE)
        : "v1", "v2", "t0", "t1", "t2"
    );

    return ret;
}
*/
