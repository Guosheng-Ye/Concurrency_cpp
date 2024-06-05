/***
 * @Author: Ye Guosheng
 * @Date: 2024-03-21 17:05:37
 * @LastEditTime: 2024-03-22 10:59:22
 * @LastEditors: Ye Guosheng
 * @Description: 线程管理
 */
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <unordered_map>
#include <sstream>

/// @brief RAII: Resource Acquisition Is Initialization
class ThreadGuard
{
    std::thread &_t;

public:
    explicit ThreadGuard(std::thread &t) : _t(t) {} // 引用必须初始化
    ~ThreadGuard()
    {
        if (_t.joinable()) // 判断线程是否已加入
        {
            std::cout << "_t.joinable()" << std::endl;
            _t.join();
            std::cout << "_t.join();" << std::endl;
        }
    }
    ThreadGuard(ThreadGuard const &) = delete;            // delete拷贝构造
    ThreadGuard &operator=(ThreadGuard const &) = delete; // delete拷贝赋值
};

struct func
{
    int &_r_var;
    func(int &i_) : _r_var(i_) {}
    void operator()()
    {
        for (unsigned j = 0; j < 20; ++j)
        {
            ++_r_var;
            std::cout << "In thread: var:" << _r_var << std::endl;
        }
    }
};

void f1()
{
    std::cout << "this is f1 function()" << std::endl;
}
void f2()
{
    std::cout << "this is f2 function()" << std::endl;
}

void demo1()
{
    int somState = 0;
    {
        func myFunc(somState);
        std::thread myThread(myFunc);
        std::cout << "after std::thread myThread(myFunc);" << std::endl;

        std::cout << "ThreadGuard g(myThread);" << std::endl;
        ThreadGuard g(myThread);
        std::cout << "in main thread" << std::endl;
    }
    std::cout << "Finish somState: " << somState << std::endl;
}

void demo2()
{
    std::thread t1(f1); // t1 -> f1

    std::thread t2 = std::move(t1); // t2 -> f1,t1->null

    t1 = std::thread(f2); // t1 -> f2, t2->f1

    std::thread t3;     // null handle
    t3 = std::move(t2); // t3 -> f1, t2->null, t1->t2
    t1 = std::move(t3); // t1->f1,t1->f2,t2->null,t3->null
}

/// @brief
class ScopedThread
{

    std::thread _t_thread;

public:
    explicit ScopedThread(std::thread t) : _t_thread(std::move(t))
    {
        if (!_t_thread.joinable())
        {
            throw std::logic_error("No thread");
        }
    }
    ~ScopedThread()
    {
        _t_thread.join();
        std::cout << "join() completed" << std::endl;
    }

    ScopedThread(ScopedThread const &) = delete;
    ScopedThread &operator=(ScopedThread const &) = delete;
};

void demo3()
{
    int some_local_state = 0;
    ScopedThread T{std::thread(func(some_local_state))};
    std::cout << "demo3 thread" << std::endl;
}

/// @brief
/// @return
void convert()
{
    uint16_t sum = 0;
    int received_data[6] = {0xAA, 0xFF, 0x48, 0xFF, 0x62, 0x52}; // Example received data

    // Extracting X and Y control values
    int16_t x, y;

    x = (received_data[1] << 8) | (received_data[2]);
    y = (received_data[3] << 8) | (received_data[4]);

    // Converting back to original values (dividing by 1000)
    float original_x = static_cast<float>(x) / 1000.0;
    float original_y = static_cast<float>(y) / 1000.0;

    // Checking checksum
    for (int i = 0; i < 5; ++i)
    {
        sum += received_data[i];
    }

    if ((sum & 0xFF) == received_data[5])
    {
        // Checksum matches, print the values
        std::cout << "Received X: " << original_x << std::endl;
        std::cout << "Received Y: " << original_y << std::endl;
    }
    else
    {
        std::cout << "Checksum mismatch! Data may be corrupted." << std::endl;
    }
}

