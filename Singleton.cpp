#include <iostream>
#include <mutex>
using namespace std;

// 线程安全的懒汉模式
class Singleton
{
private:
    Singleton() = default;
    ~Singleton() = default;

public:
    static Singleton &getInstance()
    {
        static Singleton m_instance;
        return m_instance;
    }

    void dosomething()
    {
        cout << "do something" << endl;
    }

    
    Singleton(const Singleton &other) = delete;
    Singleton &operator=(const Singleton &other) = delete;
};

int main()
{
    Singleton& s = Singleton::getInstance();
    s.dosomething();
    return 0;
}
