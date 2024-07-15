/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 17:07:26
 * @LastEditTime: 2024-07-11 17:08:36
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <winnt.h>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() {}
    void               push(T new_value);
    void               wait_and_pop(T &value);
    std::shared_ptr<T> wait_and_pop();
    bool               try_pop(T &value);
    std::shared_ptr<T> try_pop();
    bool               empty() const;

private:
    mutable std::mutex             _mtx;
    std::queue<std::shared_ptr<T>> _data_queue;
    std::condition_variable        _cv;
};

template <typename T>
void ThreadSafeQueue<T>::push(T new_value) {
    std::shared_ptr<T>          data(std::make_shared<T>(std::move(new_value)));
    std::lock_guard<std::mutex> lock(_mtx);
    _data_queue.push(data);
    _cv.notify_one();
}

template <typename T>
void ThreadSafeQueue<T>::wait_and_pop(T &value) {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock, [this]() { return !_data_queue.empty(); });
    value = std::move(*_data_queue.front());
    _data_queue.pop();
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::wait_and_pop() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock, [this]() { return !_data_queue.empty(); });
    std::shared_ptr<T> res = _data_queue.front();
    _data_queue.pop();
    return res;
} 

template <typename T>
bool ThreadSafeQueue<T>::try_pop(T &value) {
    std::lock_guard<std::mutex> lock(_mtx);
    if (_data_queue.empty()) return false;
    value = std::move(*_data_queue.front());
    _data_queue.pop();
    return true;
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::try_pop() {
    std::unique_lock<std::mutex> lock(_mtx);
    if (_data_queue.empty()) return false;
    std::shared_ptr<T> res = _data_queue.front();
    _data_queue.pop();
    return res;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(_mtx);
    return _data_queue.empty();
}