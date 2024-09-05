/*** 
 * @Author: Oneko
 * @Date: 2024-09-04 11:05:21
 * @LastEditTime: 2024-09-05 10:23:26
 * @LastEditors: Ye Guosheng
 * @Description: 
 */
/***
 * @Author: Oneko
 * @Date: 2024-09-04 11:05:21
 * @LastEditTime: 2024-09-04 11:05:21
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#ifndef __PARALLEL_FOREACH__
#define __PARALLEL_FOREACH__
#include "future_thread_pool.hpp"
#include "notify_thread_pool.hpp"
#include "simple_thread_pool.hpp"
#include <algorithm>
#include <future>
#include <iterator>
#include <thread>
#include <vector>

/// @brief simple while thread for foreach
/// @tparam Iterator
/// @tparam Func
/// @param first
/// @param last
/// @param f
template <typename Iterator, typename Func>
void simple_foreach(Iterator first, Iterator last, Func f) {
    unsigned long const length = std::distance(first, last);
    if (!length) return;
    unsigned long const minPerThread = 25;
    unsigned long const numThreads   = (length + minPerThread - 1) / minPerThread;
    unsigned long const blockSize    = length / numThreads;

    std::vector<std::future<void>> futures(numThreads - 1);
    Iterator                       blockStart = first;
    for (size_t i = 0; i < (numThreads - 1); ++i) {
        Iterator blockEnd = blockStart;
        std::advance(blockEnd, blockSize);

        SimpleThreadPool::instance().submit([=]() { std::for_each(blockStart, blockEnd, f); });
        blockStart = blockEnd;
    }
    std::for_each(blockStart, last, f);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

template <typename Iterator, typename Func>
void future_foreach(Iterator first, Iterator last, Func f) {
    unsigned long length = std::distance(first, last);
    if (!length) return;
    unsigned long const minPerThread = 25;
    unsigned long const numThreads   = (length + minPerThread - 1) / minPerThread;
    unsigned long       blockSize    = length / numThreads;

    std::vector<std::future<void>> futures(numThreads - 1);
    Iterator                       blockStart = first;
    for (size_t i = 0; i < (numThreads - 1); ++i) {
        Iterator blockEnd = blockStart;
        std::advance(blockEnd, blockSize);

        futures[i] = FutureThreadPool::instance().submit([=]() { std::for_each(blockStart, blockEnd, f); });
        blockStart = blockEnd;
    }
    std::for_each(blockStart, last, f);
    for (auto &future : futures)
        future.get();
}

template <typename Iterator, typename Func>
void notify_foreach(Iterator first, Iterator last, Func f) {
    unsigned long length = std::distance(first, last);
    if (!length) return;
    unsigned long const minPerThread = 25;
    unsigned long const numThreads   = (length + minPerThread - 1) / minPerThread;
    unsigned long       blockSize    = length / numThreads;

    std::vector<std::future<void>> futures(numThreads - 1);
    Iterator                       blockStart = first;
    for (size_t i = 0; i < (numThreads - 1); ++i) {
        Iterator blockEnd = blockStart;
        std::advance(blockEnd, blockSize);

        futures[i] = NotifyThreadPool::instance().submit([=]() { std::for_each(blockStart, blockEnd, f); });
        blockStart = blockEnd;
    }
    std::for_each(blockStart, last, f);
    for (auto &future : futures) {
        future.get();
    }
}

#endif //__PARALLEL_FOREACH__