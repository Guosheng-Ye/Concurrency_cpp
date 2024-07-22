/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-20 15:36:32
 * @LastEditTime: 2024-07-22 09:03:14
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include "ThreadSafeLookupTable.hpp"
#include <iostream>
#include <set>
class MyClass {
public:
    MyClass(int i)
        : _data(i) {}
    friend std::ostream &operator<<(std::ostream &os, const MyClass &mc) {
        os << mc._data;
        return os;
    }

private:
    int _data;
};

void test_thread_safe_hash() {
    std::set<int>                                        removeSet;
    ThreadSafeLookUpTable<int, std::shared_ptr<MyClass>> table;

    std::thread t1([&]() { // add value 0-99
        for (int i = 0; i < 100; i++) {
            auto class_ptr = std::make_shared<MyClass>(i);
            table.add_or_update(i, class_ptr);
        }
    });

    std::thread t2([&]() { // remove value 0-99
        for (int i = 0; i < 100;) {
            auto find_res = table.value_for(i, nullptr);
            if (find_res) {
                table.remove_mapping(i);
                removeSet.insert(i);
                i++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t3([&]() {
        for (int i = 100; i < 200; i++) {
            auto class_ptr = std::make_shared<MyClass>(i);
            table.add_or_update(i, class_ptr);
        }
    });
    t1.join();
    t2.join();
    t3.join();
    for (auto &i : removeSet) {
        std::cout << "remove data is " << i << std::endl;
    }
    auto copy_map = table.get_map();
    for (auto &i : copy_map) {
        std::cout << "copy data is " << *(i.second) << std::endl;
    }
}
int main() {
    test_thread_safe_hash();
    return 0;
}