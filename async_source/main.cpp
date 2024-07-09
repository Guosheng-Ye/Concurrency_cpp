
/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-06 09:04:02
 * @LastEditTime: 2024-07-06 09:04:06
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

class TestCopy {
public:
    TestCopy() {}
    TestCopy(const TestCopy &tp) {
        std::cout << "Test Copy Copy " << std::endl;
    }
    TestCopy(TestCopy &&cp) { std::cout << "Test Copy Move " << std::endl; }
};

TestCopy TestCp() {
    std::cout << "TestCp()" << std::endl;
    TestCopy tp;
    return tp;
}

std::unique_ptr<int> ReturnUniquePtr() {
    std::unique_ptr<int> uq_ptr = std::make_unique<int>(100);
    return uq_ptr;
}
std::thread ReturnThread() {
    std::thread t([]() {
        int i = 0;
        while (true) {
            std::cout << "i is " << i << std::endl;
            i++;
            if (i == 5) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    return t;
}

void test_return_unique_ptr() {
    auto rt_ptr = ReturnUniquePtr();
    std::cout << "rt_ptr value is " << *rt_ptr << std::endl;
    std::thread rt_thread = ReturnThread();
    rt_thread.join();
}

/// @brief async return rvalue future
void block_async() {
    std::cout << "Begin block async" << std::endl;
    {
        std::future<void> futures = std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cout << "std::async called. " << std::endl;
        });
    }
    std::cout << "end block async" << std::endl;
}

void test_dead_lock() {
    std::mutex mtx;
    std::cout << "dead lock beigin" << std::endl;
    std::lock_guard<std::mutex> lock(mtx);
    {
        std::future<void> futures = std::async(std::launch::async, [&mtx]() {
            std::cout << "std::async called" << std::endl;
            std::lock_guard<std::mutex> dLock(
                mtx); // error can not lock success
            std::cout << "async working..." << std::endl;
        });
    }
    std::cout << "dead lock end" << std::endl;
}

/*
需求是func1中要异步执行asyncFunc函数。
func2中先收集asyncFunc函数运行的结果，只有结果正确才执行
func1启动异步任务后继续执行，执行完直接退出不用等到asyncFunc运行完
*/
int async_func() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "async func" << std::endl;
    return 0;
}

void fun1(std::future<int> &future_ref) {
    std::cout << "fun1" << std::endl;
    future_ref = std::async(std::launch::async, async_func);
}

void fun2(std::future<int> &future_ref) {
    std::cout << "fun2" << std::endl;
    auto future_res = future_ref.get();
    if (future_res == 0) {
        std::cout << "get async result success" << std::endl;
    } else {
        std::cout << "get async result failed" << std::endl;
        return;
    }
}

void test_async_method() {
    std::future<int> future_tmp;
    fun1(future_tmp);
    fun2(future_tmp);
}

template <typename Func, typename... Args>
auto parallel_exe(Func &&func, Args &&...args)
    -> std::future<decltype(func(args...))> {

    using returnType = decltype(func(args...));
    std::function<returnType()> bind_func =
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    std::packaged_task<returnType()> task(bind_func);

    auto        return_feature = task.get_future();
    std::thread t(std::move(task));

    t.detach(); // 异步执行task
    return return_feature;
}

void test_parallel_1() {
    int i = 0;
    std::cout << "Begin TestParallel 1 ..." << std::endl;
    {
        parallel_exe(
            [](int i) {
                while (i < 3) {
                    i++;
                    std::cout << "Parallel thread func " << i << " times"
                              << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            },
            i);
    } 
    std::cout << "End test parallel 1..." << std::endl;
}

void test_parallel_2() {
    int i = 0;
    std::cout << "Begin TestParallel 2 ..." << std::endl;
    auto rt_future = parallel_exe(
        [](int i) {
            while (i < 5) {
                i++;
                std::cout << "Parallel thread func " << i << " times"
                          << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        },
        i);

    std::cout << "End test parallel 2..." << std::endl;
    rt_future.wait(); // wait for thread end
}

int main() {
    // auto t = TestCp();
    // test_return_unique_ptr();
    // block_async();
    // test_dead_lock();
    // test_async_method();

    // test_parallel_1();
    // std::this_thread::sleep_for(std::chrono::seconds(10));

    test_parallel_2();
    return 0;
}