/**
 * @file similarityCluster/similarityCluster.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_SIMILARITYCLUSTER_H
#define TGNEWS_SIMILARITYCLUSTER_H

#include <mutex>

#include "types.h"

class similarityCluster_t {
public:
    similarityCluster_t(uint8_t _threads,
                        const std::unordered_map<std::string, float> &_similarityThreshold,
                        const langVecSet_t &_langVecSet,
                        const groupSet_t &_groupSet);

    [[nodiscard]] const clusterSet_t &clusters() const noexcept {return m_clusters;}

private:
    clusterSet_t m_clusters;
    std::mutex m_mtx;
};

#endif //TGNEWS_SIMILARITYCLUSTER_H
