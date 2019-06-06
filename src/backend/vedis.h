//
// Created by icyf on 2019/6/5.
//

#ifndef HERMES_VEDIS_H
#define HERMES_VEDIS_H

#include "hermes.h"
#include "backend/common.h"
#include "backend/vedis/vedis.h"

namespace hermes::backend {
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
        void iterate_directory(const std::string_view &path, F accessor);

    private:
        vedis *metadata;
        vedis *content;

        uint64_t counter;
    };
}

#endif //HERMES_VEDIS_H
