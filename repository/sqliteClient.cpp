/**
 * @file repository/sqliteClient.cpp
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#include <unistd.h>

#include <iostream>

#include <sqlite3.h>

#include "sqliteClient.h"

sqliteClient_t::sqliteClient_t(const std::string &_fileName) {
    char buf[4096];
    if (getcwd(buf, 4096) == nullptr) {
        throw std::runtime_error("path is too long");
    }
    std::string absFileName = std::string(buf) + "/" + _fileName;
    auto ret = sqlite3_open_v2(absFileName.c_str(),
                               &m_db,
                               SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,
                               nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error(absFileName + " - " + sqlite3_errmsg(m_db));
    }

    std::string query = "PRAGMA journal_mode=WAL;";
//    std::string query = "PRAGMA journal_mode=OFF;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }

    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (ret != SQLITE_ROW) {
        throw std::runtime_error("failed to initialize DB;");
    }
}

sqliteClient_t::~sqliteClient_t() {
    if (m_db != nullptr) {
        sqlite3_close(m_db);
    }
}

std::string sqliteClient_t::escapeLiterals(const std::string &_str) {
    auto str = _str;
    std::size_t pos = 0;
    while (true) {
        pos = str.find('\'', pos);
        if (pos == std::string::npos) {
            break;
        }
        str.replace(pos, 1, R"('')");
        pos += 2;
    }

    return str;
}

bool sqliteClient_t::checkIfExist(const std::string &_name, std::tuple<uint64_t, uint8_t> &_result) {
    std::string query = "SELECT vector_id, lang_id FROM attributes WHERE name = '"
                        + escapeLiterals(_name) + "';";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }

    switch (sqlite3_step(stmt)) {
        case SQLITE_ROW: {
            std::get<0>(_result) = sqlite3_column_int64(stmt, 0);
            std::get<1>(_result) = sqlite3_column_int(stmt, 1);;
            sqlite3_finalize(stmt);
            return true;
        }
        case SQLITE_DONE: {
            sqlite3_finalize(stmt);
            return false;
        }
        default: {
            sqlite3_finalize(stmt);
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }
    }
}

void sqliteClient_t::getByName(const std::string &_name, std::tuple<uint64_t, uint8_t> &_result) {
    std::string query = "SELECT vector_id, lang_id FROM attributes WHERE name = '"
                        + escapeLiterals(_name) + "';";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }

    while (true) {
        switch (sqlite3_step(stmt)) {
            case SQLITE_ROW: {
                _result = std::make_tuple(
                        sqlite3_column_int64(stmt, 0),
                        sqlite3_column_int(stmt, 1));
                continue;
            }
            case SQLITE_DONE: {
                sqlite3_finalize(stmt);
                return;
            }
            default: {
                sqlite3_finalize(stmt);
                throw std::runtime_error(sqlite3_errmsg(m_db));
            }
        }
    }
}

void sqliteClient_t::removeByName(const std::string &_name) {
    std::string query = "DELETE FROM attributes WHERE name = '"
                        + escapeLiterals(_name) + "'; COMMIT;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }

    switch (sqlite3_step(stmt)) {
        case SQLITE_DONE: {
            sqlite3_finalize(stmt);
            return;
        }
        default: {
            sqlite3_finalize(stmt);
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }
    }
}

void sqliteClient_t::insert(const std::tuple<uint64_t, uint8_t, uint8_t,
        std::string, std::string, std::string,
        uint64_t, uint32_t> &_record) {
    std::string query = "INSERT INTO attributes VALUES ("
                        + std::to_string(std::get<0>(_record)) + ","
                        + std::to_string(std::get<1>(_record)) + ","
                        + std::to_string(std::get<2>(_record)) + ",'"
                        + escapeLiterals(std::get<3>(_record)) + "','"
                        + escapeLiterals(std::get<4>(_record)) + "','"
                        + escapeLiterals(std::get<5>(_record)) + "',"
                        + std::to_string(std::get<6>(_record)) + ","
                        + std::to_string(std::get<7>(_record)) + "); COMMIT;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }

    switch (sqlite3_step(stmt)) {
        case SQLITE_DONE: {
            sqlite3_finalize(stmt);
            return;
        }
        default: {
            sqlite3_finalize(stmt);
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }
    }
}

void sqliteClient_t::getExpired(uint8_t _langCode, std::vector<uint64_t> &_result) {
    std::string query = "SELECT vector_id "
                        "FROM attributes "
                        "WHERE published < "
                        "(SELECT MAX(published) "
                        "FROM attributes "
                        "WHERE lang_id = " + std::to_string(_langCode)
                        + ") - ttl AND lang_id = " + std::to_string(_langCode) + ";";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }

    while (true) {
        switch (sqlite3_step(stmt)) {
            case SQLITE_ROW: {
                _result.emplace_back(sqlite3_column_int64(stmt, 0));
                continue;
            }
            case SQLITE_DONE: {
                sqlite3_finalize(stmt);
                return;
            }
            default: {
                sqlite3_finalize(stmt);
                throw std::runtime_error(sqlite3_errmsg(m_db));
            }
        }
    }
}

void sqliteClient_t::removeExpired(uint8_t _langCode) {
    std::string query = "DELETE FROM attributes "
                        "WHERE published < "
                        "(SELECT MAX(published) "
                        "FROM attributes "
                        "WHERE lang_id = " + std::to_string(_langCode)
                        + ") - ttl AND lang_id = " + std::to_string(_langCode) + ";";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }

    switch (sqlite3_step(stmt)) {
        case SQLITE_DONE: {
            sqlite3_finalize(stmt);
            return;
        }
        default: {
            sqlite3_finalize(stmt);
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }
    }
}

sqliteClient_t::ret_t sqliteClient_t::add(const std::tuple<uint64_t, uint8_t, uint8_t,
        std::string, std::string, std::string,
        uint64_t, uint32_t> &_record,
                                          std::tuple<uint64_t, uint8_t> &_result) noexcept {
    ret_t ret;
    try {
        {
            if (checkIfExist(std::get<3>(_record), _result)) {
                removeByName(std::get<3>(_record));
                ret = ret_t::REPLACED;
            } else {
                ret = ret_t::OK;
            }
        }
        insert(_record);
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
        ret = ret_t::FAILED;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        ret = ret_t::FAILED;
    }

    return ret;
}

sqliteClient_t::ret_t sqliteClient_t::getHelper(const std::string &_query,
                                                std::vector<std::tuple<uint64_t, uint8_t,
                                                        std::string, std::string,
                                                        std::string, uint64_t>> &_result) noexcept {
    sqlite3_stmt *stmt = nullptr;
    try {
        if (sqlite3_prepare_v2(m_db, _query.c_str(), _query.length(), &stmt, nullptr) != SQLITE_OK) {
            std::cerr << sqlite3_errmsg(m_db) << std::endl;
            return ret_t::FAILED;
        }

        while (true) {
            switch (sqlite3_step(stmt)) {
                case SQLITE_ROW: {
                    _result.emplace_back(std::make_tuple(
                            sqlite3_column_int64(stmt, 0),
                            sqlite3_column_int(stmt, 1),
                            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
                            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
                            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)),
                            sqlite3_column_int64(stmt, 5)));
                    continue;
                }
                case SQLITE_DONE: {
                    sqlite3_finalize(stmt);
                    return ret_t::OK;
                }
                case SQLITE_ERROR: {
                    std::cerr << sqlite3_errmsg(m_db) << std::endl;
                    break;
                }
                default: {
                    std::cerr << "unknown m_db error" << std::endl;
                    break;
                }
            }
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    sqlite3_finalize(stmt);
    return ret_t::FAILED;
}

sqliteClient_t::ret_t sqliteClient_t::get(const std::tuple<uint8_t, uint8_t, uint32_t> &_params,
                                          std::vector<std::tuple<uint64_t, uint8_t,
                                                  std::string, std::string, std::string,
                                                  uint64_t>> &_result) noexcept {
    try {
        std::string query =
                "SELECT vector_id, category_id, name, title, site, published "
                "FROM attributes WHERE lang_id = "
                + std::to_string(std::get<0>(_params)) + " AND category_id = "
                + std::to_string(std::get<1>(_params)) + " AND published >= (SELECT MAX(published) - "
                + std::to_string(std::get<2>(_params)) + " FROM attributes WHERE lang_id = "
                + std::to_string(std::get<0>(_params)) + ");";

        return getHelper(query, _result);
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    return ret_t::FAILED;
}

sqliteClient_t::ret_t sqliteClient_t::get(const std::tuple<uint8_t, uint32_t> &_params,
                                          std::vector<std::tuple<uint64_t, uint8_t,
                                                  std::string, std::string,
                                                  std::string, uint64_t>> &_result) noexcept {
    try {
        std::string query =
                "SELECT vector_id, category_id, name, title, site, published "
                "FROM attributes WHERE lang_id = "
                + std::to_string(std::get<0>(_params)) + " AND published >= (SELECT MAX(published) - "
                + std::to_string(std::get<1>(_params)) + " FROM attributes WHERE lang_id = "
                + std::to_string(std::get<0>(_params)) + ");";

        return getHelper(query, _result);
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    return ret_t::FAILED;
}

sqliteClient_t::ret_t sqliteClient_t::get(uint8_t _langCode, uint64_t &_id) noexcept {
    sqlite3_stmt *stmt = nullptr;
    try {
        std::string query = "SELECT MAX(vector_id) FROM attributes WHERE lang_id = "
                            + std::to_string(_langCode) + ";";
        if (sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, nullptr) != SQLITE_OK) {
            std::cerr << sqlite3_errmsg(m_db) << std::endl;
            return ret_t::FAILED;
        }

        while (true) {
            switch (sqlite3_step(stmt)) {
                case SQLITE_ROW: {
                    _id = sqlite3_column_int64(stmt, 0);
                    sqlite3_finalize(stmt);
                    return ret_t::OK;
                }
                case SQLITE_DONE: {
                    _id = 0;
                    sqlite3_finalize(stmt);
                    return ret_t::OK;
                }
                default: {
                    std::cerr << sqlite3_errmsg(m_db) << std::endl;
                    break;
                }
            }
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    sqlite3_finalize(stmt);
    return ret_t::FAILED;
}

sqliteClient_t::ret_t sqliteClient_t::remove(const std::string &_name,
                                             std::tuple<uint64_t, uint8_t> &_result) noexcept {
    try {
        getByName(_name, _result);
        removeByName(_name);

        return ret_t::OK;
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    return ret_t::FAILED;
}

sqliteClient_t::ret_t sqliteClient_t::remove(uint8_t _langCode, std::vector<uint64_t> &_result) noexcept {
    try {
        getExpired(_langCode, _result);
        removeExpired(_langCode);

        return ret_t::OK;
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    return ret_t::FAILED;
}
