/**
 * @file newsDetector/newsDetector.h
 * @brief
 * @author Max Fomichev
 * @date 25.02.2020
*/


#ifndef TGNEWS_NEWSDETECTOR_H
#define TGNEWS_NEWSDETECTOR_H

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

class newsDetector_t {
public:
    explicit newsDetector_t(const std::string &_newsModelFileName);
    ~newsDetector_t();

    void operator()(const std::vector<std::vector<float>> &_vectors,
                    std::size_t _startFrom, std::size_t _stopAt,
                    std::vector<bool> &_result) noexcept;

private:
    using binaryLabelNet_t = dlib::loss_binary_log<
    dlib::fc<1,
            dlib::input<dlib::matrix<float>>
    >>;

    std::unique_ptr<binaryLabelNet_t> m_binaryLabelNet;
};

#endif //TGNEWS_NEWSDETECTOR_H
