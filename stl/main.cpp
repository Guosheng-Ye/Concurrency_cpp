/***
 * @Author: Oneko
 * @Date: 2024-08-16 14:31:17
 * @LastEditTime: 2024-08-19 15:53:07
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#include "parallel_find.cpp"
#include "parallel_for_each.cpp"
#include "partial_sum.cpp"
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
    std::default_random_engine         e(time(0));
    for (size_t i = 0; i < 10; ++i) {
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
    parallel_for_each(parallelDatas.begin(), parallelDatas.end(), [](int64_t &i) { i *= i; });
    // print_results(parallelDatas);
}

TEST(for_each_test, async_for_eact_test) {
    spdlog::info("async_for_each_test");
    std::vector<int64_t> asyncDatas(testDatas);
    async_for_each(asyncDatas.begin(), asyncDatas.end(), [](int64_t &i) { i *= i; });
}

TEST(find, parallel_find) {
    spdlog::error("parallel find");
    std::vector<int64_t> parallerDatas(testDatas);
    auto                 iter = parallel_find(parallerDatas.begin(), parallerDatas.end(), 87);
    if (iter == parallerDatas.end())
        return;
    else {
        std::cout << "find iter value is: " << *iter << std::endl;
    }
}

TEST(find, async_find) {
    spdlog::error("async find");
    std::atomic<bool>    done(false);
    std::vector<int64_t> asyncDatas(testDatas);
    auto                 iter = async_find(asyncDatas.begin(), asyncDatas.end(), 87, done);
    if (iter == asyncDatas.end())
        return;
    else {
        std::cout << "find iter value is: " << *iter << std::endl;
    }
}

TEST(sum, partial_sum) {
    spdlog::error("partial sum");
    std::vector<int64_t> partialDatas(testDatas);
    for (auto e : partialDatas) {
        std::cout << " " << e;
    }
    std::cout<<std::endl;
    spdlog::info("after partial sum\n");
    paraller_partial_sum(partialDatas.begin(), partialDatas.end());
    for (auto e : partialDatas) {
        std::cout << " " << e;
    }
    spdlog::info("\n");
}
int main() {
    set_random_tests();

    testing::InitGoogleTest();
    ::testing::GTEST_FLAG(color)      = "yes";
    ::testing::GTEST_FLAG(print_time) = true;
    return RUN_ALL_TESTS();
}