/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-09 19:14:03
 * @LastEditTime: 2024-07-09 19:19:13
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#include "myclass.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

template <typename T, size_t Cap>
class CircularQueue : private std::allocator<T> {
public:
    CircularQueue()
        : _max_size(Cap + 1)
        , _data(std::allocator<T>::allocate(_max_size))
        , _head(0)
        , _tail(0) {}

    CircularQueue(const CircularQueue &)                     = delete;
    CircularQueue &operator=(const CircularQueue &) volatile = delete;
    CircularQueue &operator=(const CircularQueue &)          = delete;

    ~CircularQueue() {
        std::lock_guard<std::mutex> lock(_mtx);
        while (_head != _tail) {
            std::allocator<T>::destroy(_data + _head);
            _head++;
        }
        std::allocator<T>::deallocate(_data, _max_size);
    }

    /// @brief 插入元素
    /// @tparam ...Args
    /// @param ...args
    /// @return
    template <typename... Args>
    bool emplace(Args &&...args) {
        std::lock_guard<std::mutex> lock(_mtx);

        if ((_tail + 1) % _max_size == _head) {
            std::cout << "queue full" << std::endl;
            return false;
        }

        std::allocator<T>::construct(_data + _tail,
                                     std::forward<Args>(args)...);
        _tail = (_tail + 1) % _max_size;
        return true;
    }

    bool push(const T &value) {
        std::cout << "called push T& value" << std::endl;
        return emplace(value);
    }
    bool push(T &&value) {
        std::cout << "called push T&& value" << std::endl;
        return emplace(std::move(value));
    }

    bool pop(T &value) {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_head == _tail) {
            std::cout << "queue empty! " << std::endl;
            return false;
        }
        value = std::move(_data[_head]);
        _head = (_head + 1) % _max_size;
        return true;
    }

private:
    size_t     _max_size;
    T         *_data;
    std::mutex _mtx;
    size_t     _head = 0;
    size_t     _tail = 0;
};

void test_circular_queue_lock() {

    CircularQueue<MyClass, 5> cq_lk;
    MyClass                   mc1(1);
    MyClass                   mc2(2);
    cq_lk.push(mc1);

    cq_lk.push(std::move(mc2));

    for (int i = 3; i <= 5; ++i) {
        MyClass mc(i);
        auto    res = cq_lk.push(mc);
        if (!res) break;
    }

    cq_lk.push(mc2);
    for (int i = 0; i < 5; ++i) {
        MyClass mc1;
        auto    res = cq_lk.pop(mc1);
        if (!res) break;
        std::cout << "pop success: " << mc1 << std::endl;
    }
    auto res = cq_lk.pop(mc1);
}

// std::atomic<T>::compare_exchange_weak(T &expected, T desired)
// std::atomic<T>::compare_exchange_strong(T &expected, T desired)
// 比较调用者与expected的值, 如果相等,赋值为desired,返回true;
// 否则,expected的值变为调用者的值,并返回false
//
// 它比较原子变量的当前值和一个期望值,当两值相等时,存储所提供的值;
// 当两值不等, 期望值就会被更新为原子变量中的值。

/// @brief 无锁队列
/// @tparam T
/// @tparam Cap
template <typename T, size_t Cap>
class CircularQueueSeq : private std::allocator<T> {
public:
    CircularQueueSeq()
        : _maxSize(Cap + 1)
        , _data(std::allocator<T>::allocate(_maxSize))
        , _atomic_using(false)
        , _head(0)
        , _tail(0) {}
    CircularQueueSeq(const CircularQueueSeq &)                     = delete;
    CircularQueueSeq &operator=(const CircularQueueSeq &) volatile = delete;
    CircularQueueSeq &operator=(const CircularQueueSeq &)          = delete;
    ~CircularQueueSeq() {

        // lock
        bool use_expected = false;
        bool use_desired  = true;

        do {
            use_desired = false;
            use_desired = true;
        } while (
            !_atomic_using.compare_exchange_strong(use_expected, use_desired));

        while (_head != _tail) {
            std::allocator<T>::destroy(_data + _head);
            _head = (_head + 1) % _maxSize;
        }
        std::allocator<T>::deallocate(_data, _maxSize);

        // unlock
        do {
            use_expected = true;
            use_desired  = false;
        } while (
            !_atomic_using.compare_exchange_strong(use_expected, use_desired));
    }

