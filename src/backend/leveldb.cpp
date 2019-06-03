#include "backend/leveldb.h"

#include <leveldb/cache.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>

const size_t LEVELDB_METADATA_CACHE = 128 * 1048576;
const size_t LEVELDB_CONTENT_CACHE = 128 * 1048576;

#define INIT_DB(db, cacheSize, path) \
  leveldb::Cache *db##Cache = leveldb::NewLRUCache(cacheSize); \
  leveldb::Options db##Opt; \
  PathComparator *db##Cmp = new PathComparator; \
  db##Opt.comparator = db##Cmp; \
  db##Opt.create_if_missing = true; \
  db##Opt.block_cache = db##Cache; \
  \
  leveldb::Status db##Status = leveldb::DB::Open(db##Opt, path, &db); \
  assert(db##Status.ok()); \

namespace hermes::backend {
  LDB::LDB(hermes::options opts) {
    INIT_DB(metadata, LEVELDB_METADATA_CACHE, opts.kvdev);
    INIT_DB(content, LEVELDB_CONTENT_CACHE, opts.filedev);
  }

  LDB::~LDB() {
    // Deleting LevelDB databases will force them to sync to disk
    delete this->metadata;
    delete this->content;
  }

  write_result LDB::put_metadata(const std::string_view &path, const hermes::metadata &metadata) {
    // TODO: check permission
    // metadatas are sized objects, so we are going to reinterpret it
    const leveldb::Slice pathSlice(path.data(), path.length());
    const leveldb::Slice content(reinterpret_cast<const char *>(&metadata), sizeof(hermes::metadata));

    auto status = this->metadata->Put(leveldb::WriteOptions(), pathSlice, content);

    if(status.ok())
      return write_result::Ok;
    else
      return write_result::UnknownFailure;
  }

  std::optional<hermes::metadata> LDB::fetch_metadata(const std::string_view &path) {
    std::string str;
    const leveldb::Slice pathSlice(path.data(), path.length());
    auto status = this->metadata->Get(leveldb::ReadOptions(), pathSlice, &str);

    // TODO: use seek and manually copy

    if(status.ok()) {
      hermes::metadata result;
      str.copy(reinterpret_cast<char *>(&result), std::string::npos);
      return { result };
    } else
      return {};
  }

  std::optional<hermes::metadata> LDB::remove_metadata(const std::string_view &path) {
    // TODO: check permission
    const leveldb::Slice pathSlice(path.data(), path.length());

    auto fetched = this->fetch_metadata(path);

    if(!fetched)
      return fetched;

    auto status = this->metadata->Delete(leveldb::WriteOptions(), pathSlice);

    if(status.ok())
      return fetched;
    else
      return {}; // TODO: log error
  }

  write_result LDB::put_content(const std::string_view &path, const std::string_view &content) {
    // TODO: check permission

    const leveldb::Slice pathSlice(path.data(), path.length());
    const leveldb::Slice contentSlice(content.data(), content.length());
    auto status = this->content->Put(leveldb::WriteOptions(), pathSlice, contentSlice);

    if(status.ok())
      return write_result::Ok;
    else
      return write_result::UnknownFailure;
  }

  std::optional<std::string> LDB::fetch_content(const std::string_view &path) {
    std::string result;
    const leveldb::Slice pathSlice(path.data(), path.length());
    auto status = this->content->Get(leveldb::ReadOptions(), pathSlice, &result);

    if(status.ok())
      return { result };
    else
      return {}; // TODO: handle other types of errors
  }

  std::optional<std::string> LDB::remove_content(const std::string_view &path) {
    const leveldb::Slice pathSlice(path.data(), path.length());

    auto fetched = this->fetch_content(path);

    if(!fetched)
      return fetched;

    auto status = this->content->Delete(leveldb::WriteOptions(), pathSlice);

    if(status.ok())
      return fetched;
    else
      return {}; // TODO: log error
  }
}
