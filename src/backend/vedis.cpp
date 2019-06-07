//
// Created by icyf on 6/5/19.
//

#include "vedis.h"

using namespace std;

const size_t VEDIS_CHUNK_SIZE = 4096;
const char *VEDIS_NEXT_ID_KEY = "#id";

namespace hermes::backend {

Vedis::Vedis(hermes::options opt)
{
    assert(vedis_open(&metadata, opt.metadev) == VEDIS_OK);
    assert(vedis_open(&content, opt.filedev) == VEDIS_OK);

    vedis_int64 bufSize = sizeof(uint64_t);
    if (vedis_kv_fetch(metadata, VEDIS_NEXT_ID_KEY, -1, &counter, &bufSize))
        counter = 0;
}

Vedis::~Vedis()
{
    assert(vedis_close(metadata) == VEDIS_OK);
    assert(vedis_close(content) == VEDIS_OK);
}

uint64_t Vedis::next_id()
{
    uint64_t next = counter++;
    assert(!vedis_kv_store(this->metadata, VEDIS_NEXT_ID_KEY, -1, &counter, sizeof(uint64_t)));
    return next;
}

write_result Vedis::put_metadata(const string_view &path, const hermes::metadata &meta)
{
    if (path.length() > 1) {    // not root
        // A special type of metadata, which is a list of all children
        // of a directory, is represented by ("D"+directory_name).
        auto splitted = split_parent(path);
        std::string dkey = "D";
        dkey += splitted.first;
        auto child = splitted.second;

        // Value length is set to child.length() + 1.
        // This means the separator between entries is either '/' or '\0'.
        if (vedis_kv_fetch_callback(this->metadata, path.data(), path.length(),
                                    [](const void *pData, unsigned int pLen, void *userData) -> int {
                                        return VEDIS_OK;
                                    }, NULL) == VEDIS_NOTFOUND)
            if (vedis_kv_append(this->metadata, dkey.data(), dkey.length(), child.data(), child.length() + 1))
                return write_result::UnknownFailure;
    }
    if (vedis_kv_store(this->metadata, path.data(), path.length(), &meta, sizeof(hermes::metadata)))
        return write_result::UnknownFailure;
    return write_result::Ok;
}

optional<hermes::metadata> Vedis::fetch_metadata(const string_view &path)
{
    hermes::metadata result;
    vedis_int64 bufSize = sizeof(hermes::metadata);

    if (vedis_kv_fetch(this->metadata, path.data(), path.length(), &result, &bufSize))
        return {};
    return {result};
}

optional<hermes::metadata> Vedis::remove_metadata(const string_view &path)
{
    auto fetched = this->fetch_metadata(path);
    if (vedis_kv_delete(this->metadata, path.data(), path.length()))
        return {};

    auto splitted = split_parent(path);
    string dkey = "D";
    dkey += splitted.first;
    if (vedis_kv_delete(this->metadata, dkey.data(), dkey.length()))
        return {};
    return fetched;
}

write_result Vedis::put_content(uint64_t id, size_t offset, const string_view &cont)
{
    static uint8_t block[VEDIS_CHUNK_SIZE + 5];
    string key;

    off_t fromOffset = offset / VEDIS_CHUNK_SIZE * VEDIS_CHUNK_SIZE;
    for (off_t blkoff = fromOffset; blkoff < offset + cont.size(); blkoff += VEDIS_CHUNK_SIZE) {
        key = string_view(reinterpret_cast<char *>(&id), 8);
        key += string_view(reinterpret_cast<char *>(&blkoff), 8);

        if (__builtin_expect(blkoff < offset, 0)) {
            vedis_int64 bufSize = VEDIS_CHUNK_SIZE;
            if (vedis_kv_fetch(this->content, key.data(), key.length(), block, &bufSize))
                memset(block, 0, sizeof(block));

            size_t len = VEDIS_CHUNK_SIZE - (offset - blkoff);
            if (len > cont.length())
                len = cont.length();
            memcpy(block + offset - blkoff, cont.data(), len);
            if (vedis_kv_store(this->content, key.data(), key.length(), block, VEDIS_CHUNK_SIZE))
                return write_result::UnknownFailure;
        }
        else if (__builtin_expect(blkoff + VEDIS_CHUNK_SIZE > offset + cont.length(), 0)) {
            vedis_int64 bufSize = VEDIS_CHUNK_SIZE;
            if (vedis_kv_fetch(this->content, key.data(), key.length(), block, &bufSize))
                memset(block, 0, sizeof(block));

            size_t len = offset + cont.length() - blkoff;
            memcpy(block, cont.data() + blkoff - offset, len);
            if (vedis_kv_store(this->content, key.data(), key.length(), block, VEDIS_CHUNK_SIZE))
                return write_result::UnknownFailure;
        }
        else {
            const string_view cv = cont.substr(blkoff - offset, blkoff - offset + VEDIS_CHUNK_SIZE);
            if (vedis_kv_store(this->content, key.data(), key.length(), cv.data(), cv.length()))
                return write_result::UnknownFailure;
        }
    }
    return write_result::Ok;
}

struct VedisFetchUserData {
    void *dst;
    int srcoff;
    int len;
};

static int fetchCallback(const void *pData, unsigned int pLen, void *_userData)
{
    auto userData = reinterpret_cast<VedisFetchUserData *>(_userData);
    memcpy(userData->dst, pData + userData->srcoff, userData->len);
    return VEDIS_OK;
}

void Vedis::fetch_content(uint64_t id, size_t offset, size_t len, char *buf)
{
    VedisFetchUserData userData;
    string key;

    off_t fromOffset = offset / VEDIS_CHUNK_SIZE * VEDIS_CHUNK_SIZE;
    for (off_t blkoff = fromOffset; blkoff < offset + len; blkoff += VEDIS_CHUNK_SIZE) {
        key = string_view(reinterpret_cast<char *>(&id), 8);
        key += string_view(reinterpret_cast<char *>(&blkoff), 8);

        if (__builtin_expect(blkoff < offset, 0)) {
            size_t _len = VEDIS_CHUNK_SIZE - (offset - blkoff);
            if (_len > len)
                _len = len;

            userData.dst = buf;
            userData.srcoff = offset - blkoff;
            userData.len = _len;

            if (vedis_kv_fetch_callback(this->content, key.data(), key.length(), fetchCallback, &userData))
                memset(buf, 0, len);
        }
        else if (__builtin_expect(blkoff + VEDIS_CHUNK_SIZE > offset + len, 0)) {
            size_t _len = offset + len - blkoff;

            userData.dst = buf + blkoff - offset;
            userData.srcoff = 0;
            userData.len = _len;
            if (vedis_kv_fetch_callback(this->content, key.data(), key.length(), fetchCallback, &userData))
                memset(buf + blkoff - offset, 0, len);
        }
        else {
            vedis_int64 bufSize = VEDIS_CHUNK_SIZE;
            if (vedis_kv_fetch(this->content, key.data(), key.length(), buf + blkoff - offset, &bufSize))
                memset(buf + blkoff - offset, 0, VEDIS_CHUNK_SIZE);
        }
    }
}

write_result Vedis::remove_content(uint64_t id)
{
    // now ignore.
    return write_result::Ok;
}

}   // namespace hermes::backend
