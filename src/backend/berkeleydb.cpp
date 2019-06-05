#include "berkeleydb.h"

const char *COUNTER_KEY = "\xca\xfe";

int compare_path(Db *_dbp, const Dbt *a, const Dbt *b, size_t *) {
    auto ad = (char *)a->get_data();
    auto bd = (char *)b->get_data();
    for (size_t i = 0; i < a->get_size() && i < b->get_size(); ++i) {
        if (ad[i] != bd[i]) {
            if (ad[i] == '/')
                return -1;
            else if (bd[i] == '/')
                return 1;
            return ad[i] < bd[i] ? -1 : 1;
        }
    }

    if (a->get_size() == b->get_size()) return 0;
    return a->get_size() < b->get_size() ? -1 : 1;
}

namespace hermes::backend {
BDB::BDB(hermes::options opts) {
    metadata = new Db(nullptr, 0);
    content = new Db(nullptr, 0);

    metadata->set_bt_compare(compare_path);

    metadata->open(nullptr, opts.filedev, "hermes", DB_BTREE, DB_CREATE, 0755);
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
    Dbt data((void *)&storing, sizeof(storing));

    metadata->put(nullptr, &key, &data, 0);

    return next;
}
write_result BDB::put_content(uint64_t id, size_t offset, const std::string_view &content) {
    // TODO: chunking
    uint64_t key_id = htobe64(id);
    Dbt key((void *)&key_id, sizeof(key_id));
    Dbt value;
    if (this->content->get(nullptr, &key, &value, 0) == 0) {
        // update
        size_t new_size = std::max((size_t)value.get_size(), offset + content.size());
        char *buffer = new char[new_size];
        memcpy(buffer, value.get_data(), value.get_size());
        memcpy(&buffer[offset], content.data(), content.size());

        Dbt insert_key((void *)&id, sizeof(id));
        Dbt insert_value(buffer, offset + content.size());
        this->content->put(nullptr, &insert_key, &insert_value, 0);

        delete [] buffer;
        return write_result::Ok;
    } else {
        // new
        char *buffer = new char[offset + content.size()];
        memcpy(&buffer[offset], content.data(), content.size());

        Dbt insert_key((void *)&id, sizeof(id));
        Dbt insert_value(buffer, offset + content.size());
        this->content->put(nullptr, &insert_key, &insert_value, 0);

        delete [] buffer;
        return write_result::Ok;
    }
}
void BDB::fetch_content(uint64_t id, size_t offset, size_t len, char *buf) {
    uint64_t key_id = htobe64(id);
    Dbt key((void *)&key_id, sizeof(key_id));
    Dbt value;
    value.set_data(buf);
    value.set_doff(offset);
    value.set_ulen(len);
    this->content->get(nullptr, &key, &value, 0);
}
write_result BDB::put_metadata(const std::string_view &path, const hermes::metadata &metadata) {
    Dbt key((void *)path.data(), path.size());
    Dbt value((void *)&metadata, sizeof(metadata));

    if (this->metadata->put(nullptr, &key, &value, 0) == 0) {
        return write_result::Ok;
    } else {
        return write_result::UnknownFailure;
    }
}
std::optional<hermes::metadata> BDB::fetch_metadata(const std::string_view &path) {
    Dbt key((void *)path.data(), path.size());
    Dbt value;

    if (metadata->get(nullptr, &key, &value, 0) == DB_NOTFOUND) {
        return {};
    } else {
        hermes::metadata result;
        memcpy(&result, value.get_data(), value.get_size());
        return {result};
    }
}
std::optional<hermes::metadata> BDB::remove_metadata(const std::string_view &path) {
    auto data = fetch_metadata(path);

    if (!data) return data;

    Dbt key((void *)path.data(), path.size());
    if (metadata->del(nullptr, &key, 0) == 0) {
        return data;
    } else {
        // TODO: log error
        return {};
    }
}
BDB::~BDB() {
    metadata->close(0);
    content->close(0);
    delete metadata;
    delete content;
}
}  // namespace hermes::backend
