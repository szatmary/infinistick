#pragma once

#include <memory>
#include <vector>

class Cache;
class BlockDevice { // TODO rename this to VirtualBlockDevice
public:
    int deviceId;
    int blockSize;
    int blockCount;
    std::shared_ptr<Cache> cache;

public:
    int Read(char* buf, size_t size, off_t offset);
    int Write(const char* buf, size_t size, off_t offset);

    std::vector<uint8_t> Read(size_t size, off_t offset)
    {
        std::vector<uint8_t> buf(size);
        buf.resize(Read(reinterpret_cast<char*>(buf.data()), buf.size(), offset));
        return buf;
    }

    int Write(const std::vector<uint8_t>& buf, off_t offset)
    {
        return Write(reinterpret_cast<const char*>(buf.data()), buf.size(), offset);
    }
};
