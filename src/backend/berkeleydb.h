#ifndef __BACKEND_BERKELEYDB_h__
#define __BACKEND_BERKELEYDB_h__

#include <db_cxx.h>
#include "backend/common.h"
#include "hermes.h"

#include <iostream>
#include <memory>
#include <optional>
#include <string_view>

namespace hermes::backend {
class BDB {
   public:
    BDB(hermes::options opts);

    ~BDB();

    /**
     * Return next file id
     */
    uint64_t next_id();

    write_result put_metadata(const std::string_view &path, const hermes::metadata &metadata);

    std::optional<hermes::metadata> fetch_metadata(const std::string_view &path);

    std::optional<hermes::metadata> remove_metadata(const std::string_view &path);

    write_result put_content(uint64_t id, size_t offset, const std::string_view &content);

    /**
     * For all content chunks that are not found, we assume they are holes in files.
     * So this call will always return something.
     */
    void fetch_content(uint64_t id, size_t offset, size_t len, char *buf);

    /**
     * Now we do not need to use remove_content to rename files. So we are returning a single result
     * here
     */
    write_result remove_content(uint64_t id);

    /**
     * Iterate though a directory
     *
     * Caller should ensure that path ends with a slash
     * F: (const std::string_view &path, const hermes::metadata &metadata) -> void
     */
    template <typename F>
    inline void iterate_directory(const std::string_view &path, F accessor) {
        Dbc *dbc = nullptr;
        metadata->cursor(nullptr, &dbc, 0);
        Dbt pathKey((void *)path.data(), path.size());
        pathKey.set_flags(DB_DBT_MALLOC);
        Dbt pathData;
        pathData.set_flags(DB_DBT_MALLOC);
        dbc->get(&pathKey, &pathData, DB_SET);

        Dbt key;
        key.set_flags(DB_DBT_MALLOC);
        Dbt value;
        value.set_flags(DB_DBT_MALLOC);
        if (path != "/") {
            dbc->get(&key, &value, DB_NEXT);
        } else {
            dbc->get(&key, &value, DB_FIRST);
        }
        do {
            std::string_view view((char *)key.get_data(), key.get_size());
            if (view.size() <= path.size() || view.compare(0, path.size(), path) != 0) break;

            if (path != "/" && view[path.size()] != '/') break;

            auto mptr = reinterpret_cast<const hermes::metadata *>(value.get_data());

            accessor(view, *mptr);

            if (mptr->is_dir()) {
                // Skip the entire subdirectory
                std::string slug(view);
                slug.append("/\x7F");
                Dbt slugKey((void *)slug.data(), slug.size());
                Dbt slugData;
                dbc->get(&slugKey, &slugData, DB_SET_RANGE);
                continue;
            }
        } while (dbc->get(&key, &value, DB_NEXT) == 0);
        dbc->close();
    }

   private:
    DbEnv *metadataEnv;
    Db *metadata;
    DbEnv *contentEnv;
    Db *content;

    uint64_t counter;
};
}  // namespace hermes::backend

#endif  // __BACKEND_BERKELEYDB_h__
