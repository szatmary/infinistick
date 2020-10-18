#include "net/net.hpp"

int main(int argc, char** argv)
{
    auto x = Net();
    x.Open("/tmp/test.db");
}
