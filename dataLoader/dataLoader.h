/**
 * @file dataLoader/dataLoader.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_DATALOADER_H
#define TGNEWS_DATALOADER_H

#include <mutex>
#include <unordered_set>

#include "types.h"
#include "textExtractor.h"

class dataLoader_t final {
private:
    langDocSet_t m_langDocSet;
    std::mutex m_mtx;
    bool m_allLangs = false;

public:
    dataLoader_t(uint8_t _threads,
                 const std::vector<std::string> &_langs,
                 char  *const *_path,
                 bool _allLangs = false);

    static bool loadFile(const std::string &_fileName, std::vector<uint8_t> &_data) noexcept;
    const langDocSet_t &langDocSet() noexcept {return m_langDocSet;}

private:
    void worker(uint8_t _thrID, uint8_t _threads,
                const std::vector<std::pair<std::string, std::string>> &_fileNames) noexcept;
};

#endif //TGNEWS_DATALOADER_H
