/***
 * @Author: Ye Guosheng
 * @Date: 2024-04-19 15:04:35
 * @LastEditTime: 2024-05-06 16:33:03
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <deque>
#include <chrono>

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
            // std::cout<<"before_std::unique_lock<std::mutex> lock(_m_mtx);"<<std::endl;
            std::unique_lock<std::mutex> lock(_m_mtx);
            // std::cout<<"after_std::unique_lock<std::mutex> lock(_m_mtx);"<<std::endl;

            // 条件变量存在虚假唤醒
            while (_m_q_qd.empty()) // 队列不为空,不进循环,直接处理数据,必须while
                _m_cdv.wait(lock);  // 等待唤醒信号 -> wait:解锁+阻塞等待被唤醒+加锁
            //
            mesasge = _m_q_qd.front();
            _m_q_qd.pop();
            std::cout << "thread: " << std::this_thread::get_id() << ", message: " << mesasge << std::endl;
            lock.unlock();
            // } // 作用域内,拥有锁
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // process data
        }
    }
};

int main()
{
    AA aa;
    std::thread o1(&AA::outcache, &aa);
    std::thread o2(&AA::outcache, &aa);
    std::thread o3(&AA::outcache, &aa);

    std::thread i1(&AA::incache, &aa, 3);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::thread i2(&AA::incache, &aa, 5);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::thread i3(&AA::incache, &aa, 10);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::thread i4(&AA::incache, &aa, 5);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    o1.join();
    o2.join();
    o3.join();

    i1.join();
    i2.join();
    i3.join();
    i4.join();

    return 0;
}