//
// Created by icyf on 6/5/19.
//

#include "vedis.h"

using namespace std;

const size_t VEDIS_CHUNK_SIZE = 2048;
const char *VEDIS_NEXT_ID_KEY = "#id";

namespace hermes::backend {
/*
inline void showErr(vedis *store, int lineNo)
{
    const char *zBuf;
    int iLen;
    vedis_config(store, VEDIS_CONFIG_ERR_LOG, &zBuf, &iLen);
    if (iLen > 0)
        cout << lineNo << ": " << zBuf << endl;
}
*/
Vedis::Vedis(hermes::options opt) {
    vedis_lib_init();
    vedis_lib_config(VEDIS_LIB_CONFIG_THREAD_LEVEL_MULTI);
    assert(vedis_open(&metadata, opt.metadev) == VEDIS_OK);
    assert(vedis_open(&content, opt.filedev) == VEDIS_OK);

    vedis_int64 bufSize = sizeof(uint64_t);
    if (vedis_kv_fetch(metadata, VEDIS_NEXT_ID_KEY, -1, &counter, &bufSize)) counter = 0;
}

Vedis::~Vedis() {
    assert(vedis_close(metadata) == VEDIS_OK);
    assert(vedis_close(content) == VEDIS_OK);
}

uint64_t Vedis::next_id() {
    uint64_t next = counter++;
    assert(vedis_kv_store(this->metadata, VEDIS_NEXT_ID_KEY, -1, &counter, sizeof(uint64_t)) ==
           VEDIS_OK);
    return next;
}

write_result Vedis::put_metadata(const string_view &path, const hermes::metadata &meta) {
    if (path.length() > 1) {  // not root
        // A special type of metadata, which is a list of all children
        // of a directory, is represented by ("D"+directory_name).
        auto splitted = split_parent(path);
        auto parent = splitted.first, child = splitted.second;
        string dkey = "D";
        dkey += parent;

        // cout << "put_meta: " << splitted.first << " " << splitted.second << endl;

        vedis_exec_fmt(this->metadata, "SADD %s \"%s\"", dkey.data(), child.data());
    }
    if (vedis_kv_store(this->metadata, path.data(), path.length(), &meta,
                       sizeof(hermes::metadata))) {
        // showErr(this->metadata, __LINE__);
        return write_result::UnknownFailure;
    }
    return write_result::Ok;
}

optional<hermes::metadata> Vedis::fetch_metadata(const string_view &path) {
    hermes::metadata result;
    vedis_int64 bufSize = sizeof(hermes::metadata);

    if (vedis_kv_fetch(this->metadata, path.data(), path.length(), &result, &bufSize)) {
        // showErr(this->metadata, __LINE__);
        return {};
    }
    return {result};
}

optional<hermes::metadata> Vedis::remove_metadata(const string_view &path) {
    auto fetched = this->fetch_metadata(path);
    if (vedis_kv_delete(this->metadata, path.data(), path.length())) return {};

    auto splitted = split_parent(path);
    auto parent = splitted.first, child = splitted.second;
    string dkey = "D";
    dkey += parent;

    vedis_exec_fmt(this->metadata, "SREM %s \"%s\"", dkey.data(), child.data());
    // printf("SREM %s \"%s\"\n", dkey.data(), child.data());
    return fetched;
}

write_result Vedis::put_content(uint64_t id, size_t offset, const string_view &cont) {
    static uint8_t block[VEDIS_CHUNK_SIZE + 5];

    size_t fromOffset = offset / VEDIS_CHUNK_SIZE * VEDIS_CHUNK_SIZE;
    for (size_t blkoff = fromOffset; blkoff < offset + cont.length(); blkoff += VEDIS_CHUNK_SIZE) {
        string key;
        key += string_view(reinterpret_cast<char *>(&id), 8);
        key += string_view(reinterpret_cast<char *>(&blkoff), 8);

        vedis_exec_fmt(this->content, "SADD %lu \"%lu\"", id, blkoff);
        // printf("SADD %lu \"%lu\"\n", id, blkoff);

        if (__builtin_expect(blkoff < offset, 0)) {
            vedis_int64 bufSize = VEDIS_CHUNK_SIZE;
            if (vedis_kv_fetch(this->content, key.data(), key.length(), block, &bufSize))
                memset(block, 0, sizeof(block));

            size_t len = VEDIS_CHUNK_SIZE - (offset - blkoff);
            if (len > cont.length()) len = cont.length();
            memcpy(block + offset - blkoff, cont.data(), len);
            if (vedis_kv_store(this->content, key.data(), key.length(), block, VEDIS_CHUNK_SIZE)) {
                // showErr(this->content, __LINE__);
                return write_result::UnknownFailure;
            }
        } else if (__builtin_expect(blkoff + VEDIS_CHUNK_SIZE > offset + cont.length(), 0)) {
            vedis_int64 bufSize = VEDIS_CHUNK_SIZE;
            if (vedis_kv_fetch(this->content, key.data(), key.length(), block, &bufSize))
                memset(block, 0, sizeof(block));

            size_t len = offset + cont.length() - blkoff;
            memcpy(block, cont.data() + blkoff - offset, len);
            if (vedis_kv_store(this->content, key.data(), key.length(), block, VEDIS_CHUNK_SIZE)) {
                // showErr(this->content, __LINE__);
                return write_result::UnknownFailure;
            }
        } else {
            const string_view cv = cont.substr(blkoff - offset, VEDIS_CHUNK_SIZE);
            if (vedis_kv_store(this->content, key.data(), key.length(), cv.data(), cv.length())) {
                // showErr(this->content, __LINE__);
                return write_result::UnknownFailure;
            }
        }
    }
    return write_result::Ok;
}

void Vedis::fetch_content(uint64_t id, size_t offset, size_t len, char *buf) {
    static uint8_t block[VEDIS_CHUNK_SIZE + 5];
    string key;

    size_t fromOffset = offset / VEDIS_CHUNK_SIZE * VEDIS_CHUNK_SIZE;
    for (size_t blkoff = fromOffset; blkoff < offset + len; blkoff += VEDIS_CHUNK_SIZE) {
        key = string_view(reinterpret_cast<char *>(&id), 8);
        key += string_view(reinterpret_cast<char *>(&blkoff), 8);

        vedis_int64 bufSize = VEDIS_CHUNK_SIZE;
        if (__builtin_expect(blkoff < offset, 0)) {
            size_t _len = VEDIS_CHUNK_SIZE - (offset - blkoff);
            if (_len > len) _len = len;

            if (vedis_kv_fetch(this->content, key.data(), key.length(), block, &bufSize))
                memset(buf, 0, _len);
            else
                memcpy(buf, block + offset - blkoff, _len);
        } else if (__builtin_expect(blkoff + VEDIS_CHUNK_SIZE > offset + len, 0)) {
            size_t _len = offset + len - blkoff;

            if (vedis_kv_fetch(this->content, key.data(), key.length(), block, &bufSize))
                memset(buf + blkoff - offset, 0, _len);
            else
                memcpy(buf + blkoff - offset, block, _len);
        } else {
            if (vedis_kv_fetch(this->content, key.data(), key.length(), buf + blkoff - offset,
                               &bufSize))
                memset(buf + blkoff - offset, 0, VEDIS_CHUNK_SIZE);
        }
    }
}

write_result Vedis::remove_content(uint64_t id) {
    vedis_value *result, *entry;
    vedis_exec_fmt(this->content, "SMEMBERS %lu", id);
    vedis_exec_result(this->content, &result);
    while ((entry = vedis_array_next_elem(result)) != NULL) {
        uint64_t blkno = vedis_value_to_int64(entry);

        // cout << "removing " << id << " block " << blkno << endl;

        string key;
        key += string_view(reinterpret_cast<char *>(&id), 8);
        key += string_view(reinterpret_cast<char *>(&blkno), 8);
        vedis_kv_delete(this->content, key.data(), key.length());
    }

    vedis_exec_fmt(this->content, "SCARD %lu", id);
    vedis_exec_result(this->content, &result);
    int card = vedis_value_to_int(result);
    // cout << "card: " << card << endl;
    while (card--) vedis_exec_fmt(this->content, "SPOP %lu", id);

    return write_result::Ok;
}

}  // namespace hermes::backend
