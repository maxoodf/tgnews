/**
 * @file categoryCluster/categoryCluster.cpp
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <thread>

#include "categorizer/categorizer.h"
#include "categoryCluster.h"

categoryCluster_t::categoryCluster_t(uint8_t _threads,
                                     const std::unordered_map<std::string, std::string> &_categoryLangModelFileNames,
                                     const std::unordered_map<categories_t, std::string> &_categoryNames,
                                     const langVecSet_t &_langVecSet) {
    for (const auto &cn:_categoryNames) {
        m_groupSet.emplace(cn.first, std::unordered_map<std::string, std::string>());
    }

    for (const auto &lv:_langVecSet) {
        if (lv.second.empty() || lv.second.begin()->second.empty()) {
            continue;
        }

        auto categoryLangModelIter = _categoryLangModelFileNames.find(lv.first);
        if (categoryLangModelIter == _categoryLangModelFileNames.end()) {
            throw std::runtime_error("category detection model file is not defined for language \"" + lv.first + "\"");
        }

        std::vector<std::string> fileNames;
        std::vector<std::vector<float>> vectors;
        for (const auto &v:lv.second) {
            fileNames.emplace_back(v.first);
            vectors.emplace_back(v.second);
        }

        std::vector<std::thread> thrPool;
        uint8_t workers = fileNames.empty()?0:((fileNames.size() < _threads)?fileNames.size():_threads);
        for (int i = 0; i < workers; ++i) {
            thrPool.emplace_back(std::thread(&categoryCluster_t::worker, this, i, workers,
                                             std::cref(lv.first),
                                             std::cref(categoryLangModelIter->second),
                                             std::cref(fileNames),
                                             std::cref(vectors)));
        }
        for (auto &i:thrPool) {
            i.join();
        }
    }
}

void categoryCluster_t::worker(uint8_t _thrID,
                               uint8_t _threads,
                               const std::string &_lang,
                               const std::string &_clusteringLangModelFileName,
                               const std::vector<std::string> &_fileNames,
                               const std::vector<std::vector<float>> &_vectors) noexcept {
    std::size_t filesPerThread = _fileNames.size() / _threads;
    std::size_t startFrom = _thrID * filesPerThread;
    std::size_t stopAt = ((_thrID == _threads - 1)?_fileNames.size():startFrom + filesPerThread);

    if (_vectors.empty()) {
        return;
    }

    try {
        thread_local categorizer_t categorizer(_clusteringLangModelFileName);
        std::vector<categories_t> result;
        categorizer(_vectors, startFrom, stopAt, result);

        std::unique_lock<std::mutex> lck(m_mtx);
        for (std::size_t i = 0; i < result.size(); ++i) {
            m_groupSet.at(result[i]).emplace(_fileNames[i + startFrom], _lang);
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }
}