    /// @brief push elem
    /// @tparam ...Args
    /// @param ...args
    /// @return
    template <typename... Args>
    bool emplace(Args &&...args) {
        bool use_expected = false;
        bool use_desired  = true;

        // 多个线程调用emplace,只有一个能进来, 其他的卡在while里
        // _atomic_using = false == use_ecpectec,
        //  then _aotmic_using = desired = ture; return true;
        do {
            bool use_expected = false;
            bool use_desired  = true;
        } while (
            !_atomic_using.compare_exchange_strong(use_expected, use_desired));

        // full
        if ((_tail + 1) % _maxSize == _head) {
            std::cout << "queue full " << std::endl;
            do {
                use_expected = true;
                use_desired  = false;
            } while (!_atomic_using.compare_exchange_strong(use_expected,
                                                            use_desired));
            return false;
        }

        std::allocator<T>::construct(_data + _tail,
                                     std::forward<Args>(args)...);
        _tail = (_tail + 1) % _maxSize;
        do {
            use_expected = true;
            use_desired  = false;
        } while (
            !_atomic_using.compare_exchange_strong(use_expected, use_desired));

        return true;
    }

    /// @brief pop elem
    /// @param value
    /// @return
    bool pop(T &value) {
        bool use_expected = false;
        bool use_desired  = true;

        // lock
        do {
            use_expected = false;
            use_desired  = true;
        } while (
            !_atomic_using.compare_exchange_strong(use_expected, use_desired));

        if (_head == _tail) {
            std::cout << "queue is empty()" << std::endl;
            // unlock
            do {
                use_expected = true;
                use_desired  = false;
            } while (!_atomic_using.compare_exchange_strong(use_expected,
                                                            use_desired));
            return false;
        }

        value = std::move(_data[_tail]);
        _head = (_head + 1) % _maxSize;

        // unlock
        do {
            use_expected = true;
            use_desired  = false;
        } while (
            !_atomic_using.compare_exchange_strong(use_expected, use_desired));
        return true;
    }

    bool push(const T &value) {
        std::cout << "called push const T&" << std::endl;
        return emplace(value);
    }
    bool push(T &&value) {
        std::cout << "called push const T&&" << std::endl;
        return emplace(std::move(value));
    }

private:
    size_t            _maxSize;
    T                *_data;
    std::atomic<bool> _atomic_using;
    size_t            _head = 0;
    size_t            _tail = 0;
};

void test_circular_queue_seq() {
    CircularQueueSeq<MyClass, 3> cq_seq;
    for (int i = 0; i < 4; i++) {
        MyClass mc1(i);
        auto    res = cq_seq.push(mc1);
        if (!res) {
            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        MyClass mc1;
        auto    res = cq_seq.pop(mc1);
        if (!res) {
            break;
        }
        std::cout << "pop success, " << mc1 << std::endl;
    }

    for (int i = 0; i < 4; i++) {
        MyClass mc1(i);
        auto    res = cq_seq.push(mc1);
        if (!res) {
            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        MyClass mc1;
        auto    res = cq_seq.pop(mc1);
        if (!res) {
            break;
        }
        std::cout << "pop success, " << mc1 << std::endl;
    }
}

template <typename T, size_t Cap>
class CircularQueueLightweight : private std::allocator<T> {
public:
    CircularQueueLightweight()
        : _maxSize(Cap + 1)
        , _data(std::allocator<T>::allocate(_maxSize))
        , _head(0)
        , _tail(0)
        , _tailUpdata(0) {}
    CircularQueueLightweight(const CircularQueueLightweight &) = delete;
    CircularQueueLightweight &
        operator=(const CircularQueueLightweight &) volatile = delete;
    CircularQueueLightweight &
        operator=(const CircularQueueLightweight &) = delete;

    ~CircularQueueLightweight() {
        while (_head != _tail) {
            std::allocator<T>::destroy(_data + _head);
            _head++;
        }
        std::allocator<T>::deallocate(_data, _maxSize);
    }

    bool pop(T &value) {
        size_t h;
        do {
            h = _head.load(std::memory_order_relaxed);
            if (h == _tail.load(std::memory_order_acquire))
                return false; // full

            if (h == _tailUpdata.load(std::memory_order_acquire))
                return false; // data not update down

            value = _data[h];
        } while (!_head.compare_exchange_strong(h, (h + 1) % _maxSize,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
        return true;
    }

    bool push(T &value) {
        size_t t;
        do {
            t = _tail.load(std::memory_order_relaxed);
            if ((t + 1) % _maxSize == _head.load(std::memory_order_acquire))
                return false;
            // _data[t] = value;
        } while (!_tail.compare_exchange_strong(t, (t + 1) % _maxSize,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));

        _data[t] = value;

        // attention
        size_t taildown;
        do {
            taildown = t;
        } while (!_tailUpdata.compare_exchange_strong(
            taildown, (taildown + 1) % _maxSize, std::memory_order_release,
            std::memory_order_relaxed));
        return true;
    }

private:
    size_t         _maxSize;
    T             *_data;
    std::atomic<T> _head;
    std::atomic<T> _tail;
    std::atomic<T> _tailUpdata;
};

int main() {
    // test_circular_queue_lock();
    // test_circular_queue_seq();
    return 0;
}