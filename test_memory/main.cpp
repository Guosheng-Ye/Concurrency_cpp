/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 14:29:12
 * @LastEditTime: 2024-07-11 14:30:40
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

std::atomic<bool> x, y;
std::atomic<int>  z;

void write_x() { x.store(true, std::memory_order_release); }
void write_y() { y.store(true, std::memory_order_release); }
void read_x_then_y() {
    while (!x.load(std::memory_order_acquire))
        ;
    if (y.load(std::memory_order_acquire)) ++z;
}
void read_y_then_x() {
    while (!y.load(std::memory_order_acquire))
        ;
    if (x.load(std::memory_order_acquire)) ++z;
}

void test() {
    y = false;
    x = false;
    z = 0;
    std::thread t1(write_x);
    std::thread t2(write_y);
    std::thread t3(read_x_then_y);
    std::thread t4(read_y_then_x);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    assert(z.load() != 0);
    std::cout << "z: " << z << std::endl;
}

void write_x_then_y_fence() {
    x.store(true, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    y.store(true, std::memory_order_relaxed);
}
void read_y_then_x_fence() {
    while (!y.load(std::memory_order_relaxed))
        ;
    std::atomic_thread_fence(std::memory_order_acquire);
    if (x.load(std::memory_order_relaxed)) ++z;
}

void test_fence() {
    y = false;
    x = false;
    z = 0;
    std::thread a(write_x_then_y_fence);
    std::thread b(read_y_then_x_fence);
    a.join();
    b.join();
    assert(z.load() != 0);
    std::cout << "z: " << z << std::endl;
}

int main() {
    // test();
    test_fence();
    return 0;
}