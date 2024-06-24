/***
 * @Author: Ye Guosheng
 * @Date: 2024-04-19 15:04:35
 * @LastEditTime: 2024-06-05 13:10:35
 * @LastEditors: Ye Guosheng
 * @Description: 条件变量
 */
#include "spdlog/spdlog.h"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <random>

class AA
{
    std::mutex _m_mtx;
    std::condition_variable _m_cdv;
    std::queue<std::string, std::deque<std::string>> _m_q_qd;

public:
    void incache(int &&num) // produce data by num
    {
        std::lock_guard<std::mutex> lock(_m_mtx); // try lock
        for (size_t i = 0; i < num; ++i)
        {
            static int id = 1;
            std::string message = std::to_string(id++) + " data";
            _m_q_qd.push(message);
        }
        _m_cdv.notify_all(); // 唤醒一个被当前条件变量阻塞的线程
    }

    void outcache()
    {
        std::string mesasge;
        while (true)
        {
            // 互斥锁转换成unique_lock<mutex>
            // {
            // std::cout<<"before_std::unique_lock<std::mutex>
            // lock(_m_mtx);"<<std::endl;
            std::unique_lock<std::mutex> lock(_m_mtx);
            // std::cout<<"after_std::unique_lock<std::mutex>
            // lock(_m_mtx);"<<std::endl;

            // 条件变量存在虚假唤醒
            while (_m_q_qd.empty()) // 队列不为空,不进循环,直接处理数据,必须while
                _m_cdv.wait(lock);  // 等待唤醒信号 -> wait:解锁+阻塞等待被唤醒+加锁

            mesasge = _m_q_qd.front();
            _m_q_qd.pop();
            std::cout << "thread: " << std::this_thread::get_id()
                      << ", message: " << mesasge << std::endl;
            lock.unlock();
            // } // 作用域内,拥有锁
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // process data
        }
    }
};

void test_AA()
{
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

template <class T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() {}

    ThreadSafeQueue(ThreadSafeQueue const &rhs)
    {
        std::lock_guard<std::mutex> lk(rhs._mtx);
        _data_queue = rhs._data_queue;
    }

    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(_mtx);
        _data_queue.push(new_value);
        _data_cond.notify_one();
    }

    bool try_pop(T &value)
    {
        std::lock_guard<std::mutex> lk(_mtx);
        if (_data_queue.empty())
            return false;
        value = _data_queue.front();
        _data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(_mtx);
        if (_data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(_data_queue.front()));
        _data_queue.pop();
        return res;
    }

    void wait_and_pop(T &value)
    {
        std::unique_lock<std::mutex> unlk(_mtx);
        _data_cond.wait(unlk, [this]
                        { return !_data_queue.empty(); });
        value = _data_queue.front();
        _data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(_mtx);
        _data_cond.wait(lk, [this]
                        { return !_data_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(_data_queue.front()));
        _data_queue.pop();
        return res;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lk(_mtx);
        return _data_queue.empty();
    }

private:
    std::mutex _mtx;
    std::queue<T> _data_queue;
    std::condition_variable _data_cond;
};

int main()
{
    spdlog::info("test");
    test_AA();
    return 0;
}