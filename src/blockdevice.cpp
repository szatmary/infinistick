#include "blockdevice.hpp"
#include "block.hpp"
#include "cache.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

int BlockDevice::Read(char* buf, size_t size, off_t offset)
{
    assert(blockSize);
    auto blockNo = static_cast<int>(offset / blockSize);
    if (size == 0 || blockNo > blockCount) {
        return 0;
    }

    auto blockOffset = offset - (blockNo * blockSize);
    auto readSize = std::min(size, static_cast<size_t>(blockSize - blockOffset));
    std::cerr << "BlockDevice::Read " << blockNo << " " << blockOffset << " " << readSize << std::endl;

    // Check the local block
    if (!block.data.empty() || block.blockNo != blockNo) {
        block = Block { deviceId, blockNo };
        cache->GetBlock(block); // TODO check return value
    }

    block.data.resize(blockSize);
    std::memcpy(buf, block.data.data() + blockOffset, readSize);
    return readSize + Read(buf + readSize, size - readSize, offset + readSize);
}

int BlockDevice::Write(const char* buf, size_t size, off_t offset)
{
    // TODO batch writes?
    assert(blockSize);
    auto blockNo = static_cast<int>(offset / blockSize);
    if (size == 0 || blockNo > blockCount) {
        return 0;
    }

    auto block = Block { deviceId, blockNo };
    auto blockOffset = offset - (blockNo * blockSize);
    auto writeSize = std::min(size, static_cast<size_t>(blockSize - blockOffset));

    // if we are overwritting an entire block, we can skip the get
    if (writeSize < blockSize) {
        // TODO future optomisation, partial block writes
        if (!cache->GetBlock(block)) { // TODO check return value
            // TODO check store
        }
    }

    // Skip the write if the data is not channging
    block.data.resize(blockSize);
    if (0 != std::memcmp(block.data.data() + blockOffset, buf, writeSize)) {
        std::memcpy(block.data.data() + blockOffset, buf, writeSize);
        cache->PutBlock(block); // TODO check return value
    }

    return writeSize + Write(buf + writeSize, size - writeSize, offset + writeSize);
}
