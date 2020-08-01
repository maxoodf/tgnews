/**
 * @file embedder/embedder.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_EMBEDDER_H
#define TGNEWS_EMBEDDER_H

#include <vector>

#include "types.h"

#ifdef WITH_FAISS
namespace faiss {
    struct IndexIVFPQ;
}
#else
namespace w2v {
    class w2vModel_t;
}
#endif

class embedder_t {
public:
    explicit embedder_t(const std::string &_w2vLangModelFileName);
    ~embedder_t();

    void operator()(const std::vector<document_t> &_documents,
                   std::size_t _startFrom, std::size_t _stopAt,
                   std::vector<std::vector<float>> &_result) noexcept;
    [[nodiscard]] uint16_t vectorSize() const noexcept;

private:
#ifdef WITH_FAISS
    faiss::IndexIVFPQ *m_index;
    std::unordered_map<std::string, int64_t> m_word2id;
#else
    std::unique_ptr<w2v::w2vModel_t> m_w2vModel;
#endif
};

#endif //TGNEWS_EMBEDDER_H
