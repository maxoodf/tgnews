/**
 * @file dataLoader/dataLoader.cpp
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <thread>
#include <fstream>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif
#include <cld_3/nnet_language_identifier.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "fileEnumerator.h"
#include "dataLoader.h"

dataLoader_t::dataLoader_t(uint8_t _threads,
                           const std::vector<std::string> &_langs,
                           char  *const *_path,
                           bool _allLangs): m_allLangs(_allLangs) {
    if (!m_allLangs) {
        for (const auto &s:_langs) {
            m_langDocSet.emplace(s, docSet_t());
        }
    }

    fileEnumerator_t fe;
    std::vector<std::thread> thrPool;
    const auto &fileNames = fe(_path);
    uint8_t workers = fileNames.empty()?0:((fileNames.size() < _threads)?fileNames.size():_threads);
    for (int i = 0; i < workers; ++i) {
        thrPool.emplace_back(std::thread(&dataLoader_t::worker, this, i, workers, std::cref(fileNames)));
    }
    for (auto &i:thrPool) {
        i.join();
    }
}

bool dataLoader_t::loadFile(const std::string &_fileName, std::vector<uint8_t> &_data) noexcept {
    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit);
        ifs.open(_fileName, std::ifstream::in | std::ifstream::binary);
        auto startPos = ifs.tellg();
        ifs.ignore(std::numeric_limits<std::streamsize>::max());
        auto size = static_cast<std::size_t>(ifs.gcount());
        ifs.seekg(startPos);
        _data.resize(size);
        ifs.read(reinterpret_cast<char *>(_data.data()), size);
        ifs.close();

        return true;
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    return false;
}

void dataLoader_t::worker(uint8_t _thrID, uint8_t _threads,
                          const std::vector<std::pair<std::string, std::string>> &_fileNames) noexcept {
    std::size_t filesPerThread = _fileNames.size() / _threads;
    std::size_t startFrom = _thrID * filesPerThread;
    std::size_t stopAt = ((_thrID == _threads - 1)?_fileNames.size():startFrom + filesPerThread);

    try {
        thread_local std::unique_ptr<chrome_lang_id::NNetLanguageIdentifier> lang_id;
        {
            std::unique_lock<std::mutex> lck(m_mtx);
            lang_id = std::make_unique<chrome_lang_id::NNetLanguageIdentifier>(0, 1024);
        }
        textExtractor_t te;

        for (auto i = startFrom; i < stopAt; ++i) {
            document_t data;
            data.name = _fileNames[i].second;
            std::vector<uint8_t> body;
            std::string absFileName = _fileNames[i].first + _fileNames[i].second;
            if (loadFile(absFileName, body) && te(body, data)) {
                std::string doc(data.title);
                if (!data.text.empty()) {
                    if (doc.empty()) {
                        doc = data.text;
                    } else {
                        doc += ". " + data.text;
                    }
                }
                if (doc.empty()) {
                    continue;
                }

                const chrome_lang_id::NNetLanguageIdentifier::Result r = lang_id->FindLanguage(doc);
                std::unique_lock<std::mutex> lck(m_mtx);
                auto l = m_langDocSet.find(r.language);
                if (m_allLangs) {
                    if (l == m_langDocSet.end()) {
                        auto l1 = m_langDocSet.emplace(r.language, docSet_t());
                        l1.first->second.emplace(absFileName, data);
                    } else {
                        l->second.emplace(absFileName, data);
                    }
                } else {
                    if (l != m_langDocSet.end()) {
                        l->second.emplace(absFileName, data);
                    }
                }
            }
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }
}
