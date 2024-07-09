/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-06 16:05:35
 * @LastEditTime: 2024-07-06 16:05:35
 * @LastEditors: Ye Guosheng
 i* @Description:
 */
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
class SpinkLock {
public:
    void lock() {
        while (_atoFlag.test_and_set(std::memory_order_acquire))
            ;
    }
    void unlock() { _atoFlag.clear(std::memory_order_release); }

private:
    std::atomic_flag _atoFlag = ATOMIC_FLAG_INIT;
};

void test_spink_lock() {
    SpinkLock   spinklock;
    std::thread t1([&spinklock]() {
        spinklock.lock();
        for (int i = 0; i < 10; ++i) {
            std::cout << "*";
        }
        std::cout << std::endl;
        spinklock.unlock();
    });

    std::thread t2([&spinklock]() {
        spinklock.lock();
        for (int i = 0; i < 10; ++i) {
            std::cout << "-";
        }
        std::cout << std::endl;
        spinklock.unlock();
    });

    t1.join();
    t2.join();
}

// std::memory_order_relaxed 宽松内存序
// 不具备同步关系
// 对同一个原子变量，同一线程内有happes-before关系
// 同一原子变量,不同线程不具备happens-before关系
// 多线程不具备happens-bofore关系
std::atomic<bool> x, y;
std::atomic<int>  z;

void write_x_then_y() {
    x.store(true, std::memory_order_relaxed);
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x() {
    while (!y.load(std::memory_order_relaxed)) {
        std::cout << "y load false" << std::endl;
    }
    if (x.load(std::memory_order_relaxed)) {
        ++z;
    }
}

void test_memory_relaxed() {
    x = false;
    y = false;
    z = 0;

    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    t1.join();
    t2.join();
    assert(z.load() != 0);
}

void test_order_relaxed() {
    std::atomic<int> a{0};
    std::vector<int> v3, v4;
    std::thread      t1([&a]() {
        for (int i = 0; i < 10; i += 2) {
            a.store(i, std::memory_order_relaxed);
        }
    });
    std::thread      t2([&a]() {
        for (int i = 1; i < 10; i += 2) {
            a.store(i, std::memory_order_relaxed);
        }
    });
    std::thread      t3([&v3, &a]() {
        for (int i = 0; i < 10; i++) {
            v3.push_back(a.load(std::memory_order_relaxed));
        }
    });
    std::thread      t4([&v4, &a]() {
        for (int i = 0; i < 10; i++) {
            v4.push_back(a.load(std::memory_order_relaxed));
        }
    });
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    for (auto &i : v3) {
        std::cout << i << std::endl;
    }
    std::cout << std::endl;
    for (auto &i : v4) {
        std::cout << i << std::endl;
    }
}

// std::memory_order_seq_cst 全局一致性顺序 最严格内存序
// 所有线程看到的所有操作都有一个一致的顺序, 即使这些操作可能针对不同的变量,
// 运行在不同的线程.
// 性能开销其他模型大
std::atomic<bool> X, Y;
std::atomic<int>  Z;

void write_X_then_Y() {
    X.store(true, std::memory_order_seq_cst);
    Y.store(true, std::memory_order_seq_cst);
}

void read_Y_then_X() {
    while (!Y.load(std::memory_order_seq_cst)) {
        std::cout << "Y LOAD FALSE FIRST! " << std::endl;
    }
    if (X.load(std::memory_order_seq_cst)) {
        ++Z;
    }
}

void test_order_seq_cst() {
    std::thread t1(write_X_then_Y);
    std::thread t2(read_Y_then_X);
    t1.join();
    t2.join();
    assert(Z.load() != 0);
}

// Acquire_release 同步模型
// std::memory_order_acquire->load()
// std::memory_order_release->store()
// std::memory_order_acq_rel->read + modify + write

void test_release_acquire() {
    std::atomic<bool> rx, ry;

    std::thread t1([&]() {
        rx.store(true, std::memory_order_relaxed);
        ry.store(true, std::memory_order_release);
    });

    std::thread t2([&]() {
        while (!ry.load(std::memory_order_acquire))
            ;
        assert(rx.load(std::memory_order_relaxed));
    });
    t1.join();
    t2.join();
}

void test_release_acquire_danger() {
    std::atomic<int> xd{0}, yd{0};
    std::atomic<int> zd;

    std::thread t1([&]() {
        xd.store(1, std::memory_order_release);
        yd.store(1, std::memory_order_release);
    });

    std::thread t2([&]() { yd.store(2, std::memory_order_release); });

    std::thread t3([&]() {
        while (!yd.load(std::memory_order_acquire))
            ;
        assert(xd.load(std::memory_order_acquire) == 1);
    });
}

void test_release_sequence() {
    std::vector<int> data;
    std::atomic<int> flag{0};

    std::thread t1([&]() {
        data.push_back(2);
        flag.store(1, std::memory_order_release);
    });

    std::thread t2([&]() {
        int expected = 1;
        while (!flag.compare_exchange_strong(expected, 2,
                                             std::memory_order_relaxed)) {
            expected = 1;
        }
    });

    std::thread t3([&]() {
        while (flag.load(std::memory_order_acquire) < 2) {
            assert(data.at(0) == 2);
        }
    });

    t1.join();
    t2.join();
    t3.join();
}

// memory_order_consume
void test_consume_dependency() {
    std::atomic<std::string *> ptr;
    int                        data;

    std::thread t1([&]() {
        std::string *p = new std::string("test");
        data           = 10;
        ptr.store(p, std::memory_order_release);
    });

    std::thread t2([&]() {
        std::string *p2;
        while (!(p2 = ptr.load(std::memory_order_consume)))
            ;
        assert(*p2 == "test");
        assert(data = 10);
    });

    t1.join();
    t2.join();

}

int main() {
    // test_spink_lock();
    // test_memory_relaxed();
    // test_order_relaxed();
    // test_order_seq_cst()
    // test_release_acquire();
    // test_release_acquire_danger();
    // test_release_sequence();
    test_consume_dependency();
    return 0;
}