/**
 * @file repository/ranker.cpp
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#include "types.h"
#include "ranker.h"

ranker_t::ranker_t(const std::string &_weightModelFileName) {
    m_langWeightNet = std::make_unique<weightNet_t>();
    dlib::deserialize(_weightModelFileName) >> *m_langWeightNet;
}

float ranker_t::operator()(const extCluster_t &_cluster, std::size_t _records) {
    if (_cluster.extDocAttrs.empty()) {
        return 0.0f;
    }

    auto weightMark = 0.0f;
// TODO: train model on more samples
/*
    thread_local auto weightNet = *m_langWeightNet;
    try {
        dlib::matrix<float> matrix;
        auto vSize = _cluster.extDocAttrs.begin()->vector.size();
        matrix.set_size(1, vSize);
        std::vector<dlib::matrix<float>> samples(_cluster.extDocAttrs.size(), matrix);
        std::size_t pos = 0;
        for (std::size_t i = 0; i < samples.size(); ++i) {
            for (std::size_t j = 0; j < vSize; ++j) {
                samples[pos](j) = _cluster.extDocAttrs[i].vector[j];
            }
            pos++;
        }

        auto prediction = weightNet(samples);
        for (const auto &i:prediction) {
            if ((i <= 0.8f) and (i >= 0.0f) and (weightMark < i)) {
                weightMark = i;
            }
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
        return 0.0f;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        return 0.0f;
    }
*/
    float categoryMark = 0.0f;
    switch (_cluster.category) {
        case categories_t::SOCIETY: {
            categoryMark = 1.0f;
            break;
        }
        case categories_t::ECONOMY: {
            categoryMark = 0.5f;
            break;
        }
        case categories_t::TECHNOLOGY: {
            categoryMark = 0.6f;
            break;
        }
        case categories_t::SPORTS: {
            categoryMark = 0.7f;
            break;
        }
        case categories_t::ENTERTAINMENT: {
            categoryMark = 0.5f;
            break;
        }
        case categories_t::SCIENCE: {
            categoryMark = 0.65f;
            break;
        }
        case categories_t::OTHER: {
            categoryMark = 0.3f;
            break;
        }
    }
    float sizeMark = static_cast<float>(_cluster.extDocAttrs.size()) / _records;

    return (sizeMark + weightMark / 2.0f) * categoryMark;
}
