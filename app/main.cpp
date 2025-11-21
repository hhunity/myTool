#include "quillogger.h"
#include "config.h"

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

    Logger::Init("a");
    LOGE("test");
    std::cout << "APP_VERSION:" << PRODUCT_VERSION_STR << " BUILD_TIME:" << BUILD_TIME << " (" << GIT_HASH << ")\n";
    std::cout << "FILE_VERSION:" << FILE_VERSION_STR << "\n";

    return 0;
}
