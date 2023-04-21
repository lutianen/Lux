#include <LuxUtils/CurrentThread.h>
#include <LuxUtils/Thread.h>
#include <unistd.h>

#include <cstdio>
#include <ctime>

void mySleep(int seconds) {
    timespec t = {seconds, 0};
    nanosleep(&t, NULL);
}

void threadFunc() { printf("tid=%d\n", Lux::CurrentThread::tid()); }

void threadFunc2(int x) {
    printf("tid=%d, x=%d\n", Lux::CurrentThread::tid(), x);
}

void threadFunc3() {
    printf("tid=%d\n", Lux::CurrentThread::tid());
    mySleep(1);
}

class Foo {
public:
    explicit Foo(double x) : x_(x) {}

    void memberFunc() {
        printf("tid=%d, Foo::x_=%f\n", Lux::CurrentThread::tid(), x_);
    }

    void memberFunc2(const std::string& text) {
        printf("tid=%d, Foo::x_=%f, text=%s\n", Lux::CurrentThread::tid(), x_,
               text.c_str());
    }

private:
    double x_;
};

int main() {
    printf("pid=%d, tid=%d\n", ::getpid(), Lux::CurrentThread::tid());

    Lux::Thread t1(threadFunc);
    t1.start();
    printf("t1.tid=%d\n", t1.tid());
    t1.join();

    Lux::Thread t2(std::bind(threadFunc2, 42),
                   "thread for free function with argument");
    t2.start();
    printf("t2.tid=%d\n", t2.tid());
    t2.join();

    Foo foo(53.159);
    Lux::Thread t3(std::bind(&Foo::memberFunc, &foo),
                   "thread for member function without argument");
    t3.start();
    t3.join();

    Lux::Thread t4(
        std::bind(&Foo::memberFunc2, std::ref(foo), std::string("Lux")));
    t4.start();
    t4.join();

    {
        Lux::Thread t5(threadFunc3);
        t5.start();
    }
    mySleep(2);
    {
        Lux::Thread t6(threadFunc3);
        t6.start();
        mySleep(2);
    }

    sleep(2);
    printf("num of created threads %d\n", Lux::Thread::numCreated());
}
