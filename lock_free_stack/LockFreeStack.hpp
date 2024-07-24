/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-23 09:09:23
 * @LastEditTime: 2024-07-23 09:09:23
 * @LastEditors: Ye Guosheng
 * @Description: Lock Free Stack
 */

#include <atomic>
#include <memory>

/// @brief
/// @tparam T stack data type
template <typename T> class LockFreeStack {

private:
    struct node {
        std::shared_ptr<T> _dataSPtr;
        node              *_nextNodePtr;
        node(T const &data_)
            : _dataSPtr(std::make_shared<T>(data_)) {}
    };

public:
    LockFreeStack() {}

    /// @brief
    /// @param value
    void push(T const &value) {
        node *const newNode   = new node(value);
        newNode->_nextNodePtr = _headATM.load();
        while (
            !_headATM.compare_exchange_strong(newNode->_nextNodePtr, newNode))
            ;
    }

    /// @brief
    /// @return std::shared_ptr<T>
    std::shared_ptr<T> pop() {

        // 添加pop线程计数
        ++_theadNumInPop;
        node *oldHead = nullptr;

        do {
            // 加载head,存储到oldHead
            oldHead = _headATM.load();
            if (oldHead == nullptr) {
                --_theadNumInPop;
                return nullptr;
            }
        } while (
            !_headATM.compare_exchange_strong(oldHead, oldHead->_nextNodePtr));
        // 比较更新head为下一个

        std::shared_ptr<T> res;
        if (oldHead) {
            res.swap(oldHead->_dataSPtr); // delete oldHead;
            try_reclaim(oldHead);
        }

        return res;
    }

    /*
    引入延迟删除节点。将本该及时删除的节点放入待删节点。

    1 如果有多个线程同时pop,而且存在一个线程1已经交换取出head数据并更新了head值,
    另一个线程2即将基于旧有的head获取next数据,如果线程1删除了旧有head,线程2就有可能产生崩溃.
    这种情况我们就要将线程1取出的head放入待删除的列表.

    2
    同一时刻仅有一个线程1执行pop函数,不存在其他线程.那么线程1可以将旧head删除,并删除待删列表中的其他节点.

    3
    如果线程1已经将head节点交换弹出,线程2还未执行pop操作,当线程1准备将head删除时发现此时线程2进入执行pop操作,
    那么线程1能将旧head删除,因为线程2读取的head和线程1不同(线程2读取的是线程1交换后新的head值).
    此情形和情形1略有不同,情形1是两个线程同时pop只有一个线程交换成功的情况,情形3是一个线程已经将head交换出,
    准备删除之前发现线程2执行pop进入,所以这种情况下线程1可将head删除,但是线程1不能将待删除列表删除,
    因为有其他线程可能会用到待删除列表中的节点.
    */

    /// @brief 删除old_head或将其放入待删除列表,后续判断是否删除
    // 待删除节点处理:
    // 如果当前线程是唯一一个执行 pop 的线程,将待删除列表中的节点取出,并减少 pop
    // 操作的线程数. 如果减少线程数后,仍然是唯一一个执行 pop
    // 的线程,删除待删除列表中的所有节点. 如果减少线程数后,不再是唯一一个执行
    // pop 的线程,将节点链表重新放回待删除列表中.
    /// @param old_head
    void try_reclaim(node *old_head) {
        // 判断仅有一个线程进入
        // 表示当前线程的这个old_head一定只被一个线程共享,即使后面_theadNumInPop不再为1
        if (_theadNumInPop == 1) {
            node *nodes_to_delete =
                _toBeDeleted.exchange(nullptr); // 当前线程取出待删除列表

            // 更新pop是否仅仅被当前线程唯一调用
            if (!--_theadNumInPop) {
                // 唯一则删除待删除列表
                delete_nodes(nodes_to_delete);
            } else if (nodes_to_delete) {
                // 如果在为nodes_to_delete初始化后_theadNumInPop已经不再是1，表示本线程在执行try_reclaim期间，其他
                // 线程也进入了pop函数，此时，如果另一个线程还未执行到try_reclaim函数，则本线程其实可以直接结束并删除
                // old_head节点的。但是这里是为了防止另外一种情况发生，就是另一个线程可能也已经执行到了try_reclaim函数，
                // 并发现_theadNumInPop不为1(注意，上面的--_theadNumInPop非常的细，这样做的话，如果另外只有一个线程也
                // 同时进入了try_reclaim函数，则_theadNumInPop还是为1，则另一个线程也可以直接删除它自己的old_head，跟
                // 本线程无关)，说明除了本线程已经执行到了这里外，还有另外至少2个线程也在同时执行，然后另一个线程就会执行
                // 下面的else语句，并将线程b的old_head存储到待删除链表中。那么，此时，我们在线程a中还不能删除那些在待删除链表
                // 中的节点，又由于上面我们已经把部分待删除节点与to_be_deleted脱钩了，而现在暂时又不能删除nodes_to_delete，
                // 所以这里还得把这些已经脱钩了的待删除节点给接回去才行，等下次机会合适的时候再一起删除。
                chain_pending_nodes(nodes_to_delete);
            }
            delete old_head;
        } else {
            // 多个线程竞争heead,不删除head,放入待删除列表
            chain_pending_node(old_head);
            --_theadNumInPop;
        }
    }

    /// @brief 将单个结点放入待删除列表
    /// @param node
    void chain_pending_node(node *node) { chain_pending_nodes(node, node); }

    /// @brief 找出被剥离出来的待删除链表的头和尾,然后再插回到toBeDeleted链表里
    /// @param nodes
    void chain_pending_nodes(node *nodes) {
        node *last = nodes;
        while (node *const next = last->_nextNodePtr) {
            last = next;
        }
        chain_pending_nodes(nodes, last); // 链表放入全局待删除列表
    }

    /// @brief 将last节点接到待删除链表的最前面，假设first和last是另外一个链表，
    //  然后这里是把另外一个链表插入到已有链表的前面。
    /// @param first
    /// @param last
    void chain_pending_nodes(node *first, node *last) {
        last->_nextNodePtr =
            _toBeDeleted; // 先将last的next更新为待删除列表的首结点
        // 保证last->next指向正确
        // 将待删除列表的首结点更新为first结点
        while (!_toBeDeleted.compare_exchange_strong(last->_nextNodePtr, first))
            ;
    }

    /// @brief 删除一nodex为首结点的链表
    /// @param nodes
    void delete_nodes(node *nodes) {
        while (nodes) {
            node *next = nodes->_nextNodePtr;
            delete nodes;
            nodes = next;
        }
    }

private:
    LockFreeStack(const LockFreeStack &)                 = delete;
    LockFreeStack      &operator=(const LockFreeStack &) = delete;
    std::atomic<node *> _headATM;
    std::atomic<node *> _toBeDeleted;
    std::atomic<int>    _theadNumInPop;
};