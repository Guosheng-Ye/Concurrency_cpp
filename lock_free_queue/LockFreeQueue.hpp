/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-29 11:16:10
 * @LastEditTime: 2024-07-29 11:16:10
 * @LastEditors: Ye Guosheng
 * @Description: Lock Free Queue
 */
#ifndef LOCKFREEQUEUE_HPP
#define LOCKFREEQUEUE_HPP

#include "spdlog.h"
#include <atomic>
#include <memory>
#include <set>

/// @brief 单生产、消费者的无锁队列实现
/// @tparam DataType
template <typename DataType>
class SingleQueue {
private:
    struct node {
        std::shared_ptr<DataType> _data;
        node                     *_nextNodePtr;

        node()
            : _nextNodePtr(nullptr) {}
    };

    std::atomic<node *> _atmHead;
    std::atomic<node *> _atmTail;

    node *pop_head() {
        node *const oldHead = _atmHead.load(std::memory_order_acquire);
        if (oldHead == _atmTail.load(std::memory_order_acquire)) return nullptr;
        _atmHead.store(oldHead->_nextNodePtr, std::memory_order_release);
        return oldHead;
    }

public:
    SingleQueue()
        : _atmHead(new node)
        , _atmTail(_atmHead.load(std::memory_order_relaxed)) {}

    SingleQueue(const SingleQueue &) = delete;

    SingleQueue operator=(const SingleQueue &) = delete;

    ~SingleQueue() {
        while (node *const old_head =
                   _atmHead.load(std::memory_order_relaxed)) {
            _atmHead.store(old_head->_nextNodePtr, std::memory_order_relaxed);
            delete old_head;
        }
    }

    std::shared_ptr<DataType> pop() {
        node *oldHead = pop_head();
        if (!oldHead) {
            return std::shared_ptr<DataType>();
        }
        std::shared_ptr<DataType> const res(oldHead->_data);
        delete oldHead;
        return res;
    }

    void push(DataType newValue_) {
        std::shared_ptr<DataType> newData(
            std::make_shared<DataType>(newValue_));
        node       *newNode = new node;
        node *const oldTail = _atmTail.load(std::memory_order_acquire);
        oldTail->_data.swap(newData);
        oldTail->_nextNodePtr = newNode;
        _atmTail.store(newNode, std::memory_order_release);
    }
};

/// @brief
/// @tparam DataType
template <typename DataType>
class LockFreeQueue {
private:
    struct node;

    /// @brief 作为副本使用
    struct countedNodePtr {
        countedNodePtr()
            : externalCount(0)
            , nodePtr(nullptr) {}

        int   externalCount;
        node *nodePtr;
    };

    std::atomic<countedNodePtr> _atmHead;
    std::atomic<countedNodePtr> _atmTail;

    struct nodeCounter {
        unsigned internalCount : 30;
        unsigned externalCounters : 2;
        // 一个结点最多被tail和前一个结点的node->next指射,因此外部引用计数最多为2
    };

    struct node {
        std::atomic<DataType *>     _atmData;
        std::atomic<nodeCounter>    _atmNodeCount;
        std::atomic<countedNodePtr> _atmNextCountNodePtr;

        explicit node(int externalCount_ = 2) {
            nodeCounter newNodeCounter;
            newNodeCounter.internalCount    = 0;
            newNodeCounter.externalCounters = externalCount_;
            _atmNodeCount.store(newNodeCounter);

            countedNodePtr nodePtr;
            nodePtr.nodePtr       = nullptr;
            nodePtr.externalCount = 0;

            _atmNextCountNodePtr.store(nodePtr, std::memory_order_acquire);
        }

        void release_ref() {
            spdlog::info("call release ref");
            nodeCounter oldCounter =
                _atmNodeCount.load(std::memory_order_acquire);
            nodeCounter newCounter;

            do {
                newCounter = oldCounter;
                --newCounter.internalCount;
            } while (!_atmNodeCount.compare_exchange_strong(
                oldCounter, newCounter, std::memory_order_acquire,
                std::memory_order_release));

            if (!newCounter.internalCount && !newCounter.externalCounters) {
                delete this;
                spdlog::info("release_ref delete success");
                destruct_count.fetch_add(1);
            }
        }
    };

    static void increase_external_count(std::atomic<countedNodePtr> &counter_,
                                        countedNodePtr &oldCounter) {
        countedNodePtr newCounter;
        do {
            newCounter = oldCounter;
            ++newCounter.externalCount;
        } while (!counter_.compare_exchange_strong(oldCounter, newCounter,
                                                   std::memory_order_acquire,
                                                   std::memory_order_release));
        oldCounter.externalCount = newCounter.externalCount;
    }

    static void free_external_counter(countedNodePtr &oldNodePtr) {
        spdlog::info("call free_external_counter");
        node *const nodePtr       = oldNodePtr.nodePtr;
        int const   countIncrease = oldNodePtr.externalCount - 2;
        nodeCounter oldCounter =
            nodePtr->_atmNodeCount.load(std::memory_order_acquire);
        nodeCounter newCounter;

        do {
            newCounter = oldCounter;
            --newCounter.externalCounters;
            newCounter.internalCount += countIncrease;
        } while (!nodePtr->_atmNodeCount.compare_exchange_strong(
            oldCounter, newCounter, std::memory_order_acquire,
            std::memory_order_release));

        if (!newCounter.internalCount && !newCounter.externalCounters) {
            destruct_count.fetch_add(1);
            spdlog::info("free external counter delete success");
            delete nodePtr;
        }
    }

    void set_new_tail(countedNodePtr &oldTail_, countedNodePtr &newTail_) {
        node *const currentTailPtr = oldTail_.nodePtr;
        while (!_atmTail.compare_exchange_strong(oldTail_, newTail_) &&
               oldTail_.nodePtr == currentTailPtr)
            ;

        if (oldTail_.nodePtr == currentTailPtr) {
            free_external_counter(oldTail_);
        } else {
            currentTailPtr->release_ref();
        }
    }

public:
    LockFreeQueue() {
        countedNodePtr newNext;
        newNext.nodePtr       = new node();
        newNext.externalCount = 1;
        _atmTail.store(newNext);
        _atmHead.store(newNext);
        // spdlog::warn("new_next.ptr is{0}", newNext.nodePtr);
    }

    ~LockFreeQueue() {
        while (pop())
            ;
        countedNodePtr headCountedNode = _atmHead.load();
        delete headCountedNode.nodePtr;
    }

    void push(DataType newValue_) {
        // create new node and new countedNodePtr
        std::unique_ptr<DataType> uniPtrNewData(new DataType(newValue_));
        countedNodePtr            countedNodePtrNewNext;
        countedNodePtrNewNext.nodePtr       = new node;
        countedNodePtrNewNext.externalCount = 1;

        // load tail
        countedNodePtr oldTail = _atmTail.load(std::memory_order_acquire);

        for (;;) {
            // 将tail同步更新给oldTail
            increase_external_count(_atmTail, oldTail);

            DataType *oldData = nullptr;
            if (oldTail.nodePtr->_atmData.compare_exchange_strong(
                    oldData, uniPtrNewData.get())) {

                countedNodePtr oldNext;
                countedNodePtr nowNext =
                    oldNext.nodePtr->_atmNextCountNodePtr.load();

                if (!oldTail.nodePtr->_atmNextCountNodePtr
                         .compare_exchange_strong(oldNext,
                                                  countedNodePtrNewNext)) {
                    delete countedNodePtrNewNext.nodePtr;
                    countedNodePtrNewNext = oldNext;
                }
                set_new_tail(oldTail, countedNodePtrNewNext);
                uniPtrNewData.release();
                break;
            } else {
                countedNodePtr oldNext;
                if (oldTail.nodePtr->_atmNextCountNodePtr
                        .compare_exchange_strong(oldNext,
                                                 countedNodePtrNewNext)) {
                    oldNext                       = countedNodePtrNewNext;
                    countedNodePtrNewNext.nodePtr = new node;
                }
                set_new_tail(oldTail, countedNodePtrNewNext);
            }
        }
        ++construct_count;
    }

    /**
     *
     * @return pop the front data if exists else nullptr
     */
    std::unique_ptr<DataType> pop() {
        countedNodePtr oldHead = _atmHead.load(std::memory_order_acquire);
        for (;;) {
            increase_external_count(_atmHead, oldHead);
            node *const nodePtr = oldHead.nodePtr;

            if (nodePtr == _atmTail.load(std::memory_order_acquire).nodePtr) {
                // 头尾相等，队列为空，减少头部引用计数
                nodePtr->release_ref();
                return std::unique_ptr<DataType>();
            }

            countedNodePtr next = nodePtr->_atmNextCountNodePtr.load();
            if (_atmHead.compare_exchange_strong(oldHead, next)) {
                DataType *const res = nodePtr->_atmData.exchange(nullptr);
                free_external_counter(oldHead);
                return std::unique_ptr<DataType>(res);
            }
            nodePtr->release_ref();
        }
    }

    static std::atomic<int> destruct_count;
    static std::atomic<int> construct_count;
};

template <typename DataType>
std::atomic<int> LockFreeQueue<DataType>::destruct_count = {0};

template <typename DataType>
std::atomic<int> LockFreeQueue<DataType>::construct_count = {0};
#endif // LOCKFREEQUEUE_HPP