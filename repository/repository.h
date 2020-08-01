/**
 * @file repository/repository.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#ifndef TGNEWS_REPOSITORY_H
#define TGNEWS_REPOSITORY_H

#include <shared_mutex>
#include <atomic>
#include <thread>

#include "types.h"

namespace chrome_lang_id {
    class NNetLanguageIdentifier;
}

namespace w2v {
    class d2vModel_t;
}

class sqliteClient_t;
class embedder_t;
class newsDetector_t;
class categorizer_t;
class ranker_t;
class repository_t {

public:
    repository_t(uint8_t _threads,
                 const std::vector<std::string> &_langCodes,
                 const std::unordered_map<std::string, std::string> &_w2vFileNames,
                 const std::unordered_map<std::string, std::string> &_newsDetectionModels,
                 const std::unordered_map<std::string, std::string> &_categoryDetectionModels,
                 const std::unordered_map<std::string, std::string> &_weightDetectionModels,
                 const std::unordered_map<categories_t, std::string> &_categoryNames,
                 const std::unordered_map<std::string, float> &_similarityThreshold,
                 const std::string &_sqliteFileName,
                 const std::unordered_map<std::string, std::string> &_indexFileNames);
    ~repository_t();

    static uint16_t onPut(const std::string &_name,
                          uint32_t _ttl,
                          const std::vector<uint8_t> &_data,
                          std::string &_description,
                          void *_ctx);
    static uint16_t onDelete(const std::string &_name, std::string &_description, void *_ctx);
    static uint16_t onGet(uint32_t _period,
                          const std::string &_langCode,
                          const std::string &_category,
                          std::string &_description,
                          std::string &_reply,
                          void *_ctx);
private:
    struct dataProcessing_t {
        const uint8_t langID = 0;

        std::atomic<uint64_t> lastId {0};

        std::unique_ptr<embedder_t> embedder;

        std::unique_ptr<newsDetector_t> newsDetector;
        std::mutex newsDetectorMtx;

        std::unique_ptr<categorizer_t> categorizer;
        std::mutex categorizerMtx;

        std::unique_ptr<ranker_t> ranker;

        float similarityThreshold = 0;

        std::unique_ptr<w2v::d2vModel_t> index;
        std::string indexFileName;
        std::shared_mutex indexMtx;

        explicit dataProcessing_t(uint8_t _langID);
        ~dataProcessing_t();
    };

    struct busyFlag_t {
        std::atomic<bool> &busy;

        explicit busyFlag_t(std::atomic<bool> &_busy): busy(_busy) {busy = true;}
        ~busyFlag_t() {busy = false;}
    };

    const uint8_t m_threads;
    const std::unordered_map<categories_t, std::string> &m_categoryNames;

    std::unique_ptr<chrome_lang_id::NNetLanguageIdentifier> m_langIdentifier;
    std::mutex m_langIdentifierMtx;

    std::unique_ptr<sqliteClient_t> m_sqliteClient;

    std::unordered_map<std::string, std::unique_ptr<dataProcessing_t>> m_dataProcessingSet;

    std::thread m_saver;
    std::atomic<bool> m_stopFlag {false};
    std::atomic<bool> m_busy {false};
    std::atomic<bool> m_synced {true};

    void sync();
    void worker();
};

#endif //TGNEWS_REPOSITORY_H
