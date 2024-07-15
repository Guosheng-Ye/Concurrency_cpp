/*** 
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 15:54:18
 * @LastEditTime: 2024-07-11 17:36:48
 * @LastEditors: Ye Guosheng
 * @Description: 
 */
/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 15:54:18
 * @LastEditTime: 2024-07-11 15:54:43
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <stack>
#include <thread>

struct empty_stack : std::exception {
    const char *what() const throw();
};

/// @brief
/// @tparam T
template <typename T>
class ThreadSafeStackWithLock {
public:
    ThreadSafeStackWithLock(){};
    ThreadSafeStackWithLock(const ThreadSafeStackWithLock &other);
    ThreadSafeStackWithLock &
        operator=(const ThreadSafeStackWithLock &) = delete;
    ~ThreadSafeStackWithLock();

    void               push(T new_value);
    std::shared_ptr<T> pop();
    void               pop(T &value);
    bool               empty() const;

private:
    std::stack<T>      _data;
    mutable std::mutex _mtx;
};

