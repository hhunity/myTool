#include <iostream>
#include <config.h>
#include "http_server.hpp"

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


int server_test(int argc, char** argv) {

    int x = 2;

    std::cout << "APP_VERSION:" << PRODUCT_VERSION_STR << " BUILD_TIME:" << BUILD_TIME << " (" << GIT_HASH << ")\n";
    std::cout << "FILE_VERSION:" << FILE_VERSION_STR << "\n";

    CommandServer server;
    server.start("0.0.0.0", 8080);

    std::cout << "Server started. Waiting commands..." << std::endl;

    while (true)
    {
        auto cmd = server.popCommandBlocking();
        if (cmd)
        {
            switch (*cmd)
            {
            case CommandServer::Command::Start:
                std::cout << "[CMD] Start!" << std::endl;
                break;
            case CommandServer::Command::Update:
                std::cout << "[CMD] Update!" << std::endl;
                break;
            case CommandServer::Command::Stop:
                std::cout << "[CMD] Stop!" << std::endl;
                return 0;
            default:
                std::cout << "[CMD] Unknown" << std::endl;
                break;
            }
        }
    }

    server.stop();

    return 0;
}
