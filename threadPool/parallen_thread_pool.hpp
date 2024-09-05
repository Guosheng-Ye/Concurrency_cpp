/***
 * @Author: Oneko
 * @Date: 2024-09-04 16:17:57
 * @LastEditTime: 2024-09-05 08:50:08
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#ifndef __PARALLEN_THREAD_POOL__
#define __PARALLEN_THREAD_POOL__

#include "future_thread_pool.hpp"
#include "join_thread.hpp"
#include "thread_safe_queue.hpp"
#include <atomic>

class ParallenThreadPool {

private:
    void work_thread(int index_) {
        while (_doneFlag) {
            auto taskPtr = _threadWorkQueues[index_].wait_and_pop();
            if (taskPtr == nullptr) continue;
            (*taskPtr)();
        }
    }

public:
    static ParallenThreadPool &instance() {
        static ParallenThreadPool pool;
        return pool;
    }
    ~ParallenThreadPool() {
        _doneFlag = true;
        for (size_t i = 0; i < _threadWorkQueues.size(); ++i) {
            _threadWorkQueues[i].exit();
        }

        for (size_t i = 0; i < _threads.size(); ++i) {
            _threads[i].join();
        }
    }

    template <typename FunctionType>
    // std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
    auto submit(FunctionType f) -> std::future<decltype(f())> {
        int index = (_atmIndex.load() + 1) % _threadWorkQueues.size();
        _atmIndex.store(index);

        using returnType = decltype(f());
        std::packaged_task<returnType()> task(std::move(f));
        std::future<returnType>          res(task.get_future());
        _threadWorkQueues[index].push(std::move(task));
        return res;
    }

private:
    ParallenThreadPool()
        : _doneFlag(false)
        , _joiner(_threads)
        , _atmIndex(0) {

        unsigned const thread_count = std::thread::hardware_concurrency();
        try {
            _threadWorkQueues = std::vector<ThreadSafeQueue<FunctionWrapper>>(thread_count);
            for (unsigned i = 0; i < thread_count; ++i) {
                // ⇽-- - 9
                _threads.push_back(std::thread(&ParallenThreadPool::work_thread, this, i));
            }
        } catch (...) {
            // ⇽-- - 10
            _doneFlag = true;
            for (int i = 0; i < _threadWorkQueues.size(); i++) {
                _threadWorkQueues[i].exit();
            }
            throw;
        }
    }

private:
    std::atomic_bool _doneFlag;
    JoinThread       _joiner;
    // global queue
    std::vector<ThreadSafeQueue<FunctionWrapper>> _threadWorkQueues;
    std::vector<std::thread>                      _threads;
    std::atomic<int>                              _atmIndex;
};

#endif //__PARALLEN_THREAD_POOL__