#include <iostream>
#include <string>
#include <chrono>
#include <format>
#include "../ftxui/ftxui_main.h"

using namespace std;

class myTest
{
public:
myTest() {};
~myTest(){cout << "myTest decontracter" << endl;};
};

template<typename T>
void sansyou(T &x)
{
    cout <<"参照:"<< &x << endl;
}

template<typename T>
void uhenchi(T &&x)
{
    cout <<"右辺値:"<< &x << endl;
}

template<typename T>
void watashi(T x)
{
    cout <<"値渡:"<< &x << endl;
}

int main(int argc, char** argv) {

    int x = 2;

    cout <<"x addr:"<< &x << endl;
    watashi(x);
    sansyou(x);
    uhenchi(std::move(x));

    myTest my {};
    watashi(my);
    sansyou(my);
    uhenchi(std::move(my));

    return 0;
}
