/**
 * @file categoryCluster/categoryCluster.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_CATEGORYCLUSTER_H
#define TGNEWS_CATEGORYCLUSTER_H

#include <mutex>

#include "types.h"

class categoryCluster_t {
public:
    categoryCluster_t(uint8_t _threads,
                      const std::unordered_map<std::string, std::string> &_categoryLangModelFileNames,
                      const std::unordered_map<categories_t, std::string> &_categoryNames,
                      const langVecSet_t &_langVecSet);

    const groupSet_t &groupSet() noexcept {return m_groupSet;}

private:
    groupSet_t m_groupSet;
    std::mutex m_mtx;

    void worker(uint8_t _thrID,
                uint8_t _threads,
                const std::string &_lang,
                const std::string &_categoryLangModelFileName,
                const std::vector<std::string> &_fileNames,
                const std::vector<std::vector<float>> &_vectors) noexcept;
};

#endif //TGNEWS_CATEGORYCLUSTER_H
