
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

class JoinThread {
public:
    explicit JoinThread(std::vector<std::thread> &threads_)
        : _threads(threads_) {}
    ~JoinThread() {
        for (unsigned long i = 0; i < _threads.size(); ++i) {
            if (_threads[i].joinable()) _threads[i].join();
        }
    }

private:
    std::vector<std::thread> &_threads;
};

/// @brief 划分数据区间给n个线程去查找
/// @tparam Iterator
/// @tparam MatchType
/// @param first
/// @param last
/// @param match
/// @return
template <typename Iterator, typename MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match) {

    // find elem if exitst
    struct find_element {

        void operator()(Iterator begin, Iterator end, MatchType match, std::promise<Iterator> *result,
                        std::atomic<bool> *findDoneFlag) {
            try {
                for (; (begin != end) && !findDoneFlag->load(std::memory_order_acquire); ++begin) {
                    if (*begin == match) {
                        result->set_value(begin);
                        findDoneFlag->store(true, std::memory_order_relaxed);
                        return;
                    }
                }
            } catch (const std::exception &e) {
                try {
                    result->set_exception(std::current_exception());
                    findDoneFlag->store(true, std::memory_order_release);

                } catch (...) {
                }
            }
        }
    };

    unsigned long const length          = std::distance(first, last);
    unsigned long const minPerThread    = 25;
    unsigned long const maxThreads      = (length + minPerThread - 1) / minPerThread;
    unsigned long const hardwareThreads = std::thread::hardware_concurrency();
    unsigned long const numThreads      = std::min(hardwareThreads != 0 ? hardwareThreads : 2, maxThreads);
    unsigned long const blockSize       = length / numThreads;

    std::vector<std::thread> threads(numThreads - 1);
    std::atomic<bool>        findDoneFlag(false);
    std::promise<Iterator>   result;
    {
        JoinThread joinThreads(threads);
        Iterator   blockStart = first;
        for (unsigned long index = 0; index < (numThreads - 1); ++index) {
            Iterator blockEnd = blockStart;
            std::advance(blockEnd, blockSize);
            threads[index] = std::thread(find_element(), blockStart, blockEnd, match, &result, &findDoneFlag);
            blockStart     = blockEnd;
        }
        find_element()(blockStart, last, match, &result, &findDoneFlag);
    }
    if (!findDoneFlag.load(std::memory_order_relaxed)) {
        return last;
    }
    return result.get_future().get();
}

template <typename Iterator, typename MatchType>
Iterator async_find(Iterator first, Iterator last, MatchType match, std::atomic<bool> &findDoneFlag) {
    try {
        unsigned long const length       = std::distance(first, last);
        unsigned long const minPerThread = 30;
        if (length < 2 * minPerThread) {
            for (; (first != last) && !findDoneFlag.load(std::memory_order_acquire); ++first) {
                if (*first == match) {
                    findDoneFlag.store(true);
                    return first;
                }
            }
            return last;
        } else {
            Iterator              mid = first + (length / 2);
            std::future<Iterator> asyncResult =
                std::async(&async_find<Iterator, MatchType>, mid, last, match, std::ref(findDoneFlag));
            Iterator const directResult = async_find(first, mid, match, findDoneFlag);
            return (directResult == mid) ? asyncResult.get() : directResult;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        throw;
    }
}