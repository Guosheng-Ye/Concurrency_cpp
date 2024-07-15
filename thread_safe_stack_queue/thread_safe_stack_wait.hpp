/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 16:38:46
 * @LastEditTime: 2024-07-11 16:38:50
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include <condition_variable>
#include <mutex>
#include <stack>

template <typename T>
class ThreadSafeStackWaitable {
public:
    ThreadSafeStackWaitable() {}
    ThreadSafeStackWaitable(const ThreadSafeStackWaitable &other);
    ThreadSafeStackWaitable &
        operator=(const ThreadSafeStackWaitable &) = delete;

    void               push(T new_value);
    std::shared_ptr<T> wait_and_pop();
    void               wait_and_pop(T &value);

    bool empty() const;

    bool               try_pop(T &value);
    std::shared_ptr<T> try_pop();

private:
    std::stack<T>           _data;
    mutable std::mutex      _mtx;
    std::condition_variable _cv;
};


template <typename T>
ThreadSafeStackWaitable<T>::ThreadSafeStackWaitable(
    const ThreadSafeStackWaitable &other) {
    std::lock_guard<std::mutex> lock(other._mtx);
    _data = other._data;
}

template <typename T>
void ThreadSafeStackWaitable<T>::push(T new_value) {
    std::lock_guard<std::mutex> lock(_mtx);
    _data.push(std::move(new_value));
    _cv.notify_one();
}

template <typename T>
std::shared_ptr<T> ThreadSafeStackWaitable<T>::wait_and_pop() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock, [this]() {
        if (_data.empty()) return false;
        return true;
    });

    std::shared_ptr<T> const res(std::make_shared<T>(std::move(_data.top())));
    _data.pop();
    return res;
}

template <typename T>
void ThreadSafeStackWaitable<T>::wait_and_pop(T &value) {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock, [this]() {
        if (_data.empty()) return false;
        return true;
    });
    value = std::move(_data.top());
    _data.pop();
}

template <typename T>
bool ThreadSafeStackWaitable<T>::empty() const {
    std::lock_guard<std::mutex> lock(_mtx);
    return _data.empty();
}

template <typename T>
bool ThreadSafeStackWaitable<T>::try_pop(T &value) {
    std::lock_guard<std::mutex> lock(_mtx);
    if (_data.empty()) return false;
    value = std::move(_data.top());
    _data.pop();
    return true;
}

template <typename T>
std::shared_ptr<T> ThreadSafeStackWaitable<T>::try_pop() {
    std::lock_guard<std::mutex> lock(_mtx);
    if (_data.empty()) return std::shared_ptr<T>();
    std::shared_ptr<T> res(std::make_shared<T>(std::move(_data.top())));
    _data.pop();
    return res;
}