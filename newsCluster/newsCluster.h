/**
 * @file newsCluster/newsCluster.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_NEWSCLUSTER_H
#define TGNEWS_NEWSCLUSTER_H

#include <memory>
#include <mutex>
#include <map>

#include "types.h"

class embedder_t;

class newsCluster_t {
public:
    newsCluster_t(uint8_t _threads,
                  const std::unordered_map<std::string, std::string> &_w2vLangModelFileNames,
                  const std::unordered_map<std::string, std::string> &_newsLangModelFileNames,
                  const langDocSet_t &_langDocSet);
    ~newsCluster_t();

    newsCluster_t(const newsCluster_t &) = delete;
    void operator=(const newsCluster_t &) = delete;
    newsCluster_t(const newsCluster_t &&) = delete;
    void operator=(const newsCluster_t &&) = delete;

    const langVecSet_t &langVecSet() noexcept {return m_langVecSet;}

private:
    langVecSet_t m_langVecSet;
    std::mutex m_mtx;
    std::map<std::string, std::unique_ptr<embedder_t>> m_embedder;

    void worker(uint8_t _thrID,
                uint8_t _threads,
                const std::string &_langCode,
                const std::string &_newsLangModelFileName,
                const std::vector<std::string> &_fileNames,
                const std::vector<document_t> &_documents) noexcept;
};

#endif //TGNEWS_NEWSCLUSTER_H
