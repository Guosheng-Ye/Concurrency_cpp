/*** 
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 15:59:18
 * @LastEditTime: 2024-07-11 17:37:25
 * @LastEditors: Ye Guosheng
 * @Description: 
 */
/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 15:59:18
 * @LastEditTime: 2024-07-11 15:59:24
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include "thread_safe_stack_lock.hpp"
#include <mutex>

template <typename T>
ThreadSafeStackWithLock<T>::ThreadSafeStackWithLock(
    const ThreadSafeStackWithLock &other) {
    std::lock_guard<std::mutex> lock(_mtx);
    _data = other._data;
}

template <typename T>
void ThreadSafeStackWithLock<T>::push(T new_value) {
    std::lock_guard<std::mutex> lock(_mtx);
    _data.push(std::move(new_value));
}

template <typename T>
std::shared_ptr<T> ThreadSafeStackWithLock<T>::pop() {
    std::lock_guard<std::mutex> lock(_mtx);
    if (_data.empty()) throw empty_stack();
    std::shared_ptr<T> const res(std::make_shared<T>(std::move(_data.top())));
    _data.pop();
    return res;
}

template <typename T>
void ThreadSafeStackWithLock<T>::pop(T &value) {
    std::lock_guard<std::mutex> lock(_mtx);
    if (_data.empty()) throw empty_stack();
    value = std::move(_data.top());
    _data.pop();
}

template <typename T>
bool ThreadSafeStackWithLock<T>::empty() const {
    std::lock_guard<std::mutex> lock(_mtx);
    return _data.empty();
}
