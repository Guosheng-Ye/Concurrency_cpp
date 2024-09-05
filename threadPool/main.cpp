/***
 * @Author: Oneko
 * @Date: 2024-09-04 16:07:40
 * @LastEditTime: 2024-09-05 08:55:22
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include "parallel_foreach.hpp"
#include "parallen_thread_pool.hpp"
#include "quicksort.hpp"
#include "spdlog/spdlog.h"
#include "steal_thread_pool.hpp"
#include <iostream>
#include <list>
#include <random>

void test_simple_thread() {
    spdlog::info("simpel_thread:");
    std::vector<int> nvec;
    for (int i = 0; i < 26; ++i) {
        nvec.push_back(i);
    }
    simple_foreach(nvec.begin(), nvec.end(), [](int &i) { i *= i; });
    for (auto &e : nvec) {
        std::cout << e << " ";
    }
    std::cout << std::endl;
}

void test_future_thread() {
    spdlog::info("test_future_thread:");
    std::vector<int> nvec;
    for (int i = 0; i < 26; ++i) {
        nvec.push_back(i);
    }
    future_foreach(nvec.begin(), nvec.end(), [](int &i) { i *= i; });
    for (auto &e : nvec) {
        std::cout << e << " ";
    }
    std::cout << std::endl;
}
void test_notify_thread() {
    spdlog::info("test_notify_thread:");
    std::vector<int> nvec;
    for (int i = 0; i < 26; ++i) {
        nvec.push_back(i);
    }
    notify_foreach(nvec.begin(), nvec.end(), [](int &i) { i *= i; });
    for (auto &e : nvec) {
        std::cout << e << " ";
    }
    std::cout << std::endl;
}

void test_parallen_thread() {
    spdlog::info("test_parallen_thread:");
    unsigned                           seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::uniform_int_distribution<int> u(-1000, 1000);
    std::default_random_engine         e(seed);

    std::list<int> nList;
    for (size_t i = 0; i < 30; ++i)
        nList.push_back(u(e));
    auto sortList = parallen_pool_thread_sort<int>(nList);

    for (auto &e : sortList) {
        std::cout << e << " ";
    }
    std::cout << std::endl;
}

void test_steal_thread() {
    auto                               seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::uniform_int_distribution<int> u(-1000, 1000);
    std::default_random_engine         e(seed);
    std::list<int>                     nList{0};
    for (size_t i = 0; i < 30; ++i)
        nList.push_back(u(e));

    auto sortList = steal_pool_thread_sort<int>(nList);
    for (auto &e : sortList) {
        std::cout << e << " ";
    }
    std::cout << std::endl;
}
int main() {
    // test_simple_thread();
    // test_future_thread();
    // test_notify_thread();
    // test_parallen_thread();
    test_steal_thread();
    return 0;
}