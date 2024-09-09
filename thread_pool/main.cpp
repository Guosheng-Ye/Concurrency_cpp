/***
 * @Author: Ye Guosheng
 * @Date: 2024-06-25 15:43:30
 * @LastEditTime: 2024-06-25 15:43:30
 * @LastEditors: Ye Guosheng
 * @Description: Thread Pool
 */
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool {
public:
    using Task = std::packaged_task<void()>;

    ThreadPool(const ThreadPool &)            = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    static ThreadPool &instance() {
        static ThreadPool s_ins;
        return s_ins;
    }
    ~ThreadPool() { stop(); }

    /// @brief
    /// 将任务提交到线程池中执行。接受任意类型的函数和参数，返回future对象，以获取任务的执行结果
    /// @tparam F
    /// @tparam ...Args
    /// @param f 调用的回调函数
    /// @param ...args 可变参数
    /// @return std::future<decltype(f(args...))
    template <class F, class... Args>
    auto commit(F &&f, Args &&...args) -> std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))> {
        using ReturnType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));

        if (_ato_b_stop.load()) {
            return std::future<ReturnType>{};
        }

        // no stop
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        // f(args...)

        std::future<ReturnType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> lock(_cond_mtx);
            _tasks.emplace([task] { (*task)(); }); // 任务队列添加任务
        }
        _cond_var.notify_one();
        return ret;
    }

    int idleThreadCount() { return _ato_i_thread_num; }

private:
    ThreadPool(unsigned int num = 10)
        : _ato_b_stop(false) {
        {
            if (num < 1)
                _ato_i_thread_num = 1;
            else
                _ato_i_thread_num = num;
        }
        start();
    }

    /// @brief
    void start() {
        for (int i = 0; i < _ato_i_thread_num; ++i) {
            _pool.emplace_back([this]() {
                while (!this->_ato_b_stop.load()) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(_cond_mtx);
                        this->_cond_var.wait(lock,
                                             [this] { return this->_ato_b_stop.load() || !this->_tasks.empty(); });
                        if (this->_tasks.empty()) return;
                        task = std::move(this->_tasks.front());
                        this->_tasks.pop();
                    }
                    this->_ato_i_thread_num--;
                    task();
                    this->_ato_i_thread_num++;
                }
            });
        }
    }

    /// @brief
    void stop() {
        _ato_b_stop.store(true);
        _cond_var.notify_all();
        for (auto &td : _pool) {
            if (td.joinable()) {
                std::cout << "join thread " << td.get_id() << std::endl;
                td.join();
            }
        }
    }

private:
    std::mutex               _cond_mtx;
    std::condition_variable  _cond_var;
    std::atomic_bool         _ato_b_stop;
    std::atomic_int          _ato_i_thread_num;
    std::queue<Task>         _tasks;
    std::vector<std::thread> _pool;
};

/// @brief the address of m is different
void test_threadpool_error() {
    int m = 0;
    ThreadPool::instance().commit(
        [](int &m) {
            m = 1024;
            std::cout << "inner set m is " << m << std::endl;
            std::cout << "m address: " << &m << std::endl;
        },
        m);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "m is: " << m << std::endl;
    std::cout << "m address: " << &m << std::endl;
}

void test_no_error() {
    int m = 0;
    ThreadPool::instance().commit(
        [](int &m) {
            m = 1024;
            std::cout << "inner set m is " << m << std::endl;
            std::cout << "m address: " << &m << std::endl;
        },
        std::ref(m));

    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "m is: " << m << std::endl;
    std::cout << "m address: " << &m << std::endl;
}

class ThreadPool_ {
public:
    using TskType = std::function<void()>;
    explicit ThreadPool_(unsigned num_threads = std::thread::hardware_concurrency())
        : _ato_bool_stop(false) {

        // limit thrad_num
        int thread_num = std::thread::hardware_concurrency();
        if (thread_num == 0) thread_num = 5;
        if (num_threads == 0) num_threads = thread_num;
        if (num_threads > thread_num * 2) num_threads = thread_num;

        // create thread
        for (unsigned i = 0; i < num_threads; ++i) {
            _v_thread.emplace_back([this]() { threadFuncion(); });
        }
    }
    ~ThreadPool_() noexcept {
        _ato_bool_stop.store(false, std::memory_order_acquire);
        _cond_v.notify_all();
        for (auto &t : _v_thread) {
            t.join();
        }
    }

    template <typename T>
    auto addTask(T func) -> std::future<decltype(func())> {
        using ReturnType = decltype(func());
        auto task        = std::make_shared<std::packaged_task<ReturnType()>>(std::move(func));
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _q_tasks.emplace([task]() { (*task)(); });
        }

        _cond_v.notify_one();
        return task->get_future();
    }

private:
    /// @brief function of each thread
    void threadFuncion() {
        while (true) {
            TskType task;
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _cond_v.wait(
                    lock, [this]() { return (_ato_bool_stop.load(std::memory_order_acquire) || !_q_tasks.empty()); });

                if (_ato_bool_stop.load(std::memory_order_acquire) && _q_tasks.empty()) break;

                task = std::move(_q_tasks.front());
                _q_tasks.pop();
            }
            task();
        }
    }

private:
    std::vector<std::thread> _v_thread;
    std::queue<TskType>      _q_tasks;
    std::atomic_bool         _ato_bool_stop;
    std::mutex               _mtx;
    std::condition_variable  _cond_v;
};

void test_threadpool_() {
    int                           num_tasks   = 5;
    int                           num_threads = 5;
    ThreadPool_                   thread_pool(num_threads);
    std::vector<std::future<int>> futures;
    for (int i = 0; i < num_tasks; ++i) {
        futures.emplace_back(thread_pool.addTask([i]() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return i;
        }));
    }
    for (int i = 0; i < num_tasks; ++i) {
        std::cout << futures[i].get() << std::endl;
    }
}

int main() {
    // test_no_error();
    // test();
    test_threadpool_();
    return 0;
}