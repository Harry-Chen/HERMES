#include "berkeleydb.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <string>

const char *COUNTER_KEY = "\xca\xfe";
const size_t DB_CHUNK_SIZE = 4096;
const size_t DB_METADATA_CACHE = 32 * 1024;
const size_t DB_CONTENT_CACHE = 128 * 1024;

// backward compatible
#if DB_VERSION_FAMILY <= 11
int compare_path(Db *_dbp, const Dbt *a, const Dbt *b)
#else
int compare_path(Db *_dbp, const Dbt *a, const Dbt *b, size_t *)
#endif
{
    auto ad = (char *)a->get_data();
    auto bd = (char *)b->get_data();
    for (size_t i = 0; i < a->get_size() && i < b->get_size(); ++i) {
        if (ad[i] != bd[i]) {
            if (ad[i] == '/')
                return -1;
            else if (bd[i] == '/')
                return 1;
            return ad[i] < bd[i] ? -1 : 1;
        }
    }

    if (a->get_size() == b->get_size()) return 0;
    return a->get_size() < b->get_size() ? -1 : 1;
}

namespace hermes::backend {
BDB::BDB(hermes::options opts) {
    mkdir(opts.metadev, 0755);
    metadataEnv = new DbEnv((int)0);
    metadataEnv->set_cachesize(0, DB_METADATA_CACHE, 1);
    metadataEnv->open(opts.metadev, DB_CREATE | DB_INIT_MPOOL, 0755);
    metadata = new Db(metadataEnv, 0);

    mkdir(opts.filedev, 0755);
    contentEnv = new DbEnv((int)0);
    contentEnv->set_cachesize(0, DB_CONTENT_CACHE, 1);
    contentEnv->open(opts.filedev, DB_CREATE | DB_INIT_MPOOL, 0755);
    content = new Db(contentEnv, 0);

    metadata->set_bt_compare(compare_path);

    metadata->open(nullptr, "meta", "hermes", DB_BTREE, DB_CREATE, 0755);
    content->open(nullptr, "content", "hermes", DB_BTREE, DB_CREATE, 0755);

    Dbt key((void *)COUNTER_KEY, strlen(COUNTER_KEY));
    Dbt data;
    data.set_data((void *)&counter);
    data.set_ulen(sizeof(counter));
    data.set_flags(DB_DBT_USERMEM);
    if (metadata->get(nullptr, &key, &data, 0) == DB_NOTFOUND) {
        counter = 0;
    } else {
        counter = be64toh(counter);
    }
}
uint64_t BDB::next_id() {
    uint64_t next = counter++;
    uint64_t storing = htobe64(counter);

    Dbt key((void *)COUNTER_KEY, strlen(COUNTER_KEY));
    Dbt data((void *)&storing, sizeof(storing));

    metadata->put(nullptr, &key, &data, 0);

    return next;
}
write_result BDB::put_content(uint64_t id, size_t offset, const std::string_view &content) {
    // TODO: make it work even if LEVELDB_CHUNK_SIZE changes. Needs to check the next offset entry
    const uint64_t fromOffset = (offset / DB_CHUNK_SIZE) * DB_CHUNK_SIZE;

    std::string strbeid, key;
    std::string original;

    for (uint64_t blkoff = fromOffset; blkoff < offset + content.size(); blkoff += DB_CHUNK_SIZE) {
        uint64_t beid = htobe64(id);
        uint64_t beoffset = htobe64(blkoff);

        strbeid = std::string_view(reinterpret_cast<char *>(&beid), 8);
        key = strbeid;
        key += std::string_view(reinterpret_cast<char *>(&beoffset), 8);

        if (blkoff < offset) {
            // Writing from middle
            Dbt db_key(key.data(), key.size());
            Dbt db_value;
            if (!this->content->get(nullptr, &db_key, &db_value, 0)) {
                // Holes? we don't want that
                original = "";
            } else {
                original.assign((char *)db_value.get_data(), db_value.get_size());
            }

            size_t len = DB_CHUNK_SIZE - (offset - blkoff);
            if (len > content.size()) len = content.size();

            original.resize(offset - blkoff, 0);
            original += content.substr(0, len);

            // cout<<">> BACKEND: Put first chunk: "<<id<<" @ "<<blkoff<<endl;
            Dbt insert_key(key.data(), key.size());
            Dbt insert_value(original.data(), original.size());
            this->content->put(nullptr, &insert_key, &insert_value, 0);
        } else if (blkoff + DB_CHUNK_SIZE > offset + content.size()) {
            // Write into head
            Dbt db_key(key.data(), key.size());
            Dbt db_value;
            if (!this->content->get(nullptr, &db_key, &db_value, 0)) {
                original = "";
            } else {
                original.assign((char *)db_value.get_data(), db_value.get_size());
            }

            size_t len = content.size() + offset - blkoff;
            if (original.size() < len) {
                // Completely overwrite
                const std::string_view chunk_view = content.substr(blkoff - offset);
                Dbt insert_key(key.data(), key.size());
                Dbt insert_value((void *)chunk_view.data(), chunk_view.size());
                this->content->put(nullptr, &insert_key, &insert_value, 0);
            } else {
                original = std::string(content.substr(blkoff - offset)) + original.substr(len);
                Dbt insert_key(key.data(), key.size());
                Dbt insert_value((void *)original.data(), original.size());
                this->content->put(nullptr, &insert_key, &insert_value, 0);
            }
        } else {
            // cout<<">> BACKEND: Put last chunk: "<<id<<" @ "<<blkoff<<endl;
            // Write entire chunk
            const std::string_view chunk_view =
                content.substr(blkoff - offset, DB_CHUNK_SIZE);

            Dbt insert_key(key.data(), key.size());
            Dbt insert_value((void *)chunk_view.data(), chunk_view.size());
            this->content->put(nullptr, &insert_key, &insert_value, 0);
        }
    }

    return write_result::Ok;
}  // namespace hermes::backend

void BDB::fetch_content(uint64_t id, size_t offset, size_t len, char *buf) {
    // TODO: make it work even if LEVELDB_CHUNK_SIZE changes. Needs to check the next offset entry
    const uint64_t fromOffset = (offset / DB_CHUNK_SIZE) * DB_CHUNK_SIZE;

    std::string strbeid, key;
    uint64_t beid = htobe64(id);
    uint64_t beoffset = htobe64(fromOffset);

    char *ptr = buf;

    strbeid = std::string_view(reinterpret_cast<char *>(&beid), 8);
    key = strbeid;
    key += std::string_view(reinterpret_cast<char *>(&beoffset), 8);

    Dbc *dbc = nullptr;
    content->cursor(nullptr, &dbc, 0);

    Dbt seekKey((void *)key.data(), key.size());
    Dbt seekData;
    dbc->get(&seekKey, &seekData, DB_SET_RANGE);

    Dbt dbKey = seekKey;
    Dbt dbValue = seekData;

    do {
        uint64_t cid = be64toh(*reinterpret_cast<const uint64_t *>(dbKey.get_data()));
        uint64_t blkoff =
            be64toh(*reinterpret_cast<const uint64_t *>((char *)dbKey.get_data() + 8));

        // cout<<">> BACKEND: Read chunk: "<<cid<<" @ "<<blkoff<<endl;
        // cout<<">> BACKEND: Already read: "<<result.size()<<endl;

        if (cid != id) break;

        // Detect holes
        if (offset + (ptr - buf) < blkoff) {
            if (blkoff - offset >= len) break;

            memset(ptr, 0, blkoff - offset - (ptr - buf));
            ptr = buf + (blkoff - offset);
        }

        size_t innerOff = offset + (ptr - buf) - blkoff;
        size_t innerLen = len - (ptr - buf);
        if (innerLen > DB_CHUNK_SIZE - innerOff)
            innerLen = DB_CHUNK_SIZE - innerOff;  // At most read chunk-size bytes

        // cout<<">> BACKEND: Read off/len: "<<innerOff<<" [] "<<innerLen<<endl;

        std::string_view value((char *)dbValue.get_data(), dbValue.get_size());
        std::string_view view = value.substr(innerOff, innerLen);
        view.copy(ptr, std::string::npos);
        ptr += view.size();

        if (view.size() < innerLen) {
            memset(ptr, 0, innerLen - view.size());
            ptr += innerLen - view.size();
        }

        if ((ptr - buf) == len) break;
    } while (dbc->get(&dbKey, &dbValue, DB_NEXT) == 0);
    dbc->close();

    if (ptr - buf < len) memset(ptr, 0, len - (ptr - buf));
}
write_result BDB::put_metadata(const std::string_view &path, const hermes::metadata &metadata) {
    Dbt key((void *)path.data(), path.size());
    Dbt value((void *)&metadata, sizeof(metadata));

    if (this->metadata->put(nullptr, &key, &value, 0) == 0) {
        return write_result::Ok;
    } else {
        return write_result::UnknownFailure;
    }
}
std::optional<hermes::metadata> BDB::fetch_metadata(const std::string_view &path) {
    Dbt key((void *)path.data(), path.size());
    Dbt value;

    if (metadata->get(nullptr, &key, &value, 0) == DB_NOTFOUND) {
        return {};
    } else {
        hermes::metadata result;
        memcpy(&result, value.get_data(), value.get_size());
        return {result};
    }
}
std::optional<hermes::metadata> BDB::remove_metadata(const std::string_view &path) {
    auto data = fetch_metadata(path);

    if (!data) return data;

    Dbt key((void *)path.data(), path.size());
    if (metadata->del(nullptr, &key, 0) == 0) {
        return data;
    } else {
        // TODO: log error
        return {};
    }
}
BDB::~BDB() {
    metadata->close(0);
    content->close(0);
    delete metadata;
    delete content;
}
}  // namespace hermes::backend
