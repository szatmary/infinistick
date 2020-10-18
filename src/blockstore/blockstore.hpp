#pragma once

#include <cstdint>
#include <curl/curl.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../sqlite/sqlite3.h"

class Block {
public:
    uint64_t DiskId;
    uint32_t BlockNo;
    uint32_t Version;
    std::vector<uint8_t> data;
    std::vector<uint8_t> Serialize();
};

class Store {
public:
    Store() = default; // TODO accept a configuration (json maybe?)
    virtual int put(const Block& block) = 0;
    virtual int get(Block& block) = 0;
};

class Store_Curl {
private:
    std::unique_ptr<CURLM, void (*)(CURLM*)> curl;

public:
    Store_Curl();
    virtual int put(const Block& block) = 0;
    virtual int get(Block& block) = 0;
};

class BlockDevice {
private:
    uint64_t id;
    uint32_t blockSize;
    uint32_t blockCount;
    std::unique_ptr<Store> store;

public:
    read()
};

class Cache {
public:
private:
    std::unique_ptr<sqlite3, void (*)(sqlite3*)> sqlite;

public:
    void Clean();

    Cache();
    bool Open(std::string cacheDb);
    Disk NewDisk(std::string url, uint32_t blockSize, uint32_t blockCount);
};
