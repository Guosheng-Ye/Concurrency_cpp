/***
 * @Author: Ye Guosheng
 * @Date: 2024-07-11 17:25:59
 * @LastEditTime: 2024-07-11 17:26:03
 * @LastEditors: Ye Guosheng
 * @Description:
 */
#include <iostream>
class MyClass {
public:
    MyClass(int data)
        : _data(data) {}
    MyClass(const MyClass &rhs)
        : _data(rhs._data) {}
    MyClass(MyClass &&rhs)
        : _data(rhs._data) {}

    friend std::ostream &operator<<(std::ostream &os, const MyClass &rhs) {
        os << rhs._data;
        return os;
    }

private:
    int _data;
};
