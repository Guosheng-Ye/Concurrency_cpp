
/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-22 09:09:22
 * @LastEditTime: 2024-07-22 09:09:31
 * @LastEditors: Ye Guosheng
 * @Description: thread safe list
 */
#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
class ThraedSafeList {
public:
    ThraedSafeList() { _lstNodePtr = &_headNode; }
    ~ThraedSafeList() {
        remove_if([](listNode const &) { return true; });
    }
    ThraedSafeList(ThraedSafeList const &)            = delete;
    ThraedSafeList &operator=(ThraedSafeList const &) = delete;

    /// @brief for_each data velue
    /// @tparam Fun
    /// @param f_
    template <typename Fun>
    void for_each(Fun f_) {
        listNode                    *current = &_headNode;
        std::unique_lock<std::mutex> cur_lock(_headNode.nodeMtx);

        while (listNode *const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lock(next->nodeMtx);
            cur_lock.unlock();
            f_(*(next->data)); // for each data
            current  = next;
            cur_lock = std::move(next_lock);
        }
    }

    /// @brief find first value if exist
    /// @tparam Pred
    /// @param p_
    /// @return shared_ptr
    template <typename Pred>
    std::shared_ptr<T> find_first_if(Pred p_) {
        listNode                    *current = &_headNode;
        std::unique_lock<std::mutex> cur_lock(_headNode.nodeMtx);
        while (listNode *const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lock(next->nodeMtx);
            cur_lock.unlock();
            if (p_(*(next->data))) {
                return next->data;
            }
            current  = next;
            cur_lock = std::move(next_lock);
        }
        return std::shared_ptr<T>();
    }

    /// @brief remove node if pred
    /// @tparam Pred
    /// @param p_
    template <typename Pred>
    void remove_if(Pred p_) {
        listNode                    *current = &_headNode;
        std::unique_lock<std::mutex> cur_lock(_headNode.nodeMtx);

        while (listNode *const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lock(next->nodeMtx);

            if (p_(*(next->data))) {
                std::unique_ptr<listNode> oldNode = std::move(current->next);
                current->next                     = std::move(next->next);

                // if delete the last node
                if (current->next == nullptr) {
                    std::lock_guard<std::mutex> lst_lock(_lstNodeMtx);
                    _lstNodePtr = current;
                }
                next_lock.unlock();
            } else {
                cur_lock.unlock();
                current  = next;
                cur_lock = std::move(next_lock);
            }
        }
    }

    /// @brief remove fist node
    /// @tparam Pred
    /// @param p_
    /// @return
    template <typename Pred>
    bool remove_fist(Pred p_) {
        listNode                    *current = &_headNode;
        std::unique_lock<std::mutex> cur_lock(_headNode.nodeMtx);
        while (listNode *const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lock(next->nodeMtx);
            if (p_(*(next->data))) {
                std::unique_ptr<listNode> oldNode = std::move(current->next);
                current->next                     = std::move(next->next);
                // if delete lst node
                if (current->next == nullptr) {
                    std::lock_guard<std::mutex> lst_lock(_lstNodeMtx);
                    _lstNodePtr = current;
                }
                next_lock.unlock();
                return true;
            }
            cur_lock.unlock();
            current  = next;
            cur_lock = std::move(next_lock);
        }
        return false;
    }

    /// @brief push new node by front insert
    /// @param value
    void push_front(T const &value) {
        std::unique_ptr<listNode>   newNode(new listNode(value));
        std::lock_guard<std::mutex> cur_lock(_headNode.nodeMtx);
        newNode->next  = std::move(_headNode.next);
        _headNode.next = std::move(newNode);

        if (_headNode.next->next == nullptr) { // update tail point
            std::lock_guard<std::mutex> lskMtx(_lstNodeMtx);
            _lstNodePtr = _headNode.next.get();
        }
    }

    /// @brief push new node by tail insert
    /// @param value
    void push_back(T const &value) {
        std::unique_ptr<listNode> newNode(new listNode(value));
        std::lock(_lstNodePtr->nodeMtx, _lstNodeMtx);
        std::unique_lock<std::mutex> lock(_lstNodePtr->nodeMtx,
                                          std::adopt_lock);
        std::unique_lock<std::mutex> lst_lock(_lstNodeMtx, std::adopt_lock);

        _lstNodePtr->next = std::move(newNode);
        _lstNodePtr       = _lstNodePtr->next.get();
    }

    template <typename Pred>
    void insert_if(Pred p_, T const &value) {
        listNode                    *current = &_headNode;
        std::unique_lock<std::mutex> cur_lock(_headNode.nodeMtx);
        while (listNode *const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lock(next->nodeMtx);
            if (p_(*(next->data))) { // find location
                std::unique_ptr<listNode> new_node(new listNode(value));

                // insert between [current and next] -> [current  new_node next]
                auto old_next  = std::move(current->next);
                new_node->next = std::move(old_next);
                current->next  = std::move(new_node);
                return;
            }
            cur_lock.unlock();
            current  = next;
            cur_lock = std::move(next_lock);
        }
    }

private:
    struct listNode {
        std::mutex                nodeMtx;
        std::shared_ptr<T>        data;
        std::unique_ptr<listNode> next;

        listNode()
            : next() {}
        listNode(const T &value)
            : data(std::make_unique<T>(value)) {}
    };

    listNode   _headNode;
    listNode  *_lstNodePtr;
    std::mutex _lstNodeMtx;
};
