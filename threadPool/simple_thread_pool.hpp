/*** 
 * @Author: Oneko
 * @Date: 2024-09-03 16:06:47
 * @LastEditTime: 2024-09-05 09:56:57
 * @LastEditors: Ye Guosheng
 * @Description: 
 */
#ifndef __SIMPLE_THREAD_POOL__
#define __SIMPLE_THREAD_POOL__

#include "join_thread.hpp"
#include "thread_safe_queue.hpp"
#include <atomic>
#include <functional>

class SimpleThreadPool {
public:
    static SimpleThreadPool &instance() {
        static SimpleThreadPool pool;
        return pool;
    }
    ~SimpleThreadPool() {
        _doneFlag = true;
        for (size_t i = 0; i < _threads.size(); ++i) {
            _threads[i].join();
        }
    }

    template <typename FunctionType>
    void submit(FunctionType f) {
        _workQueue.push(std::function<void()>(f));
    }

private:
    SimpleThreadPool()
        : _doneFlag(false)
        , _joiner(_threads) {
        unsigned const threadCount = std::thread::hardware_concurrency();
        try {
            for (size_t i = 0; i < threadCount; ++i) {
                _threads.push_back(std::thread(&SimpleThreadPool::worker_thread, this));
            }
        } catch (...) {
            _doneFlag = true;
            throw;
        }
    }

    void worker_thread() {
        while (!_doneFlag) {
            std::function<void()> task;
            if (_workQueue.try_pop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }

private:
    std::atomic_bool                       _doneFlag;
    ThreadSafeQueue<std::function<void()>> _workQueue;
    std::vector<std::thread>               _threads;
    JoinThread                             _joiner;
};

#endif //__SIMPLE__THREAD_POOL__