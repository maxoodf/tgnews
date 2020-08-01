/**
 * @file categorizer/categorizer.cpp
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#include "categorizer.h"

categorizer_t::categorizer_t(const std::string &_categoryModelFileName) {
    m_langMultiLabelNet = std::make_unique<multiLabelNet_t>();
    dlib::deserialize(_categoryModelFileName) >> *m_langMultiLabelNet;
}

categorizer_t::~categorizer_t() = default;

void categorizer_t::operator()(const std::vector<std::vector<float>> &_vectors,
                               std::size_t _startFrom, std::size_t _stopAt,
                               std::vector<categories_t> &_result) noexcept {
    if (_vectors.empty()) {
        return;
    }

    try {
        dlib::matrix<float> matrix;
        auto vSize = _vectors.begin()->size();
        matrix.set_size(1, vSize);
        std::vector<dlib::matrix<float>> samples(_stopAt - _startFrom, matrix);
        std::size_t pos = 0;
        for (auto i = _startFrom; i < _stopAt; ++i) {
            for (std::size_t j = 0; j < vSize; ++j) {
                samples[pos](j) = _vectors[i][j];
            }
            pos++;
        }

        auto prediction = (*m_langMultiLabelNet)(samples);
        auto rSize = prediction.size();
        _result.resize(rSize);
        for (std::size_t i = 0; i < rSize; ++i) {
            _result[i] = static_cast<categories_t>(prediction[i]);
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }
}
