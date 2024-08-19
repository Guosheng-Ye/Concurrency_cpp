/*** 
 * @Author: Oneko
 * @Date: 2024-08-16 14:31:17
 * @LastEditTime: 2024-08-19 13:22:28
 * @LastEditors: Ye Guosheng
 * @Description: 
 */
/***
 * @Author: Oneko
 * @Date: 2024-08-16 14:31:17
 * @LastEditTime: 2024-08-16 16:15:55
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#include "parallel_for_each.cpp"
#include "gtest/gtest.h"
#include <cstdint>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

std::vector<int64_t> testDatas{};

/// @brief generate random datas
void set_random_tests() {
    std::uniform_int_distribution<int> u(10, 100);
    std::default_random_engine         e;
    for (size_t i = 0; i < 999999; ++i) {
        testDatas.push_back(u(e));
    }

    // for (auto &e : testDatas) {
    //     std::cout << e << " ";
    // }
    spdlog::info("set_random_ok\n");
}

void print_results(std::vector<int64_t> const &res) {
    for (const &elem : res) {
        std::cout << elem << " ";
    }
    spdlog::error("print end...");
}

/// @brief
/// @param
/// @param
TEST(for_each_test, single_for_each) {
    spdlog::info("single for each");
    std::vector<int64_t> singleDatae(testDatas);
    for (auto &elem : singleDatae) {
        elem *= elem;
    }
    // print_results(singleDatae);
}

TEST(for_each_test, parallel_for_each_test) {
    spdlog::info("parallel_for_each_test");
    std::vector<int64_t> parallelDatas(testDatas);
    parallel_for_each(parallelDatas.begin(), parallelDatas.end(),
                      [](int64_t &i) { i *= i; });
    // print_results(parallelDatas);
}

TEST(for_each_test, async_for_eact_test) {
    spdlog::info("async_for_each_test");
    std::vector<int64_t> asyncDatas(testDatas);
    async_for_each(asyncDatas.begin(), asyncDatas.end(),
                   [](int64_t &i) { i *= i; });
}

int main() {
    set_random_tests();

    testing::InitGoogleTest();
    ::testing::GTEST_FLAG(color)      = "yes";
    ::testing::GTEST_FLAG(print_time) = true;
    return RUN_ALL_TESTS();
}