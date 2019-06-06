//
// Created by icyf on 6/5/19.
//

#include "vedis.h"

#include <cassert>
#include <cstring>
#include <string>
#include <string_view>

using namespace std;

const int BASE64_BUF_SIZE = 128;

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

write_result Vedis::put_metadata(const std::string_view &path, const hermes::metadata &meta)
{
    if (!vedis_kv_store(this->metadata, path.data(), path.length(), &meta, sizeof(hermes::metadata)))
        return write_result::Ok;
    else
        return write_result::UnknownFailure;
}

optional<hermes::metadata> Vedis::fetch_metadata(const std::string_view &path)
{
    hermes::metadata result;
    vedis_int64 bufSize = sizeof(hermes::metadata);

    if (!vedis_kv_fetch(this->metadata, path.data(), path.length(), &result, &bufSize))
        return {result};
    else
        return {};
}

optional<hermes::metadata> Vedis::remove_metadata(const std::string_view &path)
{
    auto fetched = this->fetch_metadata(path);
    if (!vedis_kv_delete(this->metadata, path.data(), path.length()))
        return fetched;
    else
        return {};
}

write_result Vedis::put_content(uint64_t id, size_t offset, const std::string_view &cont)
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

            int len = VEDIS_CHUNK_SIZE - (offset - blkoff);
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

            int len = offset + cont.length() - blkoff;
            memcpy(block, cont.data() + blkoff - offset, len);
            if (vedis_kv_store(this->content, key.data(), key.length(), block, VEDIS_CHUNK_SIZE))
                return write_result::UnknownFailure;
        }
        else {
            const string_view cv = content.substr(blkoff - offset, blkoff - offset + DB_CHUNK_SIZE);
            if (vedis_kv_store(this->content, key.data(), key.length(), cv.data(), cv.length()))
                return write_result::UnknownFailure;
        }
    }
    return write_result::Ok;
}

void Vedis::fetch_content(uint64_t id, size_t offset, size_t len, char *buf)
{
    string key;

    off_t fromOffset = offset / VEDIS_CHUNK_SIZE * VEDIS_CHUNK_SIZE;
    for (off_t blkoff = fromOffset; blkoff < offset + cont.size(); blkoff += VEDIS_CHUNK_SIZE) {
        key = string_view(reinterpret_cast<char *>(&id), 8);
        key += string_view(reinterpret_cast<char *>(&blkoff), 8);

        if (__builtin_expect(blkoff < offset, 0)) {
            int len = VEDIS_CHUNK_SIZE - (offset - blkoff);
            if (len > cont.length())
                len = cont.length();

            auto xCallback = [&](void *pData, int iDataLen, void *userData) -> int {
                memcpy(buf, pData + offset - blkoff, len);
            };
            if (vedis_kv_fetch_callback(this->content, key.data(), key.length(), xCallback, NULL))
                memset(buf, 0, len);
        }
        else if (__builtin_expect(blkoff + VEDIS_CHUNK_SIZE > offset + cont.length(), 0)) {
            int len = offset + cont.length() - blkoff;
            auto xCallback = [&](void *pData, int iDataLen, void *userData) -> int {
                memcpy(buf + blkoff - offset, pData, len);
            };
            if (vedis_kv_fetch_callback(this->content, key.data(), key.length(), xCallback, NULL))
                memset(buf + blkoff - offset, 0, len);
        }
        else {
            vedis_int64 bufSize = VEDIS_CHUNK_SIZE;
            if (vedis_kv_fetch(this->content, key.data(), key.length(), buf + blkoff - offset, &bufSize))
                memset(buf + blkoff - offset, 0, VEDIS_CHUNK_SIZE);
        }
    }
}

}   // namespace hermes::backend
