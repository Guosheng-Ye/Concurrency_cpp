/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-18 14:47:36
 * @LastEditTime: 2024-07-20 10:34:09
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include <condition_variable>
#include <memory>
#include <mutex>

template <typename T> class ThreadSafeQueueHT {
public:
    ThreadSafeQueueHT()
        : _upHead(new node)
        , _pTail(_upHead.get()) {} // 初始为虚位结点

    ThreadSafeQueueHT(const ThreadSafeQueueHT &)            = delete;
    ThreadSafeQueueHT &operator=(const ThreadSafeQueueHT &) = delete;

    std::shared_ptr<T> wait_and_pop() {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return old_head->data;
    }
    void wait_and_pop(T &value) {
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }

    std::shared_ptr<T> try_pop() {
        std::unique_lock<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T &value) {
        std::unique_lock<node> const old_head = try_pop_head(value);
        return old_head;
    }

    bool empty() {
        std::lock_guard<std::mutex> head_lcok(_headMtx);
        return (_upHead.get() == get_tail());
    }

    /// @param new_value
    void push(T new_value) {
        // construct new node
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);        // 构建虚位结点
        node *const           new_tail = p.get(); // 更新tail

        std::lock_guard<std::mutex> tail_lock(_tailMtx);
        _pTail->data = new_data;
        _pTail->next = std::move(p);
        _pTail       = new_tail;
        _cvData.notify_one();
    }

private:
    struct node {
        std::shared_ptr<T>    data;
        std::unique_ptr<node> next;
    };

    std::mutex              _headMtx;
    std::mutex              _tailMtx;
    std::unique_ptr<node>   _upHead;
    node                   *_pTail;
    std::condition_variable _cvData;

    node *get_tail() {
        std::lock_guard<std::mutex> tail_lock(_tailMtx);
        return _pTail;
    }

    std::unique_ptr<node> pop_head() {
        std::unique_ptr<node> old_node = std::move(_upHead);
        _upHead                        = std::move(old_node->next);
        return old_node;
    }

    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> head_lock(_headMtx);
        _cvData.wait(head_lock, [&]() { return _upHead.get() != get_tail(); });
        return std::move(head_lock);
    }

    std::unique_ptr<node> wait_pop_head() {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }
    std::unique_ptr<node> wait_pop_head(T &value) {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head() {
        std::lock_guard<std::mutex> head_lock(_headMtx);
        if (_upHead.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T &value) {
        std::lock_guard<std::mutex> head_lock(_headMtx);
        if (_upHead.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        value = std::move(*_upHead->data);
        return pop_head();
    }
};