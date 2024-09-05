#ifndef __THREAD_SAFE_QUEUE__
#define __THREAD_SAFE_QUEUE__
#include <atomic>
#include <memory>
#include <mutex>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue()
        : _unipHead(new node)
        , _nodeTail(_unipHead.get()) {}
    ~ThreadSafeQueue() {}
    ThreadSafeQueue(const ThreadSafeQueue &)            = delete;
    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

    void exit() {
        _stopFlag.store(true, std::memory_order_release);
        _condvData.notify_all();
    }

    bool wait_and_pop_timeout(T &value_) {
        std::unique_lock<std::mutex> headLock(_mtxHead);
        auto                         res = _condvData.wait_for(headLock, std::chrono::milliseconds(100),
                                                               [&]() { return _unipHead.get() != get_tail() || _stopFlag.load() == true; });
        if (!res) return false;
        if (_stopFlag.load()) return false;
        value_    = std::move(*_unipHead->_data);
        _unipHead = std::move(_unipHead->_next);
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_ptr<node> const oldHead = wait_pop_head();
        if (oldHead == nullptr) return nullptr;
        return oldHead->_data;
    }

    bool wait_and_pop(T &value_) {
        std::unique_ptr<node> const oldHead = wait_and_pop(value_);
        if (oldHead == nullptr) return false;
        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> oldHead = try_pop_head();
        return oldHead ? oldHead->data : std::shared_ptr<T>();
    }

    bool try_pop(T &value_) {
        std::unique_ptr<node> const oldHead = try_pop_head(value_);
        if (oldHead) return true;
        return false;
    }

    bool empty() {
        std::lock_guard<std::mutex> headLock(_mtxHead);
        return (_unipHead.get() == get_tail());
    }

    /// @brief push node
    /// @param newValue_
    void push(T newValue_) {
        std::shared_ptr<T>    newData(std::make_shared<T>(std::move(newValue_)));
        std::unique_ptr<node> newNode(new node);
        {
            std::lock_guard<std::mutex> tailLock(_mtxTail);
            _nodeTail->_data    = newData;
            node *const newTail = newNode.get();
            newTail->_prev      = _nodeTail;
            _nodeTail->_next    = std::move(newNode);
            _nodeTail           = newTail;
        }
        _condvData.notify_one();
    }

    /// @brief try pop tail
    /// @param value_
    /// @return
    bool try_steal(T &value_) {
        std::unique_lock<std::mutex> tailLock(_mtxTail, std::defer_lock);
        std::unique_lock<std::mutex> headLock(_mtxHead, std::defer_lock);
        std::lock(tailLock, headLock);
        if (_unipHead.get() == get_tail()) {
            return false;
        }
        node *prevNode   = _nodeTail->_prev;
        value_           = std::move(*(prevNode->_data));
        _nodeTail        = prevNode;
        _nodeTail->_next = nullptr;
        return true;
    }

private:
    struct node {
        std::shared_ptr<T>    _data;
        std::unique_ptr<node> _next;
        node                 *_prev;
    };

    /// @brief  get tail ptr
    /// @return node *_nodeTail;
    node *get_tail() {
        std::lock_guard<std::mutex> lock(_mtxTail);
        return _nodeTail;
    }

    /// @brief pop head unip
    /// @return  std::unique_ptr<node> oldHead
    std::unique_ptr<node> pop_head() {
        std::unique_ptr<node> oldHead = std::move(_unipHead);
        _unipHead                     = std::move(oldHead->_next);
        return oldHead;
    }
    /// @brief wait data
    /// @return std::move(headLock)
    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> headLock(_mtxHead);
        _condvData.wait(headLock, [&]() { return _unipHead.get() != get_tail() || _stopFlag.load() == true; });
        return std::move(headLock);
    }

    /// @brief wait and pop head node
    /// @return pop_head() or nullptr
    std::unique_ptr<node> wait_pop_head() {
        std::unique_lock<std::mutex> headLock(wait_for_data());
        if (_stopFlag.load()) return nullptr;
        return pop_head();
    }

    /// @brief wait and pop head node
    /// @param value_
    /// @return pop_head() or nullptr
    std::unique_ptr<node> wait_pop_head(T &value_) {
        std::unique_lock<std::mutex> headLock(wait_for_data());
        if (_stopFlag.load()) return nullptr;
        value_ = std::move(*_unipHead->_data);
        return pop_head();
    }

    /// @brief try to pop
    /// @return  pop_head() or std::unique_ptr<node>()
    std::unique_ptr<node> try_pop_head() {
        std::lock_guard<std::mutex> headLock(_mtxHead);
        if (_unipHead.get() == get_tail()) {
            return std::unique_ptr<node>(); // queue is empty
        }
        return pop_head();
    }

    /// @brief try to pop
    /// @param value
    /// @return pop_head() or std::unique_ptr<node>()
    std::unique_ptr<node> try_pop_head(T &value) {
        std::lock_guard<std::mutex> headLock(_mtxHead);
        if (_unipHead.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        value = std::move(*_unipHead->_data);
        return pop_head();
    }

private:
    std::mutex              _mtxHead;
    std::mutex              _mtxTail;
    std::unique_ptr<node>   _unipHead;
    node                   *_nodeTail;
    std::condition_variable _condvData;
    std::atomic_bool        _stopFlag;
};

#endif //__THREAD_SAFE_QUEUE__