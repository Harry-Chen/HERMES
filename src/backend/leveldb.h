#ifndef __BACKEND_LEVELDB_h__
#define __BACKEND_LEVELDB_h__

#include "backend/common.h"
#include "hermes.h"

#ifndef _ROCKS_DB_
#include <leveldb/comparator.h>
#include <leveldb/db.h>
#else
#include <rocksdb/comparator.h>
#include <rocksdb/db.h>
#endif

#include <iostream>
#include <memory>
#include <optional>
#include <string_view>

namespace hermes::backend {
class PathComparator : public leveldb::Comparator {
    int Compare(const leveldb::Slice &a, const leveldb::Slice &b) const {
        auto ad = a.data();
        auto bd = b.data();
        for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
            if (ad[i] != bd[i]) {
                if (ad[i] == '/')
                    return -1;
                else if (bd[i] == '/')
                    return 1;
                return ad[i] < bd[i] ? -1 : 1;
            }
        }

        if (a.size() == b.size()) return 0;
        return a.size() < b.size() ? -1 : 1;
    }

    const char *Name() const { return "PathComparator"; }

    void FindShortestSeparator(std::string *start, const leveldb::Slice &limit) const { return; }

    void FindShortSuccessor(std::string *key) const { return; }
};

class LDB {
   public:
    LDB(hermes::options opts);

    ~LDB();

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
        std::unique_ptr<leveldb::Iterator> it(this->metadata->NewIterator(leveldb::ReadOptions()));
        const leveldb::Slice pathSlice(path.data(), path.length());
        it->Seek(pathSlice);  // Ignores self
        if (path != "/") it->Next();

        for (; it->Valid();) {
            auto key = it->key();
            std::string_view view(key.data(), key.size());
            // std::cout<<">>> Current:"<<view<<std::endl;

            if (view.size() <= path.size() || view.compare(0, path.size(), path) != 0) break;

            if (path != "/" && view[path.size()] != '/') break;

            // Now we guarantee to skip subdirectories, so this check is not needed anymore
            /*
            if(view.find_first_of('/', path.size() + 1) != std::string::npos)
              continue;
            */

            auto mptr = reinterpret_cast<const hermes::metadata *>(it->value().data());

            accessor(view, *mptr);

            if (mptr->is_dir()) {
                // Skip the entire subdirectory
                std::string slug(view);
                slug.append("/\x7F");
                // std::cout<<"Jumping to "<<slug<<std::endl;
                it->Seek(slug);
            } else {
                it->Next();
            }
        }
        return;
    }

   private:
    leveldb::DB *metadata;
    leveldb::DB *content;

    uint64_t counter;
};
}  // namespace hermes::backend

#endif  // __BACKEND_LEVELDB_h__
