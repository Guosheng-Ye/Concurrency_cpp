/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-04 14:59:35
 * @LastEditTime: 2024-07-04 15:12:42
 * @LastEditors: Ye Guosheng
 * @Description: CSP
 */
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

template <typename T> class Channel {
public:
    Channel(size_t capacity = 0)
        : _capacity(capacity) {}
    bool send(T value) {
        std::unique_lock<std::mutex> lock(_mtx);
        _condProducer.wait(lock, [this]() {
            return (_capacity == 0 && _queue.empty()) ||
                   _queue.size() < _capacity || _closed;
        });

        if (_closed) return false;

        _queue.push(value);
        _condComsumer.notify_one();
        return true;
    }
    bool receive(T &value) {
        std::unique_lock<std::mutex> lock(_mtx);
        _condComsumer.wait(lock,
                           [this]() { return !_queue.empty() || _closed; });
        if (_closed && _queue.empty()) return false;

        value = _queue.front();
        _queue.pop();
        _condProducer.notify_one();
        return true;
    };
    void close() {
        std::unique_lock<std::mutex> lock(_mtx);
        _closed = true;
        _condComsumer.notify_all();
        _condProducer.notify_all();
    }

private:
    std::queue<T>           _queue;
    std::mutex              _mtx;
    std::condition_variable _condProducer;
    std::condition_variable _condComsumer;
    size_t                  _capacity;
    bool                    _closed = false;
};

void test_channel() {
    Channel<int> ch(10);
    std::thread  producer([&]() {
        for (int i = 0; i < 10; ++i) {
            ch.send(i);
            std::cout << "send: " << i << " " << std::endl;
        }
    });
    std::thread  comsumer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        int value;
        while (ch.receive(value)) {
            std::cout << "receive: " << value << std::endl;
        }
    });

    producer.join();
    comsumer.join();
}

int main() {
    test_channel();
    return 0;
}