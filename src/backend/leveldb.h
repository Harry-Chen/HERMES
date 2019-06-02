#ifndef __BACKEND_LEVELDB_h__
#define __BACKEND_LEVELDB_h__

#include "hermes.h"
#include "backend/common.h"

#include <leveldb/db.h>
#include <optional>
#include <string_view>

namespace hermes::backend {
  class LDB {
    public:
      LDB(hermes::options opts);
      ~LDB();

      write_result put_metadata(const std::string_view &path, const hermes::metadata &metadata);
      std::optional<hermes::metadata> fetch_metadata(const std::string_view &path);

      write_result put_content(const std::string_view &path, const std::string_view &content);
      std::optional<std::string> fetch_content(const std::string_view &path);

      /**
       * Iterate though a directory
       *
       * Caller should ensure that path ends with a slash
       * F: (const std::string_view &path, const hermes::metadata &metadata) -> void
       */
      template<typename F>
      void iterate_director(const std::string_view &path, F accessor);

    private:
      leveldb::DB *metadata;
      leveldb::DB *content;
  };
}

#endif // __BACKEND_LEVELDB_h__
