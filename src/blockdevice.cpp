#include "blockdevice.hpp"
#include "block.hpp"
#include "cache.hpp"
#include <cassert>
#include <iostream>

int BlockDevice::Read(char* buf, size_t size, off_t offset)
{
    assert(blockSize);
    auto blockNo = static_cast<int>(offset / blockSize);
    if (size == 0 || blockNo > blockCount) {
        return 0;
    }

    auto block = Block { deviceId, blockNo };
    auto blockOffset = offset - (blockNo * blockSize);
    auto readSize = blockSize - blockOffset;

    cache->Get(block); // TODO check return value
    std::memcpy(buf, block.data.data() + blockOffset, readSize);
    return Read(buf + readSize, blockSize - readSize, offset + readSize);
}

int BlockDevice::Write(const char* buf, size_t size, off_t offset)
{
    assert(blockSize);
    auto blockNo = static_cast<int>(offset / blockSize);
    if (size == 0 || blockNo > blockCount) {
        return 0;
    }

    auto block = Block { deviceId, blockNo };
    auto blockOffset = offset - (blockNo * blockSize);
    auto writeSize = blockSize - blockOffset;

    // if we are overwritting an entire block, we can skip the get
    if (writeSize < blockSize) {
        // TODO future optomisation, partial block writes
        cache->Get(block); // TODO check return value
    }

    block.data.resize(blockSize);
    std::memcpy(block.data.data() + blockOffset, buf, writeSize);
    cache->Put(block); // TODO check return value
    return Write(buf + writeSize, blockSize - writeSize, offset + writeSize);
}
