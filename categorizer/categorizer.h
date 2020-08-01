/**
 * @file categorizer/categorizer.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#ifndef TGNEWS_CATEGORIZER_H
#define TGNEWS_CATEGORIZER_H

#include <vector>
#include <memory>

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

#include "types.h"

class categorizer_t {
public:
    explicit categorizer_t(const std::string &_categoryModelFileName);
    ~categorizer_t();

    void operator()(const std::vector<std::vector<float>> &_vectors,
                    std::size_t _startFrom, std::size_t _stopAt,
                    std::vector<categories_t> &_result) noexcept;

private:
    using multiLabelNet_t = dlib::loss_multiclass_log<
            dlib::fc<7,
                    dlib::input<dlib::matrix<float>>
            >>;

    std::unique_ptr<multiLabelNet_t> m_langMultiLabelNet;
};

#endif //TGNEWS_CATEGORIZER_H
