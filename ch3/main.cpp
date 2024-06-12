/***
 * @Author: Ye Guosheng
 * @Date: 2024-03-26 15:34:40
 * @LastEditTime: 2024-03-26 16:00:59
 * @LastEditors: Ye Guosheng
 * @Description: 线程间共享数据
 */
#include <mutex>
#include <thread>
#include <iostream>
#include <stack>
#include <list>
#include <algorithm>
#include <exception>
#include <memory>

std::list<int> lst_Int;
std::mutex mtx_;

void add_list(int new_value)
{
    std::lock_guard<std::mutex> guard(mtx_);
    lst_Int.push_back(new_value);
}
bool list_contains(int value_to_find)
{
    std::lock_guard<std::mutex> guard(mtx_);
    return std::find(lst_Int.begin(), lst_Int.end(), value_to_find) != lst_Int.end();
}

// contruct shared data
class SomeData
{
public:
    void doSth()
    {
        std::cout << "Some Data function doSth()" << std::endl;
    }

private:
    int _m_nVal;
};

class DataWrapper
{
public:
    template <typename Funcion>
    void process_data(Funcion func)
    {
        std::lock_guard<std::mutex> lock(_m_mtxMtx);
        func(_m_data);
    }

private:
    SomeData _m_data;
    std::mutex _m_mtxMtx;
};

SomeData *unprotected;
void malicious_function(SomeData &data)
{
    unprotected = &data;
}

/// @brief test mutex
void test_mutex()
{
    DataWrapper x;
    x.process_data(malicious_function);
    // unsafe,not use mutex
    unprotected->doSth();
}

struct empetyStack : std::exception
{
    const char *what() const noexcept;
};

/// @brief 没有竞争条件的CustomStack
/// @tparam T
template <typename T>
class CustomStack
{
public:
    CustomStack();
    CustomStack(const CustomStack &);
    CustomStack &operator=(const CustomStack &) = delete;
    void push(T new_value);
    std::shared_ptr<T> pop();
    void pop(T &value);
    bool empty() const;
};

/// @brief 线程安全的std::stack<>的封装
/// @tparam T
template <typename T>
class ThreadSafeStack
{
public:
    ThreadSafeStack() {}
    ThreadSafeStack(const ThreadSafeStack &right)
    {
        std::lock_guard<std::mutex> lock(right._m_mtx_m);
        _m_st_data = right._m_st_data;
    }

    ThreadSafeStack &operator=(const ThreadSafeStack &right) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(_m_mtx_m);
        _m_st_data.push(std::move(new_value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(_m_mtx_m);
        if (_m_st_data.empty())
            throw empetyStack();

        std::shared_ptr<T> const res(std::make_shared<T>(_m_st_data.top()));

        _m_st_data.pop();
        return res;
    }
    void pop(T &value)
    {
        std::lock_guard<std::mutex> lock(_m_mtx_m);
        if (_m_st_data.empty())
            throw empetyStack();
        value = _m_st_data.top();
        _m_st_data.pop();
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lokc(_m_mtx_m);
        return _m_st_data.empty();
    }

private:
    std::stack<T> _m_st_data;
    mutable std::mutex _m_mtx_m; // 即使常量函数加const修饰，也可修改
};

// std::lock 可以一次性锁住多个(两个以上)的互斥量，并且无死锁风险
class SomeBigObject
{
};
void swap(SomeBigObject &lhs, SomeBigObject &rhs);

class X
{
public:
    X(SomeBigObject const &sd) : _m_SomBO(sd){};

    friend void swap(X &lhs, X &rhs)
    {
        if (&lhs == &rhs) // same object
        {
            return;
        }
        // std::scoped_lock guard(lhs._m_mtx_M, rhs._m_mtx_M);// C++17
        std::lock(lhs._m_mtx_M, rhs._m_mtx_M);
        std::lock_guard<std::mutex> lock_a(lhs._m_mtx_M, std::adopt_lock);
        std::lock_guard<std::mutex> lock_b(rhs._m_mtx_M, std::adopt_lock);
        // std::adopt_lock:通知lock_guard不要对已锁住的互斥锁重新上锁

        swap(lhs._m_SomBO, rhs._m_SomBO);
    }

private:
    SomeBigObject _m_SomBO;
    std::mutex _m_mtx_M;
};

int main()
{
    test_mutex();
    return 0;
}