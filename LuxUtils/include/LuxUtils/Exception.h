/**
 * @file Exception.h
 * @brief 异常是指在程序运行时发生的反常行为，这些行为超出了函数正常功能的范围。
 * 典型的异常包括失去数据库连接以及遇到意外输入等。当程序的某部分检测到一个它无法
 * 处理的问题时，需要用到异常处理。此时，检测出问题的部分应该发出某种信号以表明
 * 程序遇到了故障，无法继续下去了，而且信号的发出方无须知道故障将在何处得到解决。
 * 一旦发出异常信号，检测出问题的部分也就完成了任务。
 *
 *  std::exception: 定义了无参构造函数、拷贝构造函数、拷贝赋值运算符、
 * 一个虚析构函数和一个名为what的无参虚成员，其中，what函数返回一个const char*，
 * 该指针指向一个以null结尾的字符数组，并且确保不会抛出任何异常，该字符串的目的是
 * 提供关于异常的一些文本信息
 * 除析构函数外，其它函数均通过关键字noexcept说明此函数不会抛出异常
 *
 *  C++ 中异常处理包括:
 *      1. throw表达式(throw expression)：异常检测部分使用throw表达式来表示
 *         它遇到了无法处理的问题。throw引发(raise)异常。throw表达式包含关键字throw
 *         和紧随其后的一个表达式，其中表达式的类型就是抛出的异常类型。
 *         throw表达式后面通常紧跟一个分号，从而构成一条表达式语句。
 *         抛出异常将终止当前的函数，并把控制权转移给能处理该异常的代码。
 *      2. try语句块(try block)：异常处理部分使用try语句块处理异常。try语句块
 *         以关键字try开始，并以一个或多个catch子句(catch clause)结束。try语句块
 *         中代码抛出的异常通常会被某个catch子句处理。因为catch子句处理异常，
 *         所以它们也被称作异常处理代码(exception handler)。
 *         catch子句包括三部分：关键字catch、括号内一个(可能未命名的)对象的声明
 *         (称作异常声明，exception
 * declaration)以及一个块。当选中了某个catch子句
 *         处理异常之后，执行与之对应的块。catch一旦完成，程序跳转到try语句块最后一个
 *         catch子句之后的那条语句继续执行。一如往常，try语句块声明的变量在块外部无法访
 *         问，特别是在catch子句内也无法访问。如果一段程序没有try语句块且发生了异常，系
 *         统会调用terminate函数并终止当前程序的执行。
 *
 * @author Tianen Lu
 */

#pragma once

#include <LuxUtils/Types.h>  // string

#include <exception>  // exception

namespace Lux {

/// @brief 继承于 std::exception
class Exception : public std::exception {
private:
    string message_;
    string stack_;

public:
    Exception(string what);
    ~Exception() noexcept override = default;

    // default copy-ctor and operator= are okay.

    const char* what() const noexcept override { return message_.c_str(); }

    const char* stackTrace() const noexcept { return stack_.c_str(); }
};
}  // namespace Lux
