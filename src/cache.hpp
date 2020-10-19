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

    Cache();
    Cache(sqlite3*);

public:
    static std::shared_ptr<Cache> New(std::string cacheDb);

    void Clean();
    // friendly name
    BlockDevice NewBlockDevice(std::string name, int blockSize, int blockCount);
    // BlockDevice OpenBlockDevice(int32 deviceId);

    std::shared_ptr<BlockDevice> GetBlockDevice(int deviceId);
    std::vector<std::shared_ptr<BlockDevice>> ListBlockDevices();

    bool GetBlock(Block& block);
    bool PutBlock(const Block& block);
};
