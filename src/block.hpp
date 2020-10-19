#pragma once
#include <cinttypes>
#include <vector>

// TODO Compress, Encrypt, Sign
struct Block {
    int deviceId;
    int blockNo;
    std::vector<uint8_t> data;
};
