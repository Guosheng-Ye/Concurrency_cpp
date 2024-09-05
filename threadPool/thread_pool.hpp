/***
 * @Author: Oneko
 * @Date: 2024-09-05 09:58:07
 * @LastEditTime: 2024-09-05 09:59:12
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#ifndef __THREAD_POOL__
#define __THREAD_POOL__
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class NoCopy {
public:
    ~NoCopy() {}

protected:
    NoCopy() {}

private:
    NoCopy(const NoCopy &)            = delete;
    NoCopy &operator=(const NoCopy &) = delete;
};

class ThreadPool : public NoCopy {
public:
    static ThreadPool &instance() {
        static ThreadPool pool;
        return pool;
    }

    using Task = std::packaged_task<void()>;

    ~ThreadPool() { stop(); }

    template <class F, class... Args>
    auto commit(F &&f, Args &&...args) -> std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))> {
        using returnType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
        if (_stopFlag.load()) return std::future<returnType>{};
        auto task = std::make_shared<std::packaged_task<returnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<returnType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> lockGuard(_mtx);
            _tasks.emplace([task] { (*task)(); });
        }
        _condv.notify_one();
        return ret;
    }
    int idel_thrad_count() { return _threadNum; }

private:
    ThreadPool(unsigned int num_ = std::thread::hardware_concurrency())
        : _stopFlag(false) {
        {
            if (num_ <= 1)
                _threadNum = 2;
            else
                _threadNum = num_;
        }
        start();
    }

    void start() {
        for (size_t i = 0; i < _threadNum; ++i) {
            _pools.emplace_back([this]() {
                while (!this->_stopFlag.load()) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(_mtx);
                        this->_condv.wait(lock, [this]() { return this->_stopFlag.load() || !this->_tasks.empty(); });
                        if (this->_tasks.empty()) return;
                        task = std::move(this->_tasks.front());
                        this->_tasks.pop();
                    }
                    this->_threadNum--;
                    task();
                    this->_threadNum++;
                }
            });
        }
    }

    void stop() {
        _stopFlag.store(true);
        _condv.notify_all();
        for (auto &p : _pools) {
            if (p.joinable()) {
                std::cout << "p: " << p.get_id() << " join()" << std::endl;
                p.join();
            }
        }
    }

private:
    std::mutex               _mtx;
    std::condition_variable  _condv;
    std::atomic_bool         _stopFlag;
    std::atomic_int          _threadNum;
    std::queue<Task>         _tasks;
    std::vector<std::thread> _pools;
};

#endif //__THREAD_POOL__
