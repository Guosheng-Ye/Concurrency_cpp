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

/**
 *
 * @param maxMum_ max value for i
 * @return popCount
 */
int test_single_queue(int maxMum_) {

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

constexpr int TESTCOUNT = 200;

int test_multiple_queue() {
    LockFreeQueue<int> que;

    std::thread t1([&]() {
        for (int i = 0; i < TESTCOUNT * 100; i++) {
            que.push(i);
            std::cout << "push data is " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < TESTCOUNT * 100; i++) {
            que.push(i);
            std::cout << "push data is " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t3([&]() {
        for (int i = 0; i < TESTCOUNT * 100; i++) {
            que.push(i);
            std::cout << "push data is " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t4([&]() {
        for (int i = 0; i < TESTCOUNT * 100; i++) {
            que.push(i);
            std::cout << "push data is " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t5([&]() {
        for (int i = 0; i < TESTCOUNT * 100;) {
            auto p = que.pop();
            if (p == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            i++;
            std::cout << "pop data is " << *p << std::endl;
        }
    });

    std::thread t6([&]() {
        for (int i = 0; i < TESTCOUNT * 100;) {
            auto p = que.pop();
            if (p == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            i++;
            std::cout << "pop data is " << *p << std::endl;
        }
    });
    std::thread t7([&]() {
        for (int i = 0; i < TESTCOUNT * 100;) {
            auto p = que.pop();
            if (p == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            i++;
            std::cout << "pop data is " << *p << std::endl;
        }
    });

    std::thread t8([&]() {
        for (int i = 0; i < TESTCOUNT * 100;) {
            auto p = que.pop();
            if (p == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            i++;
            std::cout << "pop data is " << *p << std::endl;
        }
    });
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    std::cout << "construct count is " << que.construct_count.load()
              << std::endl;
    std::cout << "destruct count is " << que.destruct_count.load() << std::endl;
    // assert(que.destruct_count == TESTCOUNT * 100 * 4);
    return que.destruct_count.load(std::memory_order_acquire);
}

/// @brief 单生产者,单消费者的无锁队列
/// @param
/// @param
// TEST(test_single_case, pop_maxmum_when_push_maxmum) {
//     int maxMum_ = 1000;
//     EXPECT_EQ(test_single_queue(maxMum_), maxMum_);
// }

TEST(test_multiple_queue, pop_maxmum_when_push_maxmum) {
    EXPECT_EQ(test_multiple_queue(), TESTCOUNT * 100 * 4);
}

int main() {
    test_multiple_queue();
    // testing::InitGoogleTest();
    // // 默认启用彩色输出和显示时间
    // ::testing::GTEST_FLAG(color)      = "yes";
    // ::testing::GTEST_FLAG(print_time) = true;
    // return RUN_ALL_TESTS();
}