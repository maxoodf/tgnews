/**
 * @file repository/extDocAttr.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#ifndef TGNEWS_EXTDOCATTR_H
#define TGNEWS_EXTDOCATTR_H

#include <utility>
#include <vector>

#include "types.h"

struct extDocAttr_t: public document_t {
    std::size_t weight = 0;
    std::vector<float> vector;

    extDocAttr_t(std::string _name,
                 std::string _site,
                 std::string _title,
                 uint64_t _time,
                 std::size_t _weight,
                 std::vector<float> &_vector): document_t(std::move(_name),
                                                          std::move(_site),
                                                          std::move(_title),
                                                          _time),
                                               weight(_weight),
                                               vector(std::move(_vector)) {}
};

struct extCluster_t {
    float rank = 0.0f;
    categories_t category;
    std::vector<extDocAttr_t> extDocAttrs;

    explicit extCluster_t(categories_t _category): category(_category) {}
};

using extClusterSet_t = std::vector<extCluster_t>;

#endif //TGNEWS_EXTDOCATTR_H
