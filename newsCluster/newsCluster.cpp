/**
 * @file newsCluster/newsCluster.cpp
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <thread>

#include "embedder/embedder.h"
#include "newsDetector/newsDetector.h"
#include "newsCluster.h"

newsCluster_t::newsCluster_t(uint8_t _threads,
                             const std::unordered_map<std::string, std::string> &_w2vLangModelFileNames,
                             const std::unordered_map<std::string, std::string> &_newsLangModelFileNames,
                             const langDocSet_t &_langDocSet) {
    for (const auto &wm:_w2vLangModelFileNames) {
        m_embedder.emplace(wm.first, std::make_unique<embedder_t>(wm.second));

        m_langVecSet.emplace(wm.first, vecSet_t());

        auto newsLangModelItr = _newsLangModelFileNames.find(wm.first);
        if (newsLangModelItr == _newsLangModelFileNames.end()) {
            throw std::runtime_error("news detection model file is not defined for language \"" + wm.first + "\"");
        }

        auto langDocs = _langDocSet.find(wm.first);
        if (langDocs == _langDocSet.end()) {
            throw std::runtime_error("w2v model file is not defined for language \"" + wm.first + "\"");
        }

        std::vector<std::string> fileNames;
        std::vector<document_t> documents;
        for (const auto &d:langDocs->second) {
            fileNames.emplace_back(d.first);
            documents.emplace_back(d.second);
        }

        std::vector<std::thread> thrPool;
        uint8_t workers = documents.empty()?
                          0:
                          ((documents.size() < _threads)?
                           documents.size():
                           _threads);
        for (int i = 0; i < workers; ++i) {
            thrPool.emplace_back(std::thread(&newsCluster_t::worker, this, i, workers,
                                             std::cref(wm.first),
                                             std::cref(newsLangModelItr->second),
                                             std::cref(fileNames),
                                             std::cref(documents)));
        }
        for (auto &i:thrPool) {
            i.join();
        }
    }
}

newsCluster_t::~newsCluster_t() = default;

void newsCluster_t::worker(uint8_t _thrID,
                           uint8_t _threads,
                           const std::string &_langCode,
                           const std::string &_newsLangModelFileName,
                           const std::vector<std::string> &_fileNames,
                           const std::vector<document_t> &_documents) noexcept {
    std::size_t textsPerThread = _documents.size() / _threads;
    std::size_t startFrom = _thrID * textsPerThread;
    std::size_t stopAt = ((_thrID == _threads - 1)?_documents.size():startFrom + textsPerThread);

    try {
        std::vector<std::vector<float>> vectors;
        auto emi = m_embedder.find(_langCode);
        if (emi == m_embedder.end()) {
            std::cerr << "no embedding model for language " << _langCode << std::endl;
            return;
        }
        (*emi->second)(_documents, startFrom, stopAt, vectors);

        std::vector<bool> newsFlags;
        newsDetector_t newsDetector(_newsLangModelFileName);
        newsDetector(vectors, 0, vectors.size(), newsFlags);

        std::unique_lock<std::mutex> lck(m_mtx);
        for (std::size_t i = 0; i < vectors.size(); ++i) {
            if (newsFlags[i]) {
                m_langVecSet.at(_langCode).emplace(_fileNames[i + startFrom], vectors[i]);
            }
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }
}
