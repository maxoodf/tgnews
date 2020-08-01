/**
 * @file repository/sqliteClient.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#ifndef TGNEWS_SQLITECLIENT_H
#define TGNEWS_SQLITECLIENT_H

#include <string>
#include <vector>
#include <tuple>

struct sqlite3;

class sqliteClient_t {
public:
    enum class ret_t {
        OK,
        REPLACED,
        FAILED
    };

    explicit sqliteClient_t(const std::string &_fileName);
    ~sqliteClient_t();

    ret_t add(const std::tuple<uint64_t, uint8_t, uint8_t,
            std::string, std::string, std::string,
            uint64_t, uint32_t> &_record,
              std::tuple<uint64_t, uint8_t> &_result) noexcept;

    ret_t get(const std::tuple<uint8_t, uint8_t, uint32_t> &_params,
              std::vector<std::tuple<uint64_t, uint8_t,
                      std::string, std::string, std::string,
                      uint64_t>> &_result) noexcept;
    ret_t get(const std::tuple<uint8_t, uint32_t> &_params,
              std::vector<std::tuple<uint64_t, uint8_t,
                      std::string, std::string, std::string,
                      uint64_t>> &_result) noexcept;
    ret_t get(uint8_t _langID, uint64_t &_id) noexcept;

    ret_t remove(const std::string &_name, std::tuple<uint64_t, uint8_t> &_result) noexcept;
    ret_t remove(uint8_t _langCode, std::vector<uint64_t> &_result) noexcept;

private:
    sqlite3 *m_db = nullptr;

    static std::string escapeLiterals(const std::string &_str);

    bool checkIfExist(const std::string &_name, std::tuple<uint64_t, uint8_t> &_result);
    void getByName(const std::string &_name, std::tuple<uint64_t, uint8_t> &_result);
    void removeByName(const std::string &_name);
    void insert(const std::tuple<uint64_t, uint8_t, uint8_t,
            std::string, std::string, std::string,
            uint64_t, uint32_t> &_record);
    sqliteClient_t::ret_t getHelper(const std::string &_query,
                                    std::vector<std::tuple<uint64_t, uint8_t,
                                            std::string, std::string,
                                            std::string, uint64_t>> &_result) noexcept;
    void getExpired(uint8_t _langCode, std::vector<uint64_t> &_result);
    void removeExpired(uint8_t _langCode);
};

#endif //TGNEWS_SQLITECLIENT_H
