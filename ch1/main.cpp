/*** 
 * @Author: Ye Guosheng
 * @Date: 2024-03-21 14:24:33
 * @LastEditTime: 2024-06-05 09:12:15
 * @LastEditors: Ye Guosheng
 * @Description: 线程demo
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

class ThreadTest

{
public:
    ThreadTest(int &num) : _m_nNum(num){};
    ~ThreadTest(){};

public:
    void operator()() const
    {
        std::cout << "thread of ThreadTest" << std::endl;
        for (int i = 0; i < 200; ++i)
        {
            std::cout << "class Thread-----i: " << i << " " << _m_nNum << std::endl;
        }
    };
    int getNum()
    {
        return this->_m_nNum;
    }

private:
    int &_m_nNum;
};

void f()
{
    int some_state = 999999;
    ThreadTest ttt(some_state);
    std::thread my_th(ttt);
    try
    {
        for (int i = 0; i < 10; ++i)
        {
            std::cout << "thread f()___j___: " << i << " " << ttt.getNum() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        my_th.join();
    }
    my_th.join();
};

void thread_test()
{
    std::cout << "thread" << std::endl;
}


int main()
{
    std::thread t(thread_test);
    t.join();

    int num = 1000;
    ThreadTest tt(num);
    std::thread my_thread(tt);
    my_thread.join();
    // my_thread.detach();

    f();

    return 0;
}
