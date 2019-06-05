#include "berkeleydb.h"

const char* COUNTER_KEY = "\xca\xfe";

namespace hermes::backend {
BDB::BDB(hermes::options opts) {
    metadata = new Db(nullptr, 0);
    content = new Db(nullptr, 0);

    metadata->open(nullptr, opts.metadev, "hermes", DB_BTREE, DB_CREATE, 0755);
    content->open(nullptr, opts.metadev, "hermes", DB_BTREE, DB_CREATE, 0755);

    Dbt key((void *)COUNTER_KEY, strlen(COUNTER_KEY));
    Dbt data;
    data.set_data((void *)&counter);
    data.set_ulen(sizeof(counter));
    data.set_flags(DB_DBT_USERMEM);
    if (metadata->get(nullptr, &key, &data, 0) == DB_NOTFOUND) {
        counter = 0;
    } else {
        counter = be64toh(counter);
    }
}
uint64_t BDB::next_id() {
    uint64_t next = counter++;
    uint64_t storing = htobe64(counter);

    Dbt key((void *)COUNTER_KEY, strlen(COUNTER_KEY));
    Dbt data;
    data.set_data((void *)&counter);
    data.set_ulen(sizeof(counter));
    data.set_flags(DB_DBT_USERMEM);

    metadata->put(nullptr, &key, &data, 0);

    return next;
}
write_result BDB::put_content(uint64_t id, size_t offset, const std::string_view& content) {}
void BDB::fetch_content(uint64_t id, size_t offset, size_t len, char* buf) {}
write_result BDB::put_metadata(const std::string_view& path, const hermes::metadata& metadata) {}
std::optional<hermes::metadata> BDB::fetch_metadata(const std::string_view& path) {}
std::optional<hermes::metadata> BDB::remove_metadata(const std::string_view& path) {}
BDB::~BDB() {
    metadata->close(0);
    content->close(0);
    delete metadata;
    delete content;
}
}  // namespace hermes::backend
