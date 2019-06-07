//
// Created by icyf on 2019/6/5.
//

#ifndef HERMES_VEDIS_H
#define HERMES_VEDIS_H

#include "hermes.h"
#include "backend/common.h"
#include "vedis/vedis.h"

#include <cstring>
#include <cassert>
#include <string>
#include <iostream>

namespace hermes::backend {

static inline auto split_parent(const std::string_view &path)
{
    size_t pEnd = path.length() - 1;
    if (path[pEnd] == '/')
        --pEnd;
    size_t pos = path.find_last_of('/', pEnd);
    return make_pair(path.substr(0, (pos ? pos : 1)), path.substr(pos + 1, pEnd - pos));
}

template <typename F>
struct VedisUserData {
    char buffer[1024];
    int pos;

    vedis *that;
    std::string_view path;
    F *accessor;
};

template <typename F>
static inline int iterateCallback(const void *pData, unsigned int pLen, void *_userData) {
    auto userData = reinterpret_cast<VedisUserData<F> *>(_userData);
    auto str = reinterpret_cast<const char *>(pData);
    unsigned int i = 0, j;
    while (i < pLen) {
        for (j = i + 1; j < pLen && str[j] && str[j] != '/'; j++);
        if (j == pLen) {
            memcpy(userData->buffer, str + i, (userData->pos = j - i));
            break;
        }
        std::string fullPath;
        fullPath += userData->path;
        if (fullPath[fullPath.length() - 1] != '/')
            fullPath += "/";
        if (userData->pos) {
            fullPath += std::string_view(userData->buffer, userData->pos);
            userData->pos = 0;
        }
        fullPath += std::string_view(str + i, j - i);

        hermes::metadata meta;
        vedis_int64 bufSize = sizeof(hermes::metadata);
        vedis_kv_fetch(userData->that, fullPath.data(), fullPath.length(), &meta, &bufSize);

        (*(userData->accessor))(std::string_view(fullPath.data(), fullPath.length()), meta);

        i = j + 1;
    }
    return VEDIS_OK;
}

class Vedis {
public:
    explicit Vedis(hermes::options);
    ~Vedis();

    uint64_t next_id();
    write_result put_metadata(const std::string_view &path, const hermes::metadata &meta);
    std::optional<hermes::metadata> fetch_metadata(const std::string_view &path);
    std::optional<hermes::metadata> remove_metadata(const std::string_view &path);

    write_result put_content(uint64_t id, size_t offset, const std::string_view &cont);
    void fetch_content(uint64_t id, size_t offset, size_t len, char *buf);
    write_result remove_content(uint64_t id);

    template <typename F>
    inline void iterate_directory(const std::string_view &path, F accessor)
    {
        std::string dkey = "D";
        dkey += path;

        VedisUserData<F> userData;
        userData.that = this->metadata;
        userData.path = path;
        userData.accessor = &accessor;

        vedis_kv_fetch_callback(this->metadata, dkey.data(), dkey.length(), iterateCallback<F>, &userData);
    }

private:
    vedis *metadata;
    vedis *content;

    uint64_t counter;
};

}   // namespace hermes::backend

#endif //HERMES_VEDIS_H
