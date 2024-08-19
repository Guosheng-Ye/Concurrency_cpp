
#include "spdlog/spdlog.h"
#include <algorithm>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

/// @brief 线程管理
class JoinThreads {
public:
    explicit JoinThreads(std::vector<std::thread> &threads_)
        : _threads(threads_) {}
    ~JoinThreads() {
        for (unsigned long i = 0; i < _threads.size(); ++i) {
            if (_threads[i].joinable()) _threads[i].join();
        }
    }

private:
    std::vector<std::thread> &_threads;
};

/// @brief 并行for_each
/// @tparam Iterator
/// @tparam Func
/// @param first
/// @param last
/// @param func
template <typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func func) {
    unsigned long dataLength = std::distance(first, last);
    if (!dataLength) return;

    unsigned long const minCalEachThread = 20; // 设置每个线程的计数数量
    unsigned long const maxThreads =           // 计数最大线程数
        (dataLength + minCalEachThread - 1) / minCalEachThread;
    unsigned long const hardwareThreads = std::thread::hardware_concurrency();
    unsigned long const numThreads      = // 获取最终需要的线程数
        std::min(hardwareThreads != 0 ? hardwareThreads : 2, maxThreads);
    spdlog::info("num of threads is:{} ", numThreads);
    unsigned long const blockSize = dataLength / numThreads; // 每个线程的块大小

    std::vector<std::future<void>> futures(numThreads - 1);
    std::vector<std::thread>       threads(numThreads - 1);

    JoinThreads joiner(threads); // 线程管理
    Iterator    blockStart = first;

    for (unsigned long i = 0; i < (numThreads - 1); ++i) {
        Iterator blockEnd = blockStart; // 初始化每个线程的块尾地址
        std::advance(blockEnd, blockSize); // 获得每个线程的块区域

        std::packaged_task<void(void)> task(
            [=]() { std::for_each(blockStart, blockEnd, func); }); // 任务打包
        futures[i] = task.get_future();            // 存储结果
        threads[i] = std::thread(std::move(task)); // 线程绑定
        blockStart = blockEnd;
    }

    std::for_each(blockStart, last, func); // 处理剩余任务
    for (unsigned long i = 0; i < (numThreads - 1); ++i) {
        futures[i].get();
    }
}

/// @brief 递归for_each
/// @tparam Iterator
/// @tparam Func
/// @param first
/// @param last
/// @param func
template <typename Iterator, typename Func>
void async_for_each(Iterator first, Iterator last, Func func) {
    unsigned long const dataLength = std::distance(first, last);
    if (!dataLength) return;

    unsigned long const minCalEachThread = 20;
    if (dataLength < (2 * minCalEachThread)) {
        std::for_each(first, last, func);
    } else {
        Iterator const    mid = first + dataLength / 2; // 截半
        std::future<void> first_half =
            std::async(&async_for_each<Iterator, Func>, first, mid,
                       func);            // 前半部分给async
        async_for_each(mid, last, func); // 本线程处理后半部分
        first_half.get();                // 本线程获取前半部分的结果
    }
}