#include "cache.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    auto x = Cache::New("/tmp/test.db");

    const auto blockSize = 2 * 1024 * 1024;
    auto device = x->NewBlockDevice(blockSize, 1024);
    auto str = std::string("Hello World");
    device.Write(str.data(), str.size(), blockSize - 5);
    auto v = device.Read(str.size(), blockSize - 5);
    auto s = std::string(v.begin(), v.end());
    std::cout << s << std::endl;
}
