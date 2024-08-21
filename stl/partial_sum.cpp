/***
 * @Author: Oneko
 * @Date: 2024-08-19 16:37:41
 * @LastEditTime: 2024-08-20 09:19:38
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include <algorithm>
#include <atomic>
#include <future>
#include <numeric>
#include <thread>
#include <vector>

class Join_Thread {
public:
    explicit Join_Thread(std::vector<std::thread> &threads_)
        : _threads(threads_) {}
    ~Join_Thread() {
        for (unsigned long i = 0; i < _threads.size(); ++i) {
            if (_threads[i].joinable()) _threads[i].join();
        }
    }

private:
    std::vector<std::thread> &_threads;
};

/// @brief [1,2,3,4,5,6,7,8,9]
/// split as         : [1,2,3]    [4,5,6]        [7,8,9]
/// Add in group     : [1,3,6]    [4,9,15]       [7,15,24]
/// Add beteeen group: [1,3,6] 6->[10,15,21] 21->[28,36,45]
/// res              : [1,3,6,10,15,21,28,36,55]
/// @tparam Iterator
/// @param first
/// @param last
template <typename Iterator>
void paraller_partial_sum(Iterator first, Iterator last) {
    using valueType = typename Iterator::value_type;

    struct processChunk {

        void operator()(Iterator begin, Iterator last, std::future<valueType> *previousEndValuesFuture_,
                        std::promise<valueType> *endValuesPro_) {
            try {

                Iterator end = last;
                ++end;
                std::partial_sum(begin, end, begin); // 计算当前块的部分和

                if (previousEndValuesFuture_) {                         // 当前块不是第一个块
                    valueType addEnd = previousEndValuesFuture_->get(); // 获取前一个块的和
                    *last += addEnd; // 当前块的最后一个元素添加前一个块的和addEnd（**先添加，及时给下一块使用**）
                    if (endValuesPro_) {                 // 需要将当前块的值传给下一个块
                        endValuesPro_->set_value(*last); // 将endValuesPro_设置为当前块的最后一个元素的值
                    }
                    std::for_each(begin, last, [addEnd](valueType &item) {
                        item += addEnd; // 当前块的所有元素（除了最后一个）加上前一个块的和addEnd
                    });
                } else if (endValuesPro_) { // 无前一块，直接将当前块的最后一个元素设为endValuesPro_的值
                    endValuesPro_->set_value(*last); // 提醒 future
                }

            } catch (...) {
                if (endValuesPro_) {
                    endValuesPro_->set_exception(std::current_exception());
                } else {
                    throw;
                }
            }
        }
    };

    unsigned long length = std::distance(first, last);
    if (!length) return;

    unsigned long minPerThread   = 30;
    unsigned long maxThreads     = (length + minPerThread - 1) / minPerThread;
    unsigned long hardwareThread = std::thread::hardware_concurrency();
    unsigned long numThreads     = std::min(hardwareThread != 0 ? hardwareThread : 2, maxThreads);
    unsigned long blockSize      = length / numThreads;

    /*
    1 2 3,  4  5  6,  7  8
    1 3 6  10 15 21  28 36

    t1: 1 3 6                           p1=6
    t2: (4 9 15).add(6)=>(10,15,21); f2=p1=6, p2=21
    t:  (7 15).add(21) =>(28,36)   ; f3=p2=21
    */
    std::vector<std::thread>             threads(numThreads - 1);
    std::vector<std::promise<valueType>> endValuesPro(numThreads - 1); // 前一个线程的累加结果
    std::vector<std::future<valueType>>  previousEndValuesFuture;      // future获取前一个线程的promise
    previousEndValuesFuture.reserve(numThreads - 1);
    Join_Thread joinThread(threads);
    Iterator    blockStart = first;

    for (unsigned long index = 0; index < numThreads - 1; ++index) {
        Iterator blockEnd = blockStart;
        std::advance(blockEnd, blockSize - 1); // 17

        // 不是第一个块线程，传入前一个块的部分和结果，并存储当前块的部分和结果。
        threads[index] = std::thread(processChunk(), blockStart, blockEnd,
                                     (index != 0) ? &previousEndValuesFuture[index - 1] : 0, &endValuesPro[index]);
        blockStart     = blockEnd;
        ++blockStart;

        // 当前块的部分和结果的future对象存入previousEndValuesFuture中，以便传递给下一个线程。
        previousEndValuesFuture.push_back(endValuesPro[index].get_future());
    }

    // 处理最后一块数据
    Iterator finalElement = blockStart;
    // 21
    std::advance(finalElement, std::distance(blockStart, last) - 1);
    // 22
    processChunk()(blockStart, finalElement, (numThreads > 1) ? &previousEndValuesFuture.back() : 0, 0);
}