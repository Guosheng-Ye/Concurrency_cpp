/***
 * @Author: Oneko
 * @Date: 2024-09-05 09:11:39
 * @LastEditTime: 2024-09-05 09:12:43
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#ifndef __STEAL_THREAD_POOL_
#define __STEAL_THREAD_POOL_
#include "future_thread_pool.hpp"
#include "join_thread.hpp"
#include "thread_safe_queue.hpp"
#include <cstddef>
#include <future>

class StealThreadPool {

private:
    void work_thread(int index_) {
        while (!_doneFlag) {
            FunctionWrapper wrapper;
            bool            popRes = _threadWorkQueues[index_].try_pop(wrapper);
            if (popRes) {
                wrapper();
                continue;
            }
            bool stealRes = false;
            for (size_t i = 0; i < _threadWorkQueues.size(); ++i) {
                if (i == index_) continue;

                stealRes = _threadWorkQueues[i].try_steal(wrapper);
                if (stealRes) {
                    wrapper();
                    break;
                }
            }
            if (stealRes) continue;
            std::this_thread::yield();
        }
    }

public:
    static StealThreadPool &instance() {
        static StealThreadPool pool;
        return pool;
    }
    ~StealThreadPool() {
        _doneFlag = true;
        for (size_t i = 0; i < _threadWorkQueues.size(); ++i) {
            _threadWorkQueues[i].exit();
        }
        for (size_t i = 0; i < _threads.size(); ++i)
            _threads[i].join();
    }

    template <typename FunctionType>
    auto sumbit(FunctionType f) -> std::future<decltype(f())> {
        int index = (_atmIndex.load() + 1) % _threadWorkQueues.size();
        _atmIndex.store(index);
        using returnType = decltype(f());
        std::packaged_task<returnType()> task(std::move(f));
        std::future<returnType>          res(task.get_future());
        _threadWorkQueues[index].push(std::move(task));
        return res;
    }

private:
    StealThreadPool()
        : _doneFlag(false)
        , _joiner(_threads)
        , _atmIndex(0) {

        unsigned const threadCount = std::thread::hardware_concurrency();
        try {
            _threadWorkQueues = std::vector<ThreadSafeQueue<FunctionWrapper>>(threadCount);

            for (size_t i = 0; i < threadCount; ++i) {
                _threads.push_back(std::thread(&StealThreadPool::work_thread, this, i));
            }
        } catch (const std::exception &e) {
            _doneFlag = true;
            for (size_t i = 0; i < _threadWorkQueues.size(); ++i) {
                _threadWorkQueues[i].exit();
            }
            throw;
        }
    }

private:
    std::atomic_bool                              _doneFlag;
    std::vector<ThreadSafeQueue<FunctionWrapper>> _threadWorkQueues;
    std::vector<std::thread>                      _threads;
    JoinThread                                    _joiner;
    std::atomic<int>                              _atmIndex;
};
#endif