#include <iostream>
#include <format>
#include <thread>
#include <vector>
using namespace std;

vector<function<void()> > th_pool;

void add_task(function<void()> fn)
{
    th_pool.push_back(std::move(fn));
}

void run_tasks() {
    for (auto& fn : th_pool) fn();   // 後で実行
}

int main()
{
    cout << "sandbox mian " << endl;
    
    {
        int a = 10;
        int &sa = a;

        int*p = nullptr;
        int*&sp = p;

        p = &a;

        // format("a:{},sa:{},p:{},sp:{}",a,sa,p,sp);
        
        add_task([&]{//<-=が正解
            cout << format("a:{},sa:{},p:{},sp:{},*p:{},*sp:{}",a,sa,static_cast<void*>(p),static_cast<void*>(sp),*p,*sp) << endl;
        });

    }

    //参照でキャプチャしてると、スコープ外に出ると、壊れている可能性あり。=する必要がある。
    run_tasks();

    return 0;
}