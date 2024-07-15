
/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 15:54:08
 * @LastEditTime: 2024-07-11 17:28:59
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include "myclass.hpp"
#include "thread_safe_stack_wait.hpp"
#include "thread_safe_quque_ht.hpp"
#include <thread>

std::mutex mtx_cout;

void print_myclass(std::string consumer, std::shared_ptr<MyClass> data) {
    std::lock_guard<std::mutex> lock(mtx_cout);
    std::cout << consumer << " pop data success, data is " << (*data)
              << std::endl;
}

void test_threedsafe_stack() {

    ThreadSafeStackWaitable<MyClass> stack;

    std::thread consumer1([&]() {
        for (;;) {
            std::shared_ptr<MyClass> data = stack.wait_and_pop();
            print_myclass("comsumer 1", data);
        }
    });

    std::thread consumer2([&]() {
        for (;;) {
            std::shared_ptr<MyClass> data = stack.wait_and_pop();
            print_myclass("comsumer 2", data);
        }
    });

    std::thread producer([&]() {
        for (int i = 0; i < 100; ++i) {
            MyClass mc(i);
            stack.push(std::move(mc));
        }
    });
    consumer1.join();
    consumer2.join();
    producer.join();
}

void test_threadsafe_queue_ht() {
    ThreadSafeQueueHT<MyClass> safe_que;
    std::thread                  consumer1([&]() {
        for (;;) {
            std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
            print_myclass("consumer1", data);
        }
    });

    std::thread consumer2([&]() {
        for (;;) {
            std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
            print_myclass("consumer2", data);
        }
    });

    std::thread producer([&]() {
        for (int i = 0; i < 100; i++) {
            MyClass mc(i);
            safe_que.push(std::move(mc));
        }
    });

    consumer1.join();
    consumer2.join();
    producer.join();
}
int main() {
    // test_threedsafe_stack();
    test_threadsafe_queue_ht();
    return 0;
}