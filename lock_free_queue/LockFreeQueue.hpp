/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-29 11:16:10
 * @LastEditTime: 2024-07-29 11:16:10
 * @LastEditors: Ye Guosheng
 * @Description: Lock Free Queue
 */

#include <atomic>
#include <memory>

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
    SingleQueue(const SingleQueue &)           = delete;
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
