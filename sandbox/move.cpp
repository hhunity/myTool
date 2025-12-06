#include <iostream>
#include <utility>
using namespace std;

struct st {
    st() {}
    st(const st&)  { cout << " -> COPY\n"; }
    st(st&&)       { cout << " -> MOVE\n"; }
};

template<typename T>
T sansyou(T &x)
{
    cout <<"T y = x";
    T y = x;
    cout << "T w = std::move(x)";
    T w = std::move(x);
    cout << "T z = std::forward<T>(x)";
    T z = std::forward<T>(x);
    cout << "return x";

    return std::move(x);
}

template<typename T>
T uhenchi(T &&x)
{
    cout <<"T y = x";
    T y = x;
    // cout << "\nT w = std::move(x)";
    // T w = std::move(x);
    cout << "T z = std::forward<T>(x)";
    T z = std::forward<T>(x);
    cout << "return x";

    return x;
}

template<typename T>
T watashi(T x)
{
    cout <<"T y = x";
    T y = x;
    cout << "T w = std::move(x)";
    T w = std::move(x);
    cout << "T z = std::forward<T>(x)";
    T z = std::forward<T>(x);
    cout << "returnn";

    return x;
}

st uhenchi_st(st&& x)
{
    cout <<"T y = x";
    st y = x;
    cout << "T w = std::move(x)";
    st w = std::move(x);
    cout << "T z = std::forward<T>(x)";
    st z = std::forward<st>(x);
    cout << "returnn";

    return x;
}

st watashi_st(st x)
{
    cout <<"T y = x";
    st y = x;
    cout << "T w = std::move(x)";
    st w = std::move(x);
    cout << "T z = std::forward<T>(x)";
    st z = std::forward<st>(x);
    cout << "returnn";

    return x;
}

int main()
{
    st x;

    cout << "\n=== sansyou(x) ===\n";
    sansyou(x);

    cout << "\n=== uhenchi(x) ===\n";
    uhenchi(x);

    cout << "\n=== uhenchi(std::move(x)) ===\n";
    uhenchi(std::move(x));

    cout << "\n=== watashi(std::move(x)) ===\n";
    watashi(std::move(x));

    cout << "\n=== watashi_st(std::move(x)) ===\n";
    watashi_st(std::move(x));

    cout << "\n=== uhenchi_st(std::move(x)) ===\n";
    uhenchi_st(std::move(x));

    return 0;
}