/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-23 09:09:06
 * @LastEditTime: 2024-07-23 09:09:08
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include "LockFreeStack.hpp"
#include "LockFreeStackRefCount.hpp"
#include <cassert>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

void test_lock_free_stack() {
    LockFreeStack<int> lkFreeStack;
    std::set<int>      recvSet;
    std::mutex         setMtx;

    std::thread t1([&]() {
        for (int i = 0; i < 20000; i++) {
            lkFreeStack.push(i);
            std::cout << "push data " << i << " success!" << std::endl;
        }
    });
    std::thread t2([&]() {
        for (int i = 0; i < 10000;) {
            auto head = lkFreeStack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::lock_guard<std::mutex> lock(setMtx);
            recvSet.insert(*head);
            std::cout << "pop data " << *head << " success!" << std::endl;
            i++;
        }
    });
    std::thread t3([&]() {
        for (int i = 0; i < 10000;) {
            auto head = lkFreeStack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::lock_guard<std::mutex> lock(setMtx);
            recvSet.insert(*head);
            std::cout << "pop data " << *head << " success!" << std::endl;
            i++;
        }
    });
    t1.join();
    t2.join();
    t3.join();
    assert(recvSet.size() == 20000);
}

void test_ref_lock_free_stack() {
    RefStack<int> refStack;
    std::set<int> rmv_set;
    std::mutex    set_mtx;
    std::thread   t1([&]() {
        for (int i = 0; i < 20000; i++) {
            refStack.push(i);
            std::cout << "push data " << i << " success!" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    std::thread   t2([&]() {
        for (int i = 0; i < 10000;) {
            auto head = refStack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::lock_guard<std::mutex> lock(set_mtx);
            rmv_set.insert(*head);
            std::cout << "pop data " << *head << " success!" << std::endl;
            i++;
        }
    });
    std::thread   t3([&]() {
        for (int i = 0; i < 10000;) {
            auto head = refStack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::lock_guard<std::mutex> lock(set_mtx);
            rmv_set.insert(*head);
            std::cout << "pop data " << *head << " success!" << std::endl;
            i++;
        }
    });
    t1.join();
    t2.join();
    t3.join();
    assert(rmv_set.size() == 20000);
}

int main() {
    // test_lock_free_stack();
    test_ref_lock_free_stack();
    return 0;
}