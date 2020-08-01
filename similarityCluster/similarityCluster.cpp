/**
 * @file similarityCluster/similarityCluster.cpp
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <mutex>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wextra-semi"
#endif
#include <dlib/threads.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "dbscan/dbscan.h"
#include "similarityCluster.h"

similarityCluster_t::similarityCluster_t(uint8_t _threads,
                                         const std::unordered_map<std::string, float> &_similarityThreshold,
                                         const langVecSet_t &_langVecSet,
                                         const groupSet_t &_groupSet) {
    std::unordered_map<std::string, std::unordered_map<std::string, categories_t>> catByLang;
    // iterate languages
    for (const auto &lv:_langVecSet) {
        if (lv.second.empty() || lv.second.begin()->second.empty()) {
            continue;
        }
        // get threshold value for the language
        auto threshold = 0.0f;
        {
            auto st = _similarityThreshold.find(lv.first);
            if (st != _similarityThreshold.end()) {
                threshold = st->second;
            } else {
                continue;
            }
        }

        // iterate categories
        dlib::parallel_for(_threads,
                           static_cast<std::size_t>(categories_t::SOCIETY),
                           static_cast<std::size_t>(categories_t::OTHER),
                           [&](std::size_t i) {
            auto category = static_cast<categories_t>(i);
            const auto ci = _groupSet.find(category);
            if (ci == _groupSet.end()) {
                return;
            }

            std::vector<std::string> fileNames;
            std::vector<std::vector<float>> vectors;
            // iterate filenames (j = {file, lang})
            for (const auto &j:ci->second) {
                if (j.second != lv.first) {
                    continue;
                }
                // find vector by file name
                auto vi = lv.second.find(j.first);
                if (vi == lv.second.end()) {
                    continue;
                }
                fileNames.push_back(j.first);
                vectors.push_back(vi->second);
            }
            dbscan_t dbscan(vectors, threshold, 32);

            clusterSet_t tmpClusterSet(dbscan.size(), std::make_pair(cluster_t(), ci->first));
            for (const auto &j:dbscan()) {
                // cluster id starts from 1
                tmpClusterSet[std::get<0>(j) - 1].first.emplace_back(fileNames[std::get<1>(j)], lv.first);
            }

            std::unique_lock<std::mutex> lck(m_mtx);
            m_clusters.insert(m_clusters.end(),
                              std::make_move_iterator(tmpClusterSet.begin()),
                              std::make_move_iterator(tmpClusterSet.end()));
        });
    }
}