void printTime()
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 转换为 time_t
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // 获取毫秒部分
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 格式化时间
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now_c));

    // 输出时分秒毫秒
    std::cout << "now_time:" << buffer << '.' << std::setfill('0') << std::setw(3) << ms.count() << std::endl;
}

/// @brief 可调用对象,累加first-end
/// @tparam Iterator
/// @tparam T
template <typename Iterator, typename T>
struct accumulate_block
{
    void operator()(Iterator first, Iterator end, T &result)
    {
        result = std::accumulate(first, end, result);
    }
};

/// @brief 并行累计计算
/// 1. 计算容器内的元素个数，并确定实际使用的线程数
/// 2. 根据线程数和元素个数，计算每个线程处理的元素块大小
/// 3. 利用容器存储每个线程的累加结果
/// 4. 遍历并创建线程，每个线程处理一个元素块，结果存于容器
/// 5. 处理剩余的最后一个元素块
/// 6. 等待所有线程处理完成，累加所有结果，返回最终结果
/// @tparam Iterator     迭代器类型
/// @tparam T            元素类型
/// @param first         开始位置
/// @param last          结束位置
/// @param init          初始值
/// @return T            返回累加值
template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last); // 计算元素数量
    if (!length)                                             // 一个数
        return init;
    unsigned long const min_per_thread = 1000;                                                             // 每个线程的最小处理数数量
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;                      // 最大线程数,每个线程至少处理min_per_thread
    unsigned long const hardware_threads = std::thread::hardware_concurrency();                            // 硬件支持线程数
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads); // 使用合适的线程数
    unsigned long const block_size = length / num_threads;                                                 // 计算独立线程内处理的块大小

    std::vector<T> results(num_threads);               // 存放每个线程的计算题结果
    std::vector<std::thread> threads(num_threads - 1); // 线程容器
    Iterator block_start = first;                      // 块起始位

    for (size_t i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);                                                                     // block_end 前进到block_size处
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i])); // create thread[i]
        block_start = block_end;                                                                                 // updata strat
    }
    accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]); // 处理剩余元素

    // for (auto &entry : threads)
    // {
    //     //std::cout<<std::this_thread::get_id()<<std::endl;
    //     // entry.join();
    // }

    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    return std::accumulate(results.begin(), results.end(), init);
}

void test_parallel()
{
    std::vector<long> ve;
    for (long i = 0; i < 10000; i++)
        for (long j = 0; j < 1000; j++)
            for (long k = 0; k < 200; k++)
                ve.push_back(k);

    std::cout << "begin std::accumulate()" << std::endl;
    printTime();
    auto sum = std::accumulate(ve.begin(), ve.end(), 0);
    std::cout << sum << std::endl;
    printTime();
    std::cout << "end std::accumulate()" << std::endl
              << std::endl;

    std::cout << "begin parallel_accumulate()" << std::endl;
    printTime();
    auto sumParallel = parallel_accumulate(ve.begin(), ve.end(), 0);
    // auto sum = std::accumulate(ve.begin(),ve.end(),0);
    std::cout << sumParallel << std::endl;
    printTime();
    std::cout << "end parallel_accumulate()" << std::endl
              << std::endl;
}

std::unordered_map<std::thread::id, std::string> unorderedMap4Thread;
void thread_fun()
{
    std::thread::id threadId = std::this_thread::get_id();
    std::stringstream ss;
    ss << threadId;
    std::string thredString = ss.str();
    unorderedMap4Thread[threadId] = thredString;
}

void test_thread_id()
{
    std::thread t1(thread_fun);
    std::thread t2(thread_fun);

    t1.join(), t2.join();
    for (auto &t : unorderedMap4Thread)
    {
        std::cout << "Id: " << t.first << " Value: " << t.second << std::endl;
    }
}
int main()
{
    // demo1();
    // demo2();
    // demo3();
    // convert();
    // test_parallel();
    test_thread_id();
    return 0;
}
