/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-29 11:15:56
 * @LastEditTime: 2024-07-29 14:16:14
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#include "LockFreeQueue.hpp"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <chrono>
#include <thread>

int test_singlequeue(int maxMum_) {

    int              popCount = 0;
    SingleQueue<int> singleQueue;
    std::thread      t1([&]() {
        for (int i = 0; i < maxMum_; ++i) {
            singleQueue.push(i);
            spdlog::error("push data is {0}", i);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < maxMum_; ++i) {
            auto p = singleQueue.pop();
            if (p == nullptr) {
                spdlog::info("thread2 is waiting...");
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                --i; // if not add, pop num is half
                continue;
            }
            spdlog::warn("pop p is {0}", *p);
            ++popCount;
        }
    });

    t1.join();
    t2.join();
    return popCount;
}

/// @brief 单生产者,单消费者的无锁队列
/// @param
/// @param
TEST(test_case, pop_100000_when_push_100000) {
    int maxMum_ = 10000;
    EXPECT_EQ(test_singlequeue(maxMum_), maxMum_);
}

int main() {
    // test_singlequeue();
    testing::InitGoogleTest();
    // 默认启用彩色输出和显示时间
    ::testing::GTEST_FLAG(color)      = "yes";
    ::testing::GTEST_FLAG(print_time) = true;
    return RUN_ALL_TESTS();
}