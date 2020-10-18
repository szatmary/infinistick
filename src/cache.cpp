#include "cache.hpp"
#include "sqlite/sqlite3.h"

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

Cache::Cache(std::string cacheDb)
    : sqlite(std::unique_ptr<sqlite3, void (*)(sqlite3*)>(nullptr, [](sqlite3*) {}))
{
    assert(sqlite3_threadsafe());
    {
        sqlite3* db = nullptr;
        if (SQLITE_OK != sqlite3_open_v2(cacheDb.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)) {
            return;
        }
        sqlite = std::unique_ptr<sqlite3, void (*)(sqlite3*)>(db, [](sqlite3* db) { sqlite3_close_v2(db); });
    }

    char* errmsg = nullptr;
    int err = 0;
    err = sqlite3_exec(sqlite.get(), "PRAGMA schema.auto_vacuum=FULL", nullptr, nullptr, &errmsg);
    err = sqlite3_exec(sqlite.get(), "CREATE TABLE IF NOT EXISTS device (id INTEGER PRIMARY KEY, block_size INTEGER, block_count INTEGER, config BYTES)", nullptr, nullptr, &errmsg);
    err = sqlite3_exec(sqlite.get(), "CREATE TABLE IF NOT EXISTS block (device_id INTEGER, block_no INTEGER, atime INTEGER, mtime INTEGER, acount INTEGER, pin BOOL, dirty BOOL, data BLOB); CREATE UNIQUE INDEX IF NOT EXISTS blocks_idx ON blocks (device_id, block_no)", nullptr, nullptr, &errmsg);
    // TODO UNIQUE on block
}

BlockDevice Cache::NewBlockDevice(int blockSize, int blockCount)
{
    // todo cache prepared statements
    // todo check ret
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(sqlite.get(), "INSERT INTO device block_size = ?, block_count = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, blockSize);
    sqlite3_bind_int(stmt, 2, blockCount);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    auto devideId = sqlite3_last_insert_rowid(sqlite.get());
    return BlockDevice { static_cast<int>(devideId), blockSize, blockCount, shared_from_this() };
}

int Cache::Get(Block& block)
{
    // TODO block version version
    // TODO atime and acount
    // TODO compress and encrypt
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(sqlite.get(), "SELECT data FROM device WHERE device_id = ?, block_no = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, block.deviceId);
    sqlite3_bind_int(stmt, 2, block.blockNo);
    sqlite3_step(stmt);
    auto data = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt, 0));
    auto size = sqlite3_column_bytes(stmt, 0);
    block.data.assign(data, data + size);
    sqlite3_finalize(stmt);

    return 0;
}

int Cache::Put(const Block& block)
{
    // TODO block version version
    // TODO mtime, dirty, pinned
    // TODO compress and encrypt
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(sqlite.get(), "REPLACE INTO device SET data = ? WHERE device_id = ?, block_no = ?", -1, &stmt, nullptr);
    sqlite3_bind_blob(stmt, 1, block.data.data(), block.data.size(), SQLITE_TRANSIENT); // Can we use sataic here?
    sqlite3_bind_int(stmt, 2, block.deviceId);
    sqlite3_bind_int(stmt, 3, block.blockNo);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}
