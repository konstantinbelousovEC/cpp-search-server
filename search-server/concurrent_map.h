#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");
    
    struct MutexAndBucket {
        std::mutex m;
        std::map<Key, Value> bucket = std::map<Key, Value>();
    };
    
    struct Access {
        explicit Access(std::vector<MutexAndBucket>& buckets, const Key& key)
        : lock(buckets[key % buckets.size()].m),
          ref_to_value(buckets[key % buckets.size()].bucket[key]) {
              
        }
        std::lock_guard<std::mutex> lock;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
    : bucket_count_(bucket_count), buckets_(std::vector<MutexAndBucket>(bucket_count)) {
        
    };
    
    Access operator[](const Key& key) {
        return Access(buckets_, key);
    };
    
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> ordinary_map;
        for (MutexAndBucket& element : buckets_) {
            std::lock_guard<std::mutex> lock(element.m);
            ordinary_map.merge(element.bucket);
        }
        return ordinary_map;
    };

private:
    const size_t bucket_count_;
    std::vector<MutexAndBucket> buckets_;
};
