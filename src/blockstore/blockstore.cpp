#include "net.hpp"
#include <cassert>
#include <iostream>

void BlockStore::CleanCache()
{
    assert(!!sqlite);
    if (!sqlite) {
        return;
    }

    auto name = sqlite3_db_filename(sqlite.get(), nullptr);
    auto space = std::filesystem::space(name);
    std::cerr << space.capacity << std::endl;
    std::cerr << space.free << std::endl;
    std::cerr << space.available << std::endl;
}

BlockStore::BlockStore()
    : curl(std::unique_ptr<CURLM, void (*)(CURLM*)>(curl_multi_init(), [](CURLM* c) { curl_multi_cleanup(c); }))
    , sqlite(std::unique_ptr<sqlite3, void (*)(sqlite3*)>(nullptr, [](sqlite3*) {}))
{
    assert(sqlite3_threadsafe());
}

bool BlockStore::Open(std::string cacheDb)
{
    {
        sqlite3* db = nullptr;
        if (SQLITE_OK != sqlite3_open_v2(cacheDb.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)) {
            return false;
        }
        sqlite = std::unique_ptr<sqlite3, void (*)(sqlite3*)>(db, [](sqlite3* db) { sqlite3_close_v2(db); });
    }

    char* errmsg = nullptr;
    auto err = sqlite3_exec(sqlite.get(), "CREATE TABLE IF NOT EXISTS device (id INTEGER, url TEXT, block_size INTEGER, block_count INTEGER)", nullptr, nullptr, &errmsg);
    auto err = sqlite3_exec(sqlite.get(), "CREATE TABLE IF NOT EXISTS block (device_id INTEGER, no INTEGER, ver INTEGER, atime INTEGER, mtime INTEGER, acount INTEGER, pin BOOL, dirty BOOL, data BLOB); CREATE INDEX IF NOT EXISTS blocks_idx ON blocks (disk_id, block_no)", nullptr, nullptr, &errmsg);
    return !!sqlite;
}

bool BlockStore::NewDisk(std::string url)
{
    // TODO preload the first block
}

// std::vector<uint8_t> BlockStore ::get(Url& url, const Credentials& cred)
// {
// }
