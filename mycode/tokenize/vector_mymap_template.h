#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>

#define VECTOR_SIZE 256
#define BUCKET_SIZE 20001

typedef int32_t llama_token;
template<typename K, typename V>
class vector_mymap {
public:
    struct iterator {
    private:
        typename std::vector<std::pair<K, V>>::const_iterator current;
    public:

        iterator(typename std::vector<std::pair<K, V>>::const_iterator current) : current(current) {}

        //iterator& operator[](std::string str);
        const std::pair<K, V>& operator*();
        const std::pair<K, V>& operator*() const;
        bool operator !=(const iterator& other);
        bool operator ==(const iterator& other);
        bool operator ==(const iterator& other) const;
    };


    vector_mymap();
    void push(K key, V value);
    uint32_t pow(uint32_t, int32_t) const;
    uint32_t hash_function(const char* str, int len) const;
    uint32_t scalar_hash_function(const char* str, int n) const;
    vector_mymap::iterator end() const;
    vector_mymap::iterator find(K key) const;
    //llama_token at(std::string input) const;
    llama_token at(K) const;
    int& operator[] (K key);
    bool vstrcmp(const char*, const char*) const;
    void print();

    int cnt[9] = {0,};
    int not_first = 0;

private:
    std::vector<std::pair<K, V>> bucket[BUCKET_SIZE + 1];


    const uint32_t pow_table[260] = {1,53,2809,8870,10087,14585,12967,7217,2482,11540,11590,
        14240,14683,18161,2485,11699,16,848,4942,1913,1384,
        13349,7462,15467,19711,4631,5431,7829,14917,10562,19759,
        7175,256,13568,19069,10607,2143,13574,19387,7460,15361,
        14093,6892,5258,18661,8984,16129,14795,4096,17078,5089,
        9704,14287,17174,10177,19355,5764,5477,10267,4124,18562,
        3737,18052,16709,5533,13235,1420,15257,8581,14771,2824,
        9665,12220,7628,4264,5981,16978,19790,8818,7331,8524,
        11750,2719,4100,17290,16325,5182,14633,15511,2042,8221,
        15692,11635,16625,1081,17291,16378,7991,3502,5597,16627,
        1187,2908,14117,8164,12671,11530,11060,6151,5987,17296,
        16643,2035,7850,16030,9548,6019,18992,6526,5861,10618,
        2726,4471,16952,18412,15788,16723,6275,12559,5594,16468,
        12761,16300,3857,4411,13772,9880,3614,11533,11219,14578,
        12596,7555,395,934,9500,3475,4166,787,1709,10573,
        341,18073,17822,4519,19496,13237,1526,874,6320,14944,
        11993,15598,6653,12592,7343,9160,5456,9154,5138,12301,
        11921,11782,4415,13984,1115,19093,11879,9556,6443,1462,
        17483,6553,7292,6457,2204,16807,10727,8503,10637,3733,
        17840,5473,10055,12889,3083,3391,19715,4843,16667,3307,
        15263,8899,11624,16042,10184,19726,5426,7564,872,6214,
        9326,14254,15425,17485,6659,12910,4196,2377,5975,16660,
        2936,15601,6812,1018,13952,19420,9209,8053,6788,19747,
        6539,6550,7133,18031,15596,6547,6974,9604,8987,16288,
        3221,10705,7337,8842,8603,15937,4619,4795,14123,8482,
        9524,4747,11579,13657,3785,595,11534,11272,17387};
};
