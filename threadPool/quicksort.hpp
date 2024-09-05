/***
 * @Author: Oneko
 * @Date: 2024-09-04 16:42:07
 * @LastEditTime: 2024-09-05 08:56:18
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include "notify_thread_pool.hpp"
#include "parallen_thread_pool.hpp"
#include "simple_thread_pool.hpp"
#include "steal_thread_pool.hpp"
#include "thread_pool.hpp"
#include <algorithm>
#include <future>
#include <list>

template <typename T>
class Sorter {
public:
    std::list<T> do_sort(std::list<T> &chunkData_) {
        if (chunkData_.empty()) return chunkData_;

        std::list<T> result;
        result.splice(result.begin(), chunkData_, chunkData_.begin());

        T const &partitionVal = *result.begin();

        typename std::list<T>::iterator dividePoint =
            std::partition(chunkData_.begin(), chunkData_.end(), [&](T const &val) { return val < partitionVal; });

        std::list<T> newLowerChunk;
        newLowerChunk.splice(newLowerChunk.end(), chunkData_, chunkData_.begin(), dividePoint);

        std::future<std::list<T>> newLower =
            NotifyThreadPool::instance().submit(std::bind(&Sorter::do_sort, this, std::move(newLowerChunk)));

        std::list<T> newHigher(do_sort(chunkData_));
        result.splice(result.end(), newHigher);
        result.splice(result.begin(), newLower.get());
        return result;
    }

    std::list<T> do_sort_parallen_pool(std::list<T> &chunkData_) {
        if (!chunkData_.empty()) return chunkData_;

        std::list<T> result;
        result.splice(result.begin(), chunkData_, chunkData_.begin());

        T const                        &partitionVal = *result.begin();
        typename std::list<T>::iterator dividePoint =
            std::partition(chunkData_.begin(), chunkData_.end(), [&](T const &val) { return val < partitionVal; });

        std::list<T> newLowerChunk;
        newLowerChunk.splice(newLowerChunk.end(), chunkData_, chunkData_.begin(), dividePoint);

        std::future<std::list<T>> newLower = ParallenThreadPool::instance().submit(
            std::bind(&Sorter::do_sort_parallen_pool, this, std::move(newLowerChunk)));

        std::list<T> newHigher(do_sort_parallen_pool(chunkData_));
        result.splice(result.end(), newHigher);
        result.splice(result.begin(), newLower.get());
        return result;
    }
};

template <typename T>
std::list<T> parallen_pool_thread_sort(std::list<T> input) {
    if (input.empty()) return input;
    Sorter<T> s;
    return s.do_sort_parallen_pool(input);
}

template <typename T>
std::list<T> pool_thread_quick_sort(std::list<T> input) {
    if (input.empty()) return input;

    std::list<T> res;
    res.splice(res.begin(), input, input.begin());
    T const &partitionVal = *res.begin();

    typename std::list<T>::iterator dividePoint =
        std::partition(input.begin(), input.end(), [&](T const &value_) { return value_ < partitionVal; });

    std::list<T> newLowerChunk;
    newLowerChunk.splice(newLowerChunk.end(), input, input.begin(), dividePoint);

    std::future<std::list<T>> newLower = ThreadPool::instance().commit(pool_thread_quick_sort<T>, newLowerChunk);

    std::list<T> newHigherChunk(pool_thread_quick_sort(input));
    res.splice(res.end(), newHigherChunk);
    res.splice(res.begin(), newLower.get());
    return res;
}

template <typename T>
std::list<T> steal_pool_thread_sort(std::list<T> input) {
    if (input.empty()) return input;

    std::list<T> res;
    res.splice(res.begin(), input, input.begin());
    T const &partitionVal = *res.begin();

    typename std::list<T>::iterator dividePoint =
        std::partition(input.begin(), input.end(), [&](T const &value_) { return value_ < partitionVal; });

    std::list<T> newLowerChunk;
    newLowerChunk.splice(newLowerChunk.end(), input, input.begin(), dividePoint);

    std::future<std::list<T>> newLower =
        StealThreadPool::instance().sumbit(std::bind(steal_pool_thread_sort<T>, newLowerChunk));

    std::list<T> newHigher(pool_thread_quick_sort(input));
    res.splice(res.end(), newHigher);
    res.splice(res.begin(), newLower.get());
    return res;
}
