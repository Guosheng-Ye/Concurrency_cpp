/*** 
 * @Author: Oneko
 * @Date: 2024-09-04 15:30:37
 * @LastEditTime: 2024-09-05 10:24:39
 * @LastEditors: Ye Guosheng
 * @Description: 
 */
#ifndef __NOTIFY_THREAD_POOL_
#define __NOTIFY_THREAD_POOL_

#include "future_thread_pool.hpp"
#include "join_thread.hpp"
#include "thread_safe_queue.hpp"

class NotifyThreadPool {
private:
    void worker_thread() {
        while (!_doneFlag) {
            auto taskPtr = _workQueue.wait_and_pop();
            if (taskPtr == nullptr) continue;
            (*taskPtr)();
        }
    }

public:
    static NotifyThreadPool &instance() {
        static NotifyThreadPool pool;
        return pool;
    }
    ~NotifyThreadPool() {
        _doneFlag = true;
        _workQueue.exit();
        for (size_t i = 0; i < _threads.size(); ++i) {
            _threads[i].join();
        }
    }
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
        // typedef typename std::result_of<FunctionType()>::type result_type;
        using resultType = decltype(f());
        std::packaged_task<resultType()> task(std::move(f));
        std::future<resultType>          res(task.get_future());
        _workQueue.push(std::move(task));
        return res;
    }

private:
    NotifyThreadPool()
        : _doneFlag(false)
        , _joiner(_threads) {
        unsigned const threadCount = std::thread::hardware_concurrency();
        try {
            for (size_t i = 0; i < threadCount; ++i) {
                _threads.push_back(std::thread(&NotifyThreadPool::worker_thread, this));
            }
        } catch (const std::exception &e) {
            _doneFlag = true;
            _workQueue.exit();
            throw;
        }
    }

private:
    std::atomic_bool                 _doneFlag;
    ThreadSafeQueue<FunctionWrapper> _workQueue;
    std::vector<std::thread>         _threads;
    JoinThread                       _joiner;
};

#endif // __NOTIFY_THREAD_POOL_