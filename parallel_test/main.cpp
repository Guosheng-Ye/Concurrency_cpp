/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-01 19:45:09
 * @LastEditTime: 2024-07-04 11:29:43
 * @LastEditors: Ye Guosheng
 * @Description:  parallet test for quick sort
 */

#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <future>
#include <iostream>
#include <list>
#include <queue>
#include <random>
#include <thread>

/// @brief sequential func
/// @tparam T
/// @param input_list
/// @return std::list<T>
template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input_list) {
    if (input_list.empty()) return input_list;
    std::list<T> result;

    // move input.begin() into result
    result.splice(result.begin(), input_list, input_list.begin());

    // set the first to split
    T const &pivot = *result.begin();

    // divide_point -> first value >= pivot in input_list
    auto divide_point = std::partition(input_list.begin(), input_list.end(),
                                       [&](T const &t) { return t < pivot; });

    // put <= pivot value intpu lower_part
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input_list, input_list.begin(),
                      divide_point);

    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input_list)));

    // new_higher concat to result tail
    result.splice(result.end(), new_higher);
    // new_lower concat to result head
    result.splice(result.begin(), new_lower);
    return result;
}

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
    if (input.empty()) return input;
    std::list<T> result;

    result.splice(result.begin(), input, input.begin());
    T const     &pivot        = *result.begin();
    auto         divide_point = std::partition(input.begin(), input.end(),
                                               [&](T const &t) { return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    std::future<std::list<T>> new_lower(
        std::async(&parallel_quick_sort<T>, std::move(lower_part)));

    auto new_higher(parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

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
    auto commit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));

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
                        this->_cond_var.wait(lock, [this] {
                            return this->_ato_b_stop.load() ||
                                   !this->_tasks.empty();
                        });
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

template <typename T>
std::list<T> thread_pool_quick_sort(std::list<T> input) {
    if (input.empty()) return input;
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const &pivot        = *result.begin();
    auto     divide_point = std::partition(input.begin(), input.end(),
                                           [&](T const &t) { return t < pivot; });

    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    auto new_lower = ThreadPool::instance().commit(&parallel_quick_sort<T>,
                                                   std::move(lower_part));

    auto new_higher(parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

std::list<int> numlists = {};

void set_random() {
    std::uniform_int_distribution<int> u(-1000, 1000);
    std::default_random_engine         e;
    for (size_t i = 0; i < 100000; ++i) {
        numlists.push_back(u(e));
    }
}

void test_sequential_sort() {
    // std::list<int> numlists    = {6, 1, 1, 0, 7, 19, 5, 2, 9, -1};
    auto sort_result = sequential_quick_sort(numlists);
    // std::cout << "sorted result is ";
    // // for (auto iter = sort_result.begin(); iter != sort_result.end();
    // iter++)
    // // {
    // //     std::cout << " " << (*iter);
    // // }
    // for (auto &e : sort_result)
    //     std::cout << e << " ";
    std::cout << std::endl;
}

void test_parallel_quick() {
    // std::list<int> numlists    = {10, 6, 1, 0, 7, 5, 2, 9, -1};
    auto sort_result = parallel_quick_sort(numlists);
    // std::cout << "sorted result is ";

    // for (auto &e : sort_result)
    //     std::cout << e << " ";
    std::cout << std::endl;
}

void test_thread_pool_sort() {
    // std::list<int> numlists    = {6, 1, 0, 7, 5, 2, 9, -1, 10, -99, 21};
    auto sort_result = thread_pool_quick_sort(numlists);
    // std::cout << "sorted result is ";
    // for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++)
    // {
    //     std::cout << " " << (*iter);
    // }
    std::cout << std::endl;
}

TEST(test_sequential_sort, sequential_sort) { test_sequential_sort(); }

TEST(test_parallel_quick, parallel_sort) { test_parallel_quick(); }

TEST(test_thread_pool_sort, thread_pool_sort) { test_thread_pool_sort(); }

int main() {

    set_random();

    testing::InitGoogleTest();
    // 默认启用彩色输出和显示时间
    ::testing::GTEST_FLAG(color)      = "yes";
    ::testing::GTEST_FLAG(print_time) = true;
    return RUN_ALL_TESTS();
}