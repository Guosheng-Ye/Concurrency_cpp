
/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-25 09:11:40
 * @LastEditTime: 2024-07-25 09:11:40
 * @LastEditors: Ye Guosheng
 * @Description: 使用引用计数的无锁栈
 */
#include <atomic>
#include <memory>

/*
template <typename T>
class RefStack {

private:
    struct CountNode; // 前置声明引用计数结点

    struct CountNodePtr {
        int        externalCount; // 外部引用计数
        CountNode *ptr;           // CountNode数据结点指针
    };

    struct CountNode {
        std::shared_ptr<T> _spData;         // 数据域智能指针
        std::atomic<int> _atmInternalCount; // 结点内部(减少)引用计数
        CountNodePtr     next;              // 下一个结点

        CountNode(T const &data_)
            : _spData(std::make_shared<T>(data_))
            , _atmInternalCount(0) {}
    };

    std::atomic<CountNodePtr> _atmHead; // 头部结点
public:
    /// @brief
    // 1. create new node
    // 2. set externalCount = 1
    // 3. load head
    // 4. update head
    /// @param data_
    void push(T const &data_) {
        CountNodePtr newNodePtr;
        newNodePtr.ptr           = new CountNode(data_);
        newNodePtr.externalCount = 1;
        newNodePtr.ptr->next     = _atmHead.load();
        while (!_atmHead.compare_exchange_strong(
            newNodePtr.ptr->next, newNodePtr, std::memory_order_release,
            std::memory_order_relaxed))
            ;
    }

    std::shared_ptr<T> pop() {
        CountNodePtr oldHead = _atmHead.load();
        for (;;) {
            increase_head_count(oldHead);
            CountNode *const ptr = oldHead.ptr;
            // 为空直接删除
            if (!ptr) {
                return std::shared_ptr<T>();
            }

            // 本线程如果抢先完成head的更新
            if (_atmHead.compare_exchange_strong(oldHead, ptr->next,
                                                 std::memory_order_relaxed)) {
                // 返回头部数据
                std::shared_ptr<T> res;
                // 交换数据
                res.swap(ptr->_spData);
                // 减少外部引用计数,先统计到目前为止增加了多少外部引用
                int const countIncrease = oldHead.externalCount - 2;
                // 内部引用计数增加
                if (ptr->_atmInternalCount.fetch_add(
                        countIncrease, std::memory_order_release) ==
                    -countIncrease) {
                    delete ptr;
                }
                return res;

            } else if (ptr->_atmInternalCount.fetch_add(
                           -1, std::memory_order_acquire) == 1) {
                //
如果当前线程操作的head结点已经被别的线程更新,则减少内部引用计数
                //
当前线程减少内部引用计数,返回之前值为1说明指针仅被当前线程引用 int internalCount
= ptr->_atmInternalCount.load(std::memory_order_acquire); delete ptr;
            }
        }
    }

    /// @brief 增加头部结点引用计数
    /// @param oldCounterPtr_ CountNodePtr &
    void increase_head_count(CountNodePtr &oldCounterPtr_) {
        CountNodePtr newCountNodePtr;
        do {
            newCountNodePtr = oldCounterPtr_;
            ++newCountNodePtr.externalCount;
            //
循环判断保证head和old_counter相等时做更新,多线程情况保证引用计数原子递增。 }
while (!_atmHead.compare_exchange_strong( oldCounterPtr_, newCountNodePtr,
std::memory_order_acquire, std::memory_order_relaxed));

        // 此处_atmHead的externalCount已被更新
        oldCounterPtr_.externalCount = newCountNodePtr.externalCount;
    }

public:
    RefStack() {
        // init HEAD Node
        CountNodePtr _headNodePtr;
        _headNodePtr.externalCount = 0;
        _headNodePtr.ptr           = nullptr;

        _atmHead.store(_headNodePtr);
    }
    ~RefStack() {
        while (pop())
            ;
    }
};
*/

template <typename T>
class RefStack {

private:
    struct CountNode; // 前置声明引用计数结点

    struct CountNodePtr {
        std::atomic<int> externalCount; // 外部引用计数
        CountNode       *ptr;           // CountNode数据结点指针
    };

    struct CountNode {
        std::shared_ptr<T> _spData;         // 数据域智能指针
        std::atomic<int> _atmInternalCount; // 结点内部(减少)引用计数
        CountNodePtr     next;              // 下一个结点

        CountNode(T const &data_)
            : _spData(std::make_shared<T>(data_))
            , _atmInternalCount(0) {}
    };

    std::atomic<CountNode *> _atmHeadPtr;   // 头部结点指针
    std::atomic<int>         _atmHeadCount; // 头部结点的引用计数

public:
    /// @brief
    // 1. create new node
    // 2. set externalCount = 1
    // 3. load head
    // 4. update head
    /// @param data_
    void push(T const &data_) {
        CountNode *newNodePtr          = new CountNode(data_);
        newNodePtr->next.ptr           = _atmHeadPtr.load();
        newNodePtr->next.externalCount = 1;
        while (!_atmHeadPtr.compare_exchange_strong(
            newNodePtr->next.ptr, newNodePtr, std::memory_order_release,
            std::memory_order_relaxed))
            ;
    }

    std::shared_ptr<T> pop() {
        CountNode *oldHeadPtr   = _atmHeadPtr.load();
        int        oldHeadCount = _atmHeadCount.load();
        for (;;) {
            increase_head_count(oldHeadPtr, oldHeadCount);
            CountNode *const ptr = oldHeadPtr;
            // 为空直接删除
            if (!ptr) {
                return std::shared_ptr<T>();
            }

            // 本线程如果抢先完成head的更新
            if (_atmHeadPtr.compare_exchange_strong(
                    oldHeadPtr, ptr->next.ptr, std::memory_order_relaxed)) {
                _atmHeadCount.compare_exchange_strong(
                    oldHeadCount, ptr->next.externalCount,
                    std::memory_order_relaxed);
                // 返回头部数据
                std::shared_ptr<T> res;
                // 交换数据
                res.swap(ptr->_spData);
                // 减少外部引用计数,先统计到目前为止增加了多少外部引用
                int const countIncrease = oldHeadCount - 2;
                // 内部引用计数增加
                if (ptr->_atmInternalCount.fetch_add(
                        countIncrease, std::memory_order_release) ==
                    -countIncrease) {
                    delete ptr;
                }
                return res;

            } else if (ptr->_atmInternalCount.fetch_add(
                           -1, std::memory_order_acquire) == 1) {
                // 如果当前线程操作的head结点已经被别的线程更新,则减少内部引用计数
                // 当前线程减少内部引用计数,返回之前值为1说明指针仅被当前线程引用
                int internalCount =
                    ptr->_atmInternalCount.load(std::memory_order_acquire);
                delete ptr;
            }
        }
    }

    /// @brief 增加头部结点引用计数
    /// @param oldHeadPtr CountNode*
    /// @param oldHeadCount int
    void increase_head_count(CountNode *&oldHeadPtr, int &oldHeadCount) {
        int newHeadCount;
        do {
            newHeadCount = oldHeadCount + 1;
            // 循环判断保证head和old_counter相等时做更新,多线程情况保证引用计数原子递增。
        } while (!_atmHeadCount.compare_exchange_strong(
            oldHeadCount, newHeadCount, std::memory_order_acquire,
            std::memory_order_relaxed));
        oldHeadCount = newHeadCount;
    }

public:
    RefStack() {
        // init HEAD Node
        _atmHeadPtr.store(nullptr);
        _atmHeadCount.store(0);
    }
    ~RefStack() {
        while (pop())
            ;
    }
};