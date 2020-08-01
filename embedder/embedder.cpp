/**
 * @file embedder/embedder.cpp
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <iostream>

#include <mapper.hpp>
#include <wordReader.hpp>

#include <unicode/unistr.h>
#include <unicode/uchar.h>

#ifdef WITH_FAISS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <faiss/index_io.h>
#include <faiss/impl/io.h>
#include <faiss/IndexIVFPQ.h>
#pragma GCC diagnostic pop
#else
#include <word2vec.hpp>
#endif

#include "embedder.h"

embedder_t::embedder_t(const std::string &_w2vLangModelFileName) {
#ifdef WITH_FAISS
    {
        // load file
        w2v::fileMapper_t fileMapper(_w2vLangModelFileName);
        // create index reader
        faiss::VectorIOReader reader;
        reader.data.resize(fileMapper.size());
        std::memcpy(reader.data.data(), fileMapper.data(), fileMapper.size());

        // load index from memory
        m_index = dynamic_cast<faiss::IndexIVFPQ *>(faiss::read_index(&reader));
        if (m_index == nullptr) {
            throw std::runtime_error("unknown index format, file " + _w2vLangModelFileName);
        }
    }

    {
        // load file
        w2v::fileMapper_t fileMapper(_w2vLangModelFileName + ".map");
        w2v::wordReader_t<w2v::fileMapper_t> wordReader(fileMapper, "\n", "");
        std::string word;
        int64_t id = 0;
        while (wordReader.nextWord(word)) {
            if (word.empty()) {
                continue;
            }
            m_word2id.emplace(word, id++);
        }
    }

    m_index->nprobe = 1024;
    m_index->make_direct_map(true);
#else
    m_w2vModel = std::make_unique<w2v::w2vModel_t>();
    m_w2vModel->load(_w2vLangModelFileName);
#endif
}

embedder_t::~embedder_t() {
#ifdef WITH_FAISS
    delete m_index;
#endif
}

uint16_t embedder_t::vectorSize() const noexcept {
#ifdef WITH_FAISS
    return m_index->d;
#else
    return m_w2vModel->vectorSize();
#endif
}

void embedder_t::operator()(const std::vector<document_t> &_documents,
                           std::size_t _startFrom, std::size_t _stopAt,
                           std::vector<std::vector<float>> &_result) noexcept {
    if (_documents.empty()) {
        return;
    }

    try {
#ifdef WITH_FAISS
        auto vSize = m_index->d;
#else
        auto vSize = m_w2vModel->vectorSize();
#endif
        _result.resize(_stopAt - _startFrom, std::vector<float>(vSize, 0.0f));

        UChar sps(' ');
        UChar dot('.');
        UChar eol('\n');
        UChar qst('?');
        UChar xcl('!');
        std::size_t pos = 0;
        for (auto i = _startFrom; i < _stopAt; ++i) {
            icu::UnicodeString ucs = icu::UnicodeString::fromUTF8(
                    icu::StringPiece(_documents[i].title
                                     + ". "
                                     + _documents[i].text));
            ucs.toLower();
            for (int j = 0; j < ucs.length(); ++j) {
                if (!u_isalpha(ucs[j])) {
                    if ((ucs[j] != dot) && (ucs[j] != eol) && (ucs[j] != qst) && (ucs[j] != xcl)) {
                        ucs.replace(j, 1, sps);
                    }
                }
            }
            std::vector<char> tmp(_documents[i].title.length() + _documents[i].text.length() + 3, 0);
            ucs.extract(0, ucs.length(), tmp.data(), tmp.size() - 1);
// TODO: avoid copying
            std::string tmpStr(tmp.data(), tmp.size());
            w2v::stringMapper_t stringMapper(tmpStr);
            w2v::wordReader_t<w2v::stringMapper_t> wordReader(stringMapper,
                                                              " \n,.-!?:;/\"#$%&'()*+<=>@[]\\^_`{|}~\t\v\f\r",
                                                              "");

            std::string word;
            while (wordReader.nextWord(word)) {
                if (word.empty()) {
                    continue;
                }
#ifdef WITH_FAISS
                auto w2id = m_word2id.find(word);
                if (w2id == m_word2id.end()) {
                    continue;
                }
                std::vector<float> vec(vSize);
                m_index->reconstruct(w2id->second, vec.data());
#else
                auto vec = m_w2vModel->vector(word);
                if (vec == nullptr) {
                    continue;
                }
#endif
                for (uint16_t j = 0; j < vSize; ++j) {
#ifdef WITH_FAISS
                    _result[pos][j] += vec[j];
#else
                    _result[pos][j] += (*vec)[j];
#endif
                }
            }
            float med = 0.0f;
            for (auto const &j:_result[pos]) {
                med += j * j;
            }
            if (med <= 0.0f) {
                for (auto &j:_result[pos]) {
                    j = 0.0f;
                }
            } else {
                med = std::sqrt(med / _result[pos].size());
                for (auto &j:_result[pos]) {
                    j /= med;
                }
            }

            pos++;
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }
}
