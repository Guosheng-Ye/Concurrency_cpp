/***
 * @Author: Ye Guosheng
 * @Date: 2024-04-19 15:04:35
 * @LastEditTime: 2024-06-05 13:10:35
 * @LastEditors: Ye Guosheng
 * @Description: 条件变量
 */
#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <thread>

#include "spdlog/spdlog.h"

class AA {
    std::mutex                                       _m_mtx;
    std::condition_variable                          _m_cdv;
    std::queue<std::string, std::deque<std::string>> _m_q_qd;

public:
    void incache(int &&num) // produce data by num
    {
        std::lock_guard<std::mutex> lock(_m_mtx); // try lock
        for (size_t i = 0; i < num; ++i) {
            static int  id      = 1;
            std::string message = std::to_string(id++) + " data";
            _m_q_qd.push(message);
        }
        _m_cdv.notify_all(); // 唤醒一个被当前条件变量阻塞的线程
    }

    void outcache() {
        std::string mesasge;
        while (true) {
            // 互斥锁转换成unique_lock<mutex>
            // {
            // std::cout<<"before_std::unique_lock<std::mutex>
            // lock(_m_mtx);"<<std::endl;
            std::unique_lock<std::mutex> lock(_m_mtx);
            // std::cout<<"after_std::unique_lock<std::mutex>
            // lock(_m_mtx);"<<std::endl;

            // 条件变量存在虚假唤醒
            while (
                _m_q_qd.empty()) // 队列不为空,不进循环,直接处理数据,必须while
                _m_cdv.wait(
                    lock); // 等待唤醒信号 -> wait:解锁+阻塞等待被唤醒+加锁

            mesasge = _m_q_qd.front();
            _m_q_qd.pop();
            std::cout << "thread: " << std::this_thread::get_id()
                      << ", message: " << mesasge << std::endl;
            lock.unlock();
            // } // 作用域内,拥有锁
            std::this_thread::sleep_for(
                std::chrono::milliseconds(2)); // process data
        }
    }
};

void test_AA() {
    //    AA aa;
    //    std::thread o1(&AA::outcache, &aa);
    //    std::thread o2(&AA::outcache, &aa);
    //    std::thread o3(&AA::outcache, &aa);
    //
    //    std::thread i1(&AA::incache, &aa, 3);
    //    std::this_thread::sleep_for(std::chrono::seconds(2));
    //
    //    std::thread i2(&AA::incache, &aa, 5);
    //    std::this_thread::sleep_for(std::chrono::seconds(3));
    //
    //    std::thread i3(&AA::incache, &aa, 10);
    //    std::this_thread::sleep_for(std::chrono::seconds(3));
    //
    //    std::thread i4(&AA::incache, &aa, 5);
    //    std::this_thread::sleep_for(std::chrono::seconds(3));
    //
    //    o1.join();
    //    o2.join();
    //    o3.join();
    //
    //    i1.join();
    //    i2.join();
    //    i3.join();
    //    i4.join();
}

// 交替打印
int                     num = 1;
std::mutex              num_mtx;
std::condition_variable num_cond_A;
std::condition_variable num_cond_B;

void test_Add() {
    std::thread t1([]() {
        for (;;) {
            std::unique_lock<std::mutex> lock(num_mtx);
            num_cond_A.wait(lock, []() { return num == 1; });

            num++;
            spdlog::info("thread A print 1..");
            num_cond_B.notify_one();
        }
    });

    std::thread t2([]() {
        for (;;) {
            std::unique_lock<std::mutex> lock(num_mtx);
            num_cond_B.wait(lock, []() { return num == 2; });

            num--;
            spdlog::info("Thread 2 print 2...");
            num_cond_A.notify_one();
        }
    });

    t1.join();
    t2.join();
}

template <class T> class ThreadSafeQueue {
public:
    ThreadSafeQueue() {}

    /// @brief const 引用, 在未实现移动构造时可以使用拷贝构造，否则无法使用
    /// @param rhs
    ThreadSafeQueue(ThreadSafeQueue const &rhs) {

        std::lock_guard<std::mutex> lock(rhs._mtx);
        _data_queue = rhs._data_queue;
    }

    void push(T new_value) {

        std::lock_guard<std::mutex> lock(_mtx);
        _data_queue.push(new_value);
        _data_cond.notify_one(); // 唤醒可能等等的其他线程
    }

    /// @brief 队列为空则等待，不为空则pop()
    /// @param value
    void wait_and_pop(T &value) {

        // 注意是unique_lock 而不是lock_guard
        std::unique_lock<std::mutex> un_lock(_mtx);
        _data_cond.wait(un_lock, [this]() { return !_data_queue.empty(); });
        value = _data_queue.front(); // 一次copy消耗
        _data_queue.pop();
    }

    /// @brief
    /// @return std::shared_ptr<T> res
    std::shared_ptr<T> wait_and_pop() {

        std::unique_lock<std::mutex> un_lock(_mtx);
        _data_cond.wait(un_lock, [this]() { return !_data_queue.empty(); });
        std::shared_ptr<T> sp_res(std::make_shared<T>(_data_queue.front()));
        _data_queue.pop();
        return sp_res;
    }

    bool try_pop(T &value) {

        std::lock_guard<std::mutex> lock(_mtx);
        if (_data_queue.empty()) return false;
        value = _data_queue.front();
        _data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() {

        std::lock_guard<std::mutex> lock(_mtx);
        if (_data_queue.empty()) return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(_data_queue.front()));
        _data_queue.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mtx);
        return _data_queue.empty();
    }

private:
    mutable std::mutex      _mtx;
    std::queue<T>           _data_queue;
    std::condition_variable _data_cond;
};

void test_ThreadSafeQueue() {
    ThreadSafeQueue<int> ts_queue;
    std::mutex           mtx_print;

    std::thread producer([&]() {
        for (int i = 0;; ++i) {
            ts_queue.push(i);
            {
                std::lock_guard<std::mutex> print_lock(mtx_print);
                spdlog::info("producer push data: {}", i);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    std::thread consumerA([&]() {
        for (;;) {
            auto data = ts_queue.wait_and_pop();
            {
                std::lock_guard<std::mutex> print_lock(mtx_print);
                spdlog::warn("consumerA pop data: {}", *data);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });

    std::thread consumerB([&]() {
        for (;;) {
            auto data = ts_queue.wait_and_pop();
            {
                std::lock_guard<std::mutex> print_lock(mtx_print);
                spdlog::warn("consumerB pop data: {}", *data);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    producer.join();
    consumerA.join();
    consumerB.join();
}

int main() {
    spdlog::info("test");
    //   test_AA();
    // test_Add();
    test_ThreadSafeQueue();

    return 0;
}
