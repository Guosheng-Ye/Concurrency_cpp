#ifndef __THREAD_SAFE_LOOKUP_TABLE__
#define __THREAD_SAFE_LOOKUP_TABLE__

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ThreadSafeLookUpTable {
public:
    ThreadSafeLookUpTable(unsigned    num_buckets_ = 23,
                          const Hash &hasher_      = Hash())
        : _vecBuckets(num_buckets_)
        , _hasher(hasher_) {

        for (unsigned i = 0; i < num_buckets_; ++i) {
            _vecBuckets[i].reset(new Bucket_type);
        }
    }

    ThreadSafeLookUpTable(const ThreadSafeLookUpTable &)            = delete;
    ThreadSafeLookUpTable &operator=(const ThreadSafeLookUpTable &) = delete;

    /// @brief get value by key
    /// @param key
    /// @param default_value
    /// @return
    Value value_for(const Key &key, Value const &default_value = Value()) {
        return get_bucket(key).value_for(key, default_value);
    }

    /// @brief add or update value
    /// @param key
    /// @param value
    void add_or_update(const Key &key, const Value &value) {
        get_bucket(key).add_or_update(key, value);
    }

    /// @brief remove value if exist
    /// @param key
    bool remove_mapping(const Key &key) {
        return get_bucket(key).delete_elem(key);
    }

    std::map<Key, Value> get_map() {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (unsigned i = 0; i < _vecBuckets.size(); ++i) {
            locks.push_back(
                std::unique_lock<std::shared_mutex>(_vecBuckets[i]->_smtx));
        }
        std::map<Key, Value> res;
        for (unsigned i = 0; i < _vecBuckets.size(); ++i) {
            // 需用typename告诉编译器bucket_type::bucket_iterator是一个类型，以后再实例化
            // 当然此处可简写成auto it = buckets[i]->data.begin();
            typename Bucket_type::bucket_iterator it =
                _vecBuckets[i]->_data.begin();
            for (; it != _vecBuckets[i]->_data.end(); ++it) {
                res.insert(*it);
            }
        }
        return res;
    }

private:
    class Bucket_type {

        friend class ThreadSafeLookUpTable;

    private:
        using bucket_value    = std::pair<Key, Value>;   // 存储元素类型
        using bucket_lst_data = std::list<bucket_value>; // 链表
        using bucket_iterator = typename bucket_lst_data::iterator; // 迭代器
        bucket_lst_data           _data; // 链表构成的数据内容
        mutable std::shared_mutex _smtx;

    private:
        bucket_iterator find_entry_for(const Key &key) {
            return std::find_if(
                _data.begin(), _data.end(),
                [&](bucket_value const &item) { return item.first == key; });
        }

    public:
        /// @brief //查找key值，找到返回对应的value，未找到则返回默认值
        /// @return Value
        Value value_for(const Key &key, const Value &default_value) {
            std::shared_lock<std::shared_mutex> shard_lock(_smtx);
            bucket_iterator const               find_iter = find_entry_for(key);
            return find_iter == _data.end() ? default_value : find_iter->second;
        }

        /// @brief 添加key和value，找到则更新，没找到则添加
        /// @param key
        /// @param value
        void add_or_update(const Key &key, const Value &value) {
            std::unique_lock<std::shared_mutex> unique_lock(_smtx);
            bucket_iterator const               find_iter = find_entry_for(key);
            if (find_iter == _data.end()) { // add
                _data.push_back(bucket_value(key, value));
            } else {
                find_iter->second = value; // update value
            }
        }

        /// @brief 删除对应的key
        /// @param key
        /// @return bool
        bool delete_elem(const Key &key) {
            std::unique_lock<std::shared_mutex> unique_lock(_smtx);
            bucket_iterator const               find_iter = find_entry_for(key);
            if (find_iter != _data.end()) {
                _data.erase(find_iter);
                return true;
            } else {
                return false;
            }
        }
    };

private:
    std::vector<std::unique_ptr<Bucket_type>> _vecBuckets; // 存储bucket
    Hash _hasher; // 用来根据key生成哈希值

    /// @brief
    /// 根据key生成数字，并对buckets的大小取余得到下标，根据下标返回对应的buckets智能指针
    /// @param key
    /// @return
    Bucket_type &get_bucket(const Key &key) const {
        const std::size_t bucket_idx = _hasher(key) % _vecBuckets.size();
        return *(_vecBuckets[bucket_idx]); //* unique_ptr
    }
};

#endif