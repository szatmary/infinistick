#pragma once

#include <cstdint>
// #include <curl/curl.h>
// #include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Block;
class Store {
public:
    Store() = default; // TODO accept a configuration (json maybe?)
    virtual int get(Block& block) = 0;
    virtual int put(const Block& block) = 0;
};

// dummy store for pure local sotrage
class Store_Pinned {
    Store_Pinned() = default;
    virtual int put(const Block& block) { return 0; }
    virtual int get(Block& block) { return 0; }
};
