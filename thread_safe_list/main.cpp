/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-22 09:09:13
 * @LastEditTime: 2024-07-22 09:09:16
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include "ThreadSafeList.hpp"

#include <set>
class MyClass {
public:
    MyClass(int i)
        : _data(i) {}
    friend std::ostream &operator<<(std::ostream &os, const MyClass &mc) {
        os << mc._data;
        return os;
    }
    int get_data() const { return _data; }

private:
    int _data;
};

std::set<int> removeSet;

void test_thread_safe_list() {
    ThraedSafeList<MyClass> threadSafeList;
    std::thread             t1([&]() {
        for (unsigned int i = 0; i < 200; ++i) {
            MyClass mc(i);
            threadSafeList.push_front(mc);
        }
    });

    std::thread t2([&]() {
        for (unsigned i = 0; i < 200;) {
            auto find_res = threadSafeList.find_first_if(
                [&](auto &mc) { return mc.get_data() == i; });
            if (find_res == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            removeSet.insert(i);
            ++i;
        }
    });

    t1.join();
    t2.join();
}

void TestTailPush() {
    ThraedSafeList<MyClass> thread_safe_list;
    for (int i = 0; i < 10; i++) {

        MyClass mc(i);
        thread_safe_list.push_front(mc);
    }

    thread_safe_list.for_each([](const MyClass &mc) {
        std::cout << "for each print " << mc << std::endl;
    });

    for (int i = 10; i < 20; i++) {
        MyClass mc(i);
        thread_safe_list.push_back(mc);
    }

    thread_safe_list.for_each([](const MyClass &mc) {
        std::cout << "for each print " << mc << std::endl;
    });

    auto find_data = thread_safe_list.find_first_if(
        [](const MyClass &mc) { return (mc.get_data() == 19); });

    if (find_data) {
        std::cout << "find_data is " << find_data->get_data() << std::endl;
    }

    thread_safe_list.insert_if(
        [](const MyClass &mc) { return (mc.get_data() == 19); }, 20);

    thread_safe_list.for_each([](const MyClass &mc) {
        std::cout << "for each print " << mc << std::endl;
    });
}

void MultiThreadPush() {
    ThraedSafeList<MyClass> thread_safe_list;

    std::thread t1([&]() {
        for (int i = 0; i < 20000; i++) {
            MyClass mc(i);
            thread_safe_list.push_front(mc);
            std::cout << "push front " << i << " success" << std::endl;
        }
    });

    std::thread t2([&]() {
        for (int i = 20000; i < 40000; i++) {
            MyClass mc(i);
            thread_safe_list.push_back(mc);
            std::cout << "push back " << i << " success" << std::endl;
        }
    });

    std::thread t3([&]() {
        for (int i = 0; i < 40000;) {
            bool rmv_res = thread_safe_list.remove_fist(
                [&](const MyClass &mc) { return mc.get_data() == i; });

            if (!rmv_res) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            i++;
        }
    });

    t1.join();
    t2.join();
    t3.join();

    std::cout << "begin for each print...." << std::endl;
    thread_safe_list.for_each([](const MyClass &mc) {
        std::cout << "for each print " << mc << std::endl;
    });
    std::cout << "end for each print...." << std::endl;
}

int main() {
    // test_thread_safe_list();
    // for (auto &elem : removeSet) {
    //     std::cout << elem << " " << std::endl;
    // }
    // std::cout << std::endl;
    // TestTailPush();
    MultiThreadPush();
    return 0;
}