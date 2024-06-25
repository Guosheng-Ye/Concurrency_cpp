
/***
 * @Author: Ye Guosheng
 * @Date: 2024-06-13 17:47:35
 * @LastEditTime: 2024-06-25 13:09:20
 * @LastEditors: Ye Guosheng
 * @Description: future, promise, async
 */

#include "spdlog/spdlog.h"
#include <future>
#include <iostream>
#include <map>
#include <thread>

using payload_type = std::string;
class Connection {
public:
    std::map<int, std::promise<payload_type>> promises;

    std::future<payload_type> get_future(int id) {
        std::promise<payload_type> promise;
        std::future<payload_type>  future = promise.get_future();
        promises[id]                      = std::move(promise);
        return future;
    }

    void send(const payload_type &data, int id) {
        spdlog::info("send data id: {0}, mes:{1}", id, data);
        if (promises.count(id) > 0) {
            promises[id].set_value(data);
        } else {
            std::cerr << "No pormise found for ID: " << id << std::endl;
        }
    }

private:
};

void test_connection() {
    spdlog::info("test connection");
    Connection conn;
    auto       future1 = conn.get_future(1);
    auto       future2 = conn.get_future(2);

    // 模拟发送
    std::thread sender([&]() { conn.send("hello", 1); });

    std::thread sender2([&]() { conn.send("World", 2); });

    sender.join();
    sender2.join();

    spdlog::info("recv 1: {}", future1.get());
    conn.promises.erase(conn.promises.find(1));

    spdlog::info("recv 2: {}", future2.get());
    conn.promises.erase(conn.promises.find(2));
}

payload_type fetchDataFromDB(payload_type query) {

    std::cout << "fetchDataFromDB thread id: " << std::this_thread::get_id()
              << std::endl;
    // 模拟异步任务
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return "data: " + query;
}

void test_fetchDataFromDB() {
    std::cout << "test_fetchDataFromDB thread id: "
              << std::this_thread::get_id() << std::endl;
    // async 异步调用,默认策略为：std::lanunch:async | std::lanuh::deferred
    std::future<payload_type> resultDB =
        std::async(std::launch::async, fetchDataFromDB, "Data");

    spdlog::info("else mession");

    // 获取异步调用结果, 阻塞操作
    payload_type database_data = resultDB.get();
    spdlog::info("data from DB: {}", database_data);
}

/*
std::packaged_task是一个可调用目标，它包装了一个任务，该任务可以在另一个线程上运行。
它可以捕获任务的返回值或异常，并将其存储在std::future对象中，以便以后使用
std::future代表一个异步操作的结果。它可以用于从异步任务中获取返回值或异常。

1.创建一个std::packaged_task对象，该对象包装了要执行的任务。
2.调用std::packaged_task对象的get_future()方法，该方法返回一个与任务关联的std::future对象。
3.在另一个线程上调用std::packaged_task对象的operator()，以执行任务。
4.在需要任务结果的地方，调用与任务关联的std::future对象的get()方法，以获取任务的返回值或异常。
*/
int my_task(int value) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    spdlog::info("my task run 5s ");
    return value + 400;
}

void use_package() {
    spdlog::info("start use_package");
    // 创建包含my_task任务的package_task对象
    // std::packaged_task<int()> task(std::bind(my_task, 600));
    std::packaged_task<int()> task([] { return my_task(600); });
    // 传递一个函数对象或者函数指针，并在调用任务时传递参数。bind 或 lambda。

    // 获取与任务关联的std::future对象
    std::future<int> result = task.get_future();

    // 在其他线程执行任务
    std::thread t(std::move(task));
    t.detach();

    // 等待任务结束并拿到结果
    int value = result.get();
    spdlog::info("the result: {}", value);
}

/*
promise
*/

void set_value(std::promise<int> prom) {

    std::this_thread::sleep_for(std::chrono::seconds(5));
    prom.set_value(10);
    spdlog::info("promise set value done");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    spdlog::info("set value finish");
}

void use_promise() {
    // 创建一个promise对象
    std::promise<int> prom;
    // 获取与pormise绑定的future对象
    std::future<int> future = prom.get_future();
    // 在其他线程设置promise的值
    std::thread t(set_value, std::move(prom));
    // 主线程获取future的值
    spdlog::info("waiting for thread to set value");
    std::cout << "value set by thread " << future.get() << std::endl;

    t.join();
}

/*
Exception
*/
void set_exception(std::promise<void> prom) {
    try {
        throw std::runtime_error("error occurred");
    } catch (...) {
        prom.set_exception(std::current_exception());
    }
}

void use_promise_excetion() {
    std::promise<void> prom;
    std::future<void>  future = prom.get_future();
    std::thread        t(set_exception, std::move(prom));

    try {
        spdlog::info("waiting for thread to set exception");
        future.get();
    } catch (const std::exception &e) {
        std::cout << "Exception set by thread: " << e.what() << std::endl;
    }
}

/*
shared_future
*/
void function(std::promise<int> &&promise) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    promise.set_value(100);
}

void thread_function(std::shared_future<int> future) {

    try {
        int result = future.get();
        std::cout << "Result: " << result << std::endl;
    } catch (const std::future_error &e) {
        std::cout << "Future error: " << e.what() << std::endl;
    }
}

void use_shared_future() {
    std::promise<int>       promise;
    std::shared_future<int> future = promise.get_future();

    std::thread t1(function, std::move(promise)); // promise移动到线程

    // share()获得新的shared_future对象
    std::thread t2(thread_function, future);
    std::thread t3(thread_function, future);

    t1.join();
    t2.join();
    t3.join();
}

int main() {
    // test_connection();
    // std::cout << "main thread id: " << std::this_thread::get_id() <<
    // std::endl;
    // test_fetchDataFromDB();
    // use_package();
    // use_promise();
    // use_promise_excetion();
    use_shared_future();
}