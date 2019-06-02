#ifndef __BACKEND_LEVELDB_h__
#define __BACKEND_LEVELDB_h__

#include "hermes.h"
#include "backend/common.h"

#include <leveldb/db.h>
#include <optional>
#include <string_view>
#include <memory>
#include <iostream>

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
      inline
      void iterate_directory(const std::string_view &path, F accessor) {
        std::unique_ptr<leveldb::Iterator> it(this->metadata->NewIterator(leveldb::ReadOptions()));
        const leveldb::Slice pathSlice(path.data(), path.length());
        it->Seek(pathSlice); // Ignores self
        if(path != "/")
          it->Next();

        for(; it->Valid(); it->Next()) {
          std::cout<<">> At: "<<it->key().ToString()<<std::endl;

          auto key = it->key();
          std::string_view view(key.data(), key.size());

          if(view.size() <= path.size() || view.compare(0, path.size(), path) != 0)
            break;

          if(path != "/" && view[path.size()] != '/')
            break;

          if(view.find_first_of('/', path.size() + 1) != std::string::npos)
            continue;

          accessor(view, *reinterpret_cast<const hermes::metadata *>(it->value().data()));
        }
        return;
      }

    private:
      leveldb::DB *metadata;
      leveldb::DB *content;
  };
}

#endif // __BACKEND_LEVELDB_h__
