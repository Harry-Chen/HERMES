#include "backend/leveldb.h"

#include <leveldb/cache.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>
#include <leveldb/write_batch.h>

using namespace std;

const size_t LEVELDB_METADATA_CACHE = 128 * 1048576;
const size_t LEVELDB_CONTENT_CACHE = 128 * 1048576;

const size_t LEVELDB_CHUNK_SIZE = 4096;

const char *LEVELDB_NEXT_ID_KEY = "#id";

#define INIT_DB(db, cacheSize, path, cmp) \
  leveldb::Cache *db##Cache = leveldb::NewLRUCache(cacheSize); \
  leveldb::Options db##Opt; \
  if(cmp) { \
    PathComparator *db##Cmp = new PathComparator; \
    db##Opt.comparator = db##Cmp; \
  } \
  db##Opt.create_if_missing = true; \
  db##Opt.block_cache = db##Cache; \
  \
  leveldb::Status db##Status = leveldb::DB::Open(db##Opt, path, &db); \
  assert(db##Status.ok()); \

namespace hermes::backend {
  LDB::LDB(hermes::options opts) {
    INIT_DB(metadata, LEVELDB_METADATA_CACHE, opts.kvdev, true);
    INIT_DB(content, LEVELDB_CONTENT_CACHE, opts.filedev, false);
  }

  LDB::~LDB() {
    // Deleting LevelDB databases will force them to sync to disk
    delete this->metadata;
    delete this->content;
  }

  uint64_t LDB::next_id() {
    string str;
    auto status = this->metadata->Get(leveldb::ReadOptions(), LEVELDB_NEXT_ID_KEY, &str);

    uint64_t next = 0;
    if(status.ok())
      next = be64toh(*reinterpret_cast<uint64_t *>(str.data()));

    uint64_t storing = htobe64(next + 1);

    // Store next id
    if(!this->metadata->Put(leveldb::WriteOptions(), LEVELDB_NEXT_ID_KEY, leveldb::Slice(reinterpret_cast<char *>(&storing), 8)).ok()) {
      cout<<">> ERROR: cannot save next file id"<<endl;
      // TODO: handle
    }

    cout<<">> DEBUG: Yielding ID: "<<next<<endl;
    return next;
  }

  write_result LDB::put_metadata(const string_view &path, const hermes::metadata &metadata) {
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

  optional<hermes::metadata> LDB::fetch_metadata(const string_view &path) {
    string str;
    const leveldb::Slice pathSlice(path.data(), path.length());
    auto status = this->metadata->Get(leveldb::ReadOptions(), pathSlice, &str);

    // TODO: use seek and manually copy

    if(status.ok()) {
      hermes::metadata result;
      str.copy(reinterpret_cast<char *>(&result), string::npos);
      return { result };
    } else
      return {};
  }

  optional<hermes::metadata> LDB::remove_metadata(const string_view &path) {
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

  write_result LDB::put_content(uint64_t id, size_t offset, const string_view &content) {

    // TODO: make it work even if LEVELDB_CHUNK_SIZE changes. Needs to check the next offset entry
    const uint64_t fromOffset = (offset / LEVELDB_CHUNK_SIZE) * LEVELDB_CHUNK_SIZE;

    string strbeid, key;
    string original;

    for(uint64_t blkoff = fromOffset; blkoff < offset + content.size(); blkoff += LEVELDB_CHUNK_SIZE) {
      uint64_t beid = htobe64(id);
      uint64_t beoffset = htobe64(blkoff);

      strbeid = string_view(reinterpret_cast<char *>(&beid), 8);
      key = strbeid;
      key += string_view(reinterpret_cast<char *>(&beoffset), 8);

      if(blkoff < offset) {
        // Writing from middle
        if(!this->content->Get(leveldb::ReadOptions(), key, &original).ok()) {
          // Holes? we don't want that
          cout<<"Warning: Write holes"<<endl;
          original = "";
        }

        size_t len = LEVELDB_CHUNK_SIZE - (offset - blkoff);
        if(len > content.size()) len = content.size();

        original.resize(offset - blkoff, 0);
        original += content.substr(0, len);

        // cout<<">> BACKEND: Put first chunk: "<<id<<" @ "<<blkoff<<endl;

        this->content->Put(leveldb::WriteOptions(), key, original);
      } else if(blkoff + LEVELDB_CHUNK_SIZE <= offset + content.size()) {
        // Write into head
        if(!this->content->Get(leveldb::ReadOptions(), key, &original).ok()) {
          original = "";
        }

        size_t len = content.size() + offset - blkoff;
        if(original.size() < len) {
          // Completely overwrite
          const string_view chunk_view = content.substr(blkoff - offset);
          this->content->Put(leveldb::WriteOptions(), key, leveldb::Slice(chunk_view.data(), chunk_view.size()));
        } else {
          original = string(content.substr(blkoff - offset)) + original.substr(len);
          this->content->Put(leveldb::WriteOptions(), key, original);
        }
      } else {
        // cout<<">> BACKEND: Put last chunk: "<<id<<" @ "<<blkoff<<endl;
        // Write entire chunk
        const string_view chunk_view = content.substr(blkoff - offset, blkoff - offset + LEVELDB_CHUNK_SIZE);

        // cout<<">> BACKEND: Data: "<<chunk_view<<endl;
        this->content->Put(leveldb::WriteOptions(), key, leveldb::Slice(chunk_view.data(), chunk_view.size()));
      }
    }

    return write_result::Ok;
  }

  void LDB::fetch_content(uint64_t id, size_t offset, size_t len, char *buf) {
    // TODO: make it work even if LEVELDB_CHUNK_SIZE changes. Needs to check the next offset entry
    const uint64_t fromOffset = (offset / LEVELDB_CHUNK_SIZE) * LEVELDB_CHUNK_SIZE;

    string strbeid, key;
    uint64_t beid = htobe64(id);
    uint64_t beoffset = htobe64(fromOffset);

    char *ptr = buf;

    strbeid = string_view(reinterpret_cast<char *>(&beid), 8);
    key = strbeid;
    key += string_view(reinterpret_cast<char *>(&beoffset), 8);

    unique_ptr<leveldb::Iterator> it(this->content->NewIterator(leveldb::ReadOptions()));
    it->Seek(key);

    for(it->Seek(key); it->Valid(); it->Next()) {
      uint64_t cid = be64toh(*reinterpret_cast<const uint64_t *>(it->key().data()));
      uint64_t blkoff = be64toh(*reinterpret_cast<const uint64_t *>(it->key().data() + 8));

      // cout<<">> BACKEND: Read chunk: "<<cid<<" @ "<<blkoff<<endl;
      // cout<<">> BACKEND: Already read: "<<result.size()<<endl;

      if(cid != id)
        break;

      // Detect holes
      if(offset + (ptr - buf) < blkoff) {
        memset(ptr, 0, blkoff - offset - (ptr - buf));
        ptr = buf + (blkoff - offset);
      }

      size_t innerOff = offset + (ptr - buf) - blkoff;
      size_t innerLen = len - (ptr - buf);
      if(innerLen > LEVELDB_CHUNK_SIZE - innerOff) innerLen = LEVELDB_CHUNK_SIZE - innerOff; // At most read chunk-size bytes

      // cout<<">> BACKEND: Read off/len: "<<innerOff<<" [] "<<innerLen<<endl;

      string_view value(it->value().data(), it->value().size());
      string_view view = value.substr(innerOff, innerLen);
      view.copy(ptr, std::string::npos);
      ptr += view.size();

      if(view.size() < innerLen) {
        memset(ptr, 0, innerLen - view.size());
        ptr += innerLen - view.size();
      }

      if((ptr - buf) == len) break;
    }

    if(ptr - buf < len)
      memset(ptr, 0, len + (ptr-buf));

    return;
  }

  write_result LDB::remove_content(uint64_t id) {
    uint64_t beid = htobe64(id);
    string key(reinterpret_cast<char *>(&beid), 8);
    key.resize(16, 0);

    unique_ptr<leveldb::Iterator> it(this->metadata->NewIterator(leveldb::ReadOptions()));
    leveldb::WriteBatch batch;

    for(it->Seek(key); it->Valid(); it->Next()) {
      uint64_t cid = be64toh(*reinterpret_cast<const uint64_t *>(it->key().data()));

      if(cid != id) break;
      batch.Delete(it->key());
    }

    auto status = this->content->Write(leveldb::WriteOptions(), &batch);

    if(status.ok())
      return write_result::Ok;
    else
      return write_result::UnknownFailure;
  }
}
