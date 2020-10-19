#include "cache.hpp"
#include "sqlite/sqlite3.h"
#include <cassert>
#include <iostream>

// void Cache::Clean()
// {
//     assert(!!sqlite);
//     if (!sqlite)
//     {
//         return;
//     }

//     auto name = sqlite3_db_filename(sqlite.get(), nullptr);
//     auto space = std::filesystem::space(name);
//     std::cerr << space.capacity << std::endl;
//     std::cerr << space.free << std::endl;
//     std::cerr << space.available << std::endl;
// }

Cache::Cache(sqlite3* db)
    : sqlite(std::unique_ptr<sqlite3, void (*)(sqlite3*)>(db, [](sqlite3* db) { sqlite3_close_v2(db); }))
{

    int err = 0;
    char* errmsg = nullptr;
    // TOOD check errors
    err = sqlite3_exec(sqlite.get(), "PRAGMA schema.auto_vacuum=FULL", nullptr, nullptr, &errmsg);
    err = sqlite3_exec(sqlite.get(), "PRAGMA PRAGMA schema.page_size=65536", nullptr, nullptr, &errmsg);
    err = sqlite3_exec(sqlite.get(), "CREATE TABLE IF NOT EXISTS device (id INTEGER PRIMARY KEY, block_size INTEGER, block_count INTEGER, config BYTES DEFAULT NULL)", nullptr, nullptr, &errmsg);
    err = sqlite3_exec(sqlite.get(), "CREATE TABLE IF NOT EXISTS block (device_id INTEGER, block_no INTEGER, atime INTEGER, mtime INTEGER, acount INTEGER, pin BOOL, dirty BOOL, data BLOB); CREATE UNIQUE INDEX IF NOT EXISTS block_idx ON block (device_id, block_no)", nullptr, nullptr, &errmsg);
    // TODO UNIQUE on block
}

std::shared_ptr<Cache> Cache::New(std::string cacheDb)
{
    assert(sqlite3_threadsafe());
    sqlite3* db = nullptr;
    if (SQLITE_OK != sqlite3_open_v2(cacheDb.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)) {
        return std::shared_ptr<Cache>(nullptr);
    }
    return std::shared_ptr<Cache>(new Cache(db));
}

std::shared_ptr<BlockDevice> Cache::GetBlockDevice(int deviceId)
{
    sqlite3_stmt* tmp = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(sqlite.get(), "SELECT block_size, block_count FROM device WHERE id = ?", -1, &tmp, nullptr)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_prepare_v2 returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return std::shared_ptr<BlockDevice>(nullptr);
    }

    auto stmt = std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)>(tmp, [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); });
    if (SQLITE_OK != sqlite3_bind_int(stmt.get(), 1, deviceId)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_bind_int returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return std::shared_ptr<BlockDevice>(nullptr);
    }
    switch (sqlite3_step(stmt.get())) {
    case SQLITE_ROW: {
        auto blockSize = sqlite3_column_int(stmt.get(), 0);
        auto blockCount = sqlite3_column_int(stmt.get(), 1);
        return std::make_shared<BlockDevice>(BlockDevice { deviceId, blockCount, blockSize, shared_from_this() });
    }
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return std::shared_ptr<BlockDevice>(nullptr);
    }
}

std::vector<std::shared_ptr<BlockDevice>> Cache::ListBlockDevices()
{
    sqlite3_stmt* tmp = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(sqlite.get(), "SELECT id, block_size, block_count FROM device", -1, &tmp, nullptr)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_prepare_v2 returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return std::vector<std::shared_ptr<BlockDevice>>();
    }

    std::vector<std::shared_ptr<BlockDevice>> devices;
    auto stmt = std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)>(tmp, [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); });
    for (;;) {
        switch (sqlite3_step(stmt.get())) {
        case SQLITE_DONE:
            return devices;
        case SQLITE_ROW: {
            auto deviceId = sqlite3_column_int(stmt.get(), 0);
            auto blockSize = sqlite3_column_int(stmt.get(), 1);
            auto blockCount = sqlite3_column_int(stmt.get(), 2);
            devices.emplace_back(std::make_shared<BlockDevice>(BlockDevice { deviceId, blockCount, blockSize, shared_from_this() }));
        } break;
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
            return std::vector<std::shared_ptr<BlockDevice>>();
        }
    }
}

BlockDevice Cache::NewBlockDevice(std::string name, int blockSize, int blockCount)
{
    // todo cache prepared statements
    // todo check ret
    int err = 0;
    sqlite3_stmt* tmp = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(sqlite.get(), "INSERT INTO device (block_size, block_count) VALUES (?, ?)", -1, &tmp, nullptr)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_prepare_v2 returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return BlockDevice { 0, 0, 0 };
    }

    auto stmt = std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)>(tmp, [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); });
    if (SQLITE_OK != sqlite3_bind_int(stmt.get(), 1, blockSize)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_bind_int returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return BlockDevice { 0, 0, 0 };
    }
    if (SQLITE_OK != sqlite3_bind_int(stmt.get(), 2, blockCount)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_bind_int returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return BlockDevice { 0, 0, 0 };
    }
    if (SQLITE_DONE != sqlite3_step(stmt.get())) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return BlockDevice { 0, 0, 0 };
    }
    auto devideId = sqlite3_last_insert_rowid(sqlite.get());
    return BlockDevice { static_cast<int>(devideId), blockSize, blockCount, shared_from_this() };
}

// BlockDevice Cache::OpenBlockDevice(int32 deviceId)
// {
// }

// TODO error codes, Non Found vs error!
bool Cache::GetBlock(Block& block)
{
    // TODO block version version
    // TODO atime and acount
    // TODO compress and encrypt
    sqlite3_stmt* tmp = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(sqlite.get(), "SELECT data FROM block WHERE device_id = ? AND block_no = ?", -1, &tmp, nullptr)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " SWsqlite3_prepare_v2 returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }

    auto stmt = std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)>(tmp, [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); });
    if (SQLITE_OK != sqlite3_bind_int(stmt.get(), 1, block.deviceId)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_bind_int returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }
    if (SQLITE_OK != sqlite3_bind_int(stmt.get(), 2, block.blockNo)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_bind_int returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }

    switch (sqlite3_step(stmt.get())) {
    case SQLITE_ROW:
        break; // This is what we want
    case SQLITE_DONE:
        return false; // TODO return not found
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }

    auto data = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt.get(), 0));
    auto size = sqlite3_column_bytes(stmt.get(), 0);
    block.data.assign(data, data + size);

    return true;
}

bool Cache::PutBlock(const Block& block)
{
    // TODO block version version
    // TODO pinned
    // TODO compress and encrypt
    sqlite3_stmt* tmp = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(sqlite.get(), "REPLACE INTO block (device_id, block_no, data, dirty) VALUES(?, ?, ?,true)", -1, &tmp, nullptr)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }

    auto stmt = std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)>(tmp, [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); });
    if (SQLITE_OK != sqlite3_bind_int(stmt.get(), 1, block.deviceId)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }
    if (SQLITE_OK != sqlite3_bind_int(stmt.get(), 2, block.blockNo)) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }
    if (SQLITE_OK != sqlite3_bind_blob(stmt.get(), 3, block.data.data(), block.data.size(), SQLITE_TRANSIENT)) { // Can we use sataic here?
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }
    if (SQLITE_DONE != sqlite3_step(stmt.get())) {
        std::cerr << __FILE__ << ":" << __LINE__ << " sqlite3_step returned error " << sqlite3_errmsg(sqlite.get()) << std::endl;
        return false;
    }
    return true;
}
