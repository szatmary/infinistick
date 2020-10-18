#pragma once

#include <memory>
class Cache;
class BlockDevice {
public:
    int deviceId;
    int blockSize;
    int blockCount;
    std::shared_ptr<Cache> cache;

public:
    int Read(char* buf, size_t size, off_t offset);
    int Write(const char* buf, size_t size, off_t offset);
};
