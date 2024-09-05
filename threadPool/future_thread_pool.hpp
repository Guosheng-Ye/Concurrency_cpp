/***
 * @Author: Oneko
 * @Date: 2024-09-03 17:41:47
 * @LastEditTime: 2024-09-03 17:41:48
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#ifndef __FUTURE_THREAD_POOL__
#define __FUTURE_THREAD_POOL__

#include "join_thread.hpp"
#include "thread_safe_queue.hpp"
#include <future>
#include <memory>
#include <type_traits>

class FunctionWrapper {
private:
    struct ImplBase {
        virtual void call() = 0;
        virtual ~ImplBase() {}
    };
    std::unique_ptr<ImplBase> _impl;

    template <typename F>
    struct ImplType : ImplBase {
        F f;
        ImplType(F &&f_)
            : f(std::move(f_)) {}
        void call() { f(); }
    };

public:
    template <typename F>
    FunctionWrapper(F &&f_)
        : _impl(new ImplType<F>(std::move(f_))) {} // polymorphism

    void operator()() { _impl->call(); }

    FunctionWrapper() = default;
    FunctionWrapper(FunctionWrapper &&other_)
        : _impl(std::move(other_._impl)) {}
    FunctionWrapper &operator=(FunctionWrapper &&other_) {
        _impl = std::move(other_._impl);
        return *this;
    }
    FunctionWrapper(const FunctionWrapper &)             = delete;
    FunctionWrapper(FunctionWrapper &)                   = delete;
    FunctionWrapper &operator=(const FunctionWrapper &&) = delete;
};

class FutureThreadPool {
private:
    std::atomic_bool                 _doneFlag;
    ThreadSafeQueue<FunctionWrapper> _workQueue;
    std::vector<std::thread>         _threads;
    JoinThread                       _joiner;

public:
    static FutureThreadPool &instance() {
        static FutureThreadPool pool;
        return pool;
    }
    ~FutureThreadPool() {
        _doneFlag = true;
        for (size_t i = 0; i < _threads.size(); ++i) {
            _threads[i].join();
        }
    }

    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
        using resultType = typename std::result_of<FunctionType()>::type;
        // using resultType = decltype(f());
        std::packaged_task<resultType()> task(std::move(f));
        std::future<resultType>          res(task.get_future());
        _workQueue.push(std::move(task));
        return res;
    }

private:
    FutureThreadPool()
        : _doneFlag(false)
        , _joiner(_threads) {
        size_t const threadCount = std::thread::hardware_concurrency();
        try {
            for (size_t i = 0; i < threadCount; ++i) {
                _threads.push_back(std::thread(&FutureThreadPool::work_thread, this));
            }
        } catch (const std::exception &e) {
            _doneFlag = true;
            throw;
        }
    }

    void work_thread() {
        while (!_doneFlag) {
            FunctionWrapper task;
            if (_workQueue.try_pop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }
};
#endif // __FUTURE_THREAD_POOL__