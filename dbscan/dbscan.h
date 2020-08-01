/**
 * @file dbscan/dbscan.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#ifndef DBSCAN_DBSCAN_H
#define DBSCAN_DBSCAN_H

#include <vector>
#include <map>

class dbscan_t {
public:
    dbscan_t(const std::vector<std::vector<float>> &_db, float _eps, uint8_t _minPts);
    ~dbscan_t() = default;

    const auto &operator()() const noexcept {return m_clusters;}
    [[nodiscard]] std::size_t size() const noexcept {return m_id;}

private:
    enum class label_t {
        UNDEFINED,
        NOISE,
        CLUSTERED
    };
    struct item_t {
        label_t label = label_t::UNDEFINED;
        std::size_t neighbors = 0;
        std::size_t clusterID = 0;
    };
    const std::vector<std::vector<float>> &m_db;
    float m_threshold;
    const uint8_t m_minPts;

    std::vector<item_t> m_items;
    // idx1, idx2, distance
    std::multimap<std::size_t, std::pair<std::size_t, float>> m_similarityMatrix;
    // cluster ID, idx, weight
    std::vector<std::tuple<std::size_t, std::size_t, std::size_t>> m_clusters;
    uint64_t m_id = 0;

    static float distance(const std::vector<float> &_l, const std::vector<float> &_r) noexcept;
    void createSimilarityMatrix();
    void baseRangeQuery(std::size_t _idx, std::vector<std::size_t> &_neighbors);
};

#endif //DBSCAN_DBSCAN_H
