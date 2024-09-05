/***
 * @Author: Oneko
 * @Date: 2024-09-03 16:07:54
 * @LastEditTime: 2024-09-03 16:08:01
 * @LastEditors: Ye Guosheng
 * @Description:
 */

#ifndef __JOIN_THREAD_
#define __JOIN_THREAD_
#include <thread>
#include <vector>
class JoinThread {
public:
    explicit JoinThread(std::vector<std::thread> &threads_)
        : _threads(threads_) {}
    ~JoinThread() {
        for (size_t i = 0; i < _threads.size(); ++i) {
            if (_threads[i].joinable()) _threads[i].join();
        }
    }

private:
    std::vector<std::thread> &_threads;
};
#endif //__JOIN_THREAD_