#pragma once

#include "block.hpp"
#include "blockdevice.hpp"
#include <memory>
#include <string>

class sqlite3;
class Cache : public std::enable_shared_from_this<Cache> {
public:
private:
    std::unique_ptr<sqlite3, void (*)(sqlite3*)> sqlite;

public:
    Cache(std::string cacheDb);

    void Clean();
    BlockDevice NewBlockDevice(int blockSize, int blockCount);
    BlockDevice OpenBlockDevice(std::string url, int blockSize, int blockCount);

    int Get(Block& block);
    int Put(const Block& block);
};
