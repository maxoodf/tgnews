/**
 * @file dbscan/dbscan.cpp
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#include <cmath>
#include <algorithm>

#include "dbscan.h"

dbscan_t::dbscan_t(const std::vector<std::vector<float>> &_db, float _eps, uint8_t _minPts):
        m_db(_db), m_threshold(_eps), m_minPts(_minPts), m_items(_db.size()) {

    auto size = _db.size();
    if ((size >= 10) && (size < 20)) {
        m_threshold += 0.0f + (size - 10) * (0.00625f - 0.0f) / (20 - 10);
    } else if ((size >= 20) && (size < 40)) {
        m_threshold += 0.00625f + (size - 20) * (0.0125f - 0.00625f) / (40 - 20);
    } else if ((size >= 40) && (size < 80)) {
        m_threshold += 0.0125f + (size - 40) * (0.03f - 0.0125f) / (80 - 40);
    } else if ((size >= 80) && (size < 160)) {
        m_threshold += 0.03f + (size - 80) * (0.0475f - 0.03f) / (160 - 80);
    } else if ((size >= 160) && (size < 320)) {
        m_threshold += 0.0475f + (size - 160) * (0.05f - 0.0475f) / (320 - 160);
    } else if ((size >= 320) && (size < 640)) {
        m_threshold += 0.05f + (size - 320) * (0.0525f - 0.05f) / (640 - 320);
    } else if ((size >= 640) && (size < 1280)) {
        m_threshold += 0.0525f + (size - 640) * (0.055f - 0.0525f) / (1280 - 640);
    } else if ((size >= 1280) && (size < 2560)) {
        m_threshold += 0.055f + (size - 1280) * (0.0575f - 0.055f) / (2560 - 1280);
    } else if ((size >= 2560) && (size < 5120)) {
        m_threshold += 0.0575f + (size - 2560) * (0.06f - 0.0575f) / (5120 - 2560);
    } else if (size >= 5120) {
        m_threshold += 0.06f;
    }

    createSimilarityMatrix();
    for (uint8_t pts = m_minPts; pts > 0; --pts) {
        for (std::size_t i = 0; i < _db.size(); ++i) {
            if (m_items[i].label != label_t::UNDEFINED) {
                continue;
            }

            std::vector<std::size_t> seeds;
            baseRangeQuery(i, seeds);
            if (seeds.size() < pts) {
                continue;
            }

            // new cluster
            ++m_id;
            m_items[i].label = label_t::CLUSTERED;
            m_items[i].clusterID = m_id;
            m_items[i].neighbors = seeds.size();

            std::size_t n = 0;
            while (true) {
                if (m_items[seeds[n]].label != label_t::CLUSTERED) {
                    m_items[seeds[n]].label = label_t::CLUSTERED;
                    m_items[seeds[n]].clusterID = m_id;
                    std::vector<std::size_t> neighbors;
                    baseRangeQuery(seeds[n], neighbors);
                    m_items[seeds[n]].neighbors = neighbors.size();

                    if (!neighbors.empty()) {
                        seeds.insert(seeds.end(),
                                     std::make_move_iterator(neighbors.begin()),
                                     std::make_move_iterator(neighbors.end()));
                    }
                }
                if (++n >= seeds.size()) {
                    break;
                }
            }
        }
    }

    // mark noise points with a new cluster id
    for (std::size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].clusterID == 0) {
            m_items[i].clusterID = ++m_id;
        }
        m_clusters.emplace_back(std::make_tuple(m_items[i].clusterID, i, m_items[i].neighbors));
    }

    std::sort(m_clusters.begin(),
              m_clusters.end(),
              [](std::tuple<std::size_t, std::size_t, std::size_t> &_l,
                 std::tuple<std::size_t, std::size_t, std::size_t> &_r) {
                  if (std::get<0>(_l) < std::get<0>(_r)) {
                      return true;
                  } else if (std::get<0>(_l) == std::get<0>(_r)) {
                      return (std::get<2>(_l) > std::get<2>(_r));
                  } else {
                      return false;
                  }
              });
}

float dbscan_t::distance(const std::vector<float> &_l, const std::vector<float> &_r) noexcept {
    auto dst = 0.0f;
    for (std::size_t i = 0; i < _l.size(); ++i) {
        dst += _l[i] * _r[i];
    }
    return ((dst > 0.0f)?std::sqrt(dst / _l.size()):0.0f);
}

void dbscan_t::baseRangeQuery(std::size_t _idx, std::vector<std::size_t> &_neighbors) {
    // find root's neighbors
    auto range = m_similarityMatrix.equal_range(_idx);
    for (auto i = range.first; i != range.second; ++i) {
        _neighbors.push_back(i->second.first);
    }
}

void dbscan_t::createSimilarityMatrix() {
    if (m_db.empty()) {
        return;
    }

    for (std::size_t n = 0; n < m_db.size() - 1; ++n) {
        for (std::size_t k = n + 1; k < m_db.size(); ++k) {
            auto dst = distance(m_db[n], m_db[k]);
            if (dst < m_threshold) {
                continue;
            }
            m_similarityMatrix.emplace(n, std::make_pair(k, dst));
            m_similarityMatrix.emplace(k, std::make_pair(n, dst));
        }
    }
}
