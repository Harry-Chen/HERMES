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

        //std::cout << "iterating " << path << std::endl;

        vedis_value *result, *entry;
        vedis_exec_fmt(this->metadata, "SMEMBERS %s", dkey.data());
        vedis_exec_result(this->metadata, &result);
        while ((entry = vedis_array_next_elem(result)) != NULL) {
            const char *entryStr = vedis_value_to_string(entry, NULL);

            std::string fullPath;
            fullPath += path;
            if (fullPath[fullPath.length() - 1] != '/')
                fullPath += "/";
            fullPath += entryStr;

            //std::cout << "iterated: " << fullPath << std::endl;

            hermes::metadata meta;
            vedis_int64 bufSize = sizeof(hermes::metadata);
            vedis_kv_fetch(this->metadata, fullPath.data(), fullPath.length(), &meta, &bufSize);

            accessor(std::string_view(fullPath.data(), fullPath.length()), meta);
        }
    }

private:
    vedis *metadata;
    vedis *content;

    uint64_t counter;
};

}   // namespace hermes::backend

#endif //HERMES_VEDIS_H
