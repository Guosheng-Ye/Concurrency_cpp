#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include "spdlog/spdlog.h"

class PreSingle
{
public:
    static PreSingle &getSingle()
    {
        static PreSingle single;
        return single;
    }

private:
    PreSingle() {}
    PreSingle(const PreSingle &) = delete;
    PreSingle &operator=(const PreSingle &) = delete;
};

void test_presingle()
{
    // same addr
    std::cout << "s1 addr: " << &PreSingle::getSingle() << std::endl;
    std::cout << "s2 addr: " << &PreSingle::getSingle() << std::endl;
}

class SingleHungry
{
public:
    static SingleHungry *getSingle()
    {
        if (_p_single == nullptr)
        {
            _p_single = new SingleHungry();
        }
        return _p_single;
    }

private:
    SingleHungry() {}
    SingleHungry(const SingleHungry &) = delete;
    SingleHungry &operator=(const SingleHungry &) = delete;

private:
    static SingleHungry *_p_single;
};

SingleHungry *SingleHungry::_p_single = SingleHungry::getSingle();
void thread_fun(int index)
{
    std::cout << "this is thread " << index << std::endl;
    std::cout << "instance  " << SingleHungry::getSingle() << std::endl;
}

void test_singleHungry()
{
    // same addr
    std::cout << "s1 addr is " << SingleHungry::getSingle() << std::endl;
    std::cout << "s2 addr is " << SingleHungry::getSingle() << std::endl;
    for (int i = 0; i < 5; ++i)
    {
        std::thread t(thread_fun, i);
        t.join();
    }
}

class SingleLazy
{
public:
    static SingleLazy *getSingle()
    {
        if (_p_single != nullptr)
        {
            return _p_single;
        }

        _mutex.lock();
        // attention
        if (_p_single != nullptr)
        {
            _mutex.unlock();
            return _p_single;
        }

        _p_single = new SingleLazy(); // delete problem
        _mutex.unlock();
        return _p_single;
    }

private:
    SingleLazy() {}
    SingleLazy(const SingleLazy &) = delete;
    SingleLazy &operator=(const SingleLazy &) = delete;

private:
    static SingleLazy *_p_single;
    static std::mutex _mutex;
};

// init static
SingleLazy *SingleLazy::_p_single = nullptr;
std::mutex SingleLazy::_mutex;

void thread_func_lazy(int i)
{
    std::cout << "this is lazy thread " << i << std::endl;
    std::cout << "inst is " << SingleLazy::getSingle() << std::endl;
}

void test_singlelazy()
{
    for (int i = 0; i < 3; i++)
    {
        std::thread tid(thread_func_lazy, i);
        tid.join();
    }
    // 何时释放new的对象？造成内存泄漏
}

class SingleAuto
{
public:
    ~SingleAuto()
    {
        spdlog::info("~~~");
    }

    static std::shared_ptr<SingleAuto> getSingle()
    {
        if (_sp_single != nullptr)
        {
            return _sp_single;
        }

        _mutex.lock();
        if (_sp_single != nullptr)
        {
            _mutex.unlock();
            return _sp_single;
        }

        _sp_single = std::shared_ptr<SingleAuto>(new SingleAuto);
        _mutex.unlock();
        return _sp_single;
    }

private:
    SingleAuto() {}
    SingleAuto(const SingleAuto &) = delete;
    SingleAuto &operator=(const SingleAuto &) = delete;

private:
    static std::shared_ptr<SingleAuto> _sp_single;
    static std::mutex _mutex;
};

std::shared_ptr<SingleAuto> SingleAuto::_sp_single = nullptr;
std::mutex SingleAuto::_mutex;

void test_singleAuto()
{
    auto sp1 = SingleAuto::getSingle();
    auto sp2 = SingleAuto::getSingle();
    std::cout << "sp1  is  " << sp1 << std::endl;
    std::cout << "sp2  is  " << sp2 << std::endl;
    // 此时存在隐患，可以手动删除裸指针，造成崩溃
    // delete sp1.get();
}

class SingleAutoSafe;
class SingleDeletor
{
    void operator()(SingleAutoSafe *sf)
    {
        spdlog::info("delete operator()");
        delete sf;
    }
};

class SingleAutoSafe
{
public:
    static std::shared_ptr<SingleAutoSafe> getSingle()
    {
        if (_sp_single_safe != nullptr)
        {
            return _sp_single_safe;
        }

        _mutex_.lock();
        if (_sp_single_safe != nullptr)
        {
            _mutex_.unlock();
            return _sp_single_safe;
        }

        _sp_single_safe = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SingleDeletor());
        _mutex_.unlock();
        return _sp_single_safe;
    }

private:
    SingleAutoSafe() {}
    ~SingleAutoSafe()
    {
        spdlog::info("~~~");
    }
    SingleAutoSafe(const SingleAutoSafe &) = delete;
    SingleAutoSafe &operator=(const SingleAutoSafe &) = delete;
    friend class SingleDeletor;

private:
    static std::shared_ptr<SingleAutoSafe> _sp_single_safe;
    static std::mutex _mutex_;
};

std::shared_ptr<SingleAutoSafe> SingleAutoSafe::_sp_single_safe = nullptr;
std::mutex SingleAutoSafe::_mutex_;
void test_singleAutoSafe()
{
    auto sp1 = SingleAuto::getSingle();
    auto sp2 = SingleAuto::getSingle();
    std::cout << "sp1  is  " << sp1 << std::endl;
    std::cout << "sp2  is  " << sp2 << std::endl;
    // 此时存在隐患，可以手动删除裸指针，造成崩溃
    // delete sp1.get();
}

int main()
{
    // test_presingle();
    // test_singleHungry();
    // test_singlelazy();
    // test_singleAuto();
    test_singleAutoSafe();
    return 0;
}