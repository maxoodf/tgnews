/**
 * @file repository/ranker.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/


#ifndef TGNEWS_RANKER_H
#define TGNEWS_RANKER_H

#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wextra-semi"
#endif
#include <dlib/dnn.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "extDocAttr.h"

class ranker_t {
public:
    explicit ranker_t(const std::string &_weightModelFileName);
    ~ranker_t() = default;

    float operator()(const extCluster_t &_cluster, std::size_t _records);

private:
    using weightNet_t = dlib::loss_mean_squared<
            dlib::fc<1,
                    dlib::input<dlib::matrix<float>>
            >>;

    std::unique_ptr<weightNet_t> m_langWeightNet;
};

#endif //TGNEWS_RANKER_H
