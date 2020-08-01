/**
 * @file repository/repository.cpp
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#include <chrono>

#include <word2vec.hpp>

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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wextra-semi"
#endif
#include <dlib/threads.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

#include "sqliteClient.h"
#include "dataLoader/textExtractor.h"
#include "embedder/embedder.h"
#include "newsDetector/newsDetector.h"
#include "categorizer/categorizer.h"
#include "dbscan/dbscan.h"
#include "extDocAttr.h"
#include "ranker.h"
#include "repository.h"

repository_t::dataProcessing_t::dataProcessing_t(uint8_t _langID): langID(_langID) {
}

repository_t::dataProcessing_t::~dataProcessing_t() = default;

repository_t::repository_t(uint8_t _threads,
                           const std::vector<std::string> &_langCodes,
                           const std::unordered_map<std::string, std::string> &_w2vFileNames,
                           const std::unordered_map<std::string, std::string> &_newsDetectionModels,
                           const std::unordered_map<std::string, std::string> &_categoryDetectionModels,
                           const std::unordered_map<std::string, std::string> &_weightDetectionModels,
                           const std::unordered_map<categories_t, std::string> &_categoryNames,
                           const std::unordered_map<std::string, float> &_similarityThreshold,
                           const std::string &_sqliteFileName,
                           const std::unordered_map<std::string, std::string> &_indexFileNames):
        m_threads(_threads), m_categoryNames(_categoryNames) {

    std::cout << "repository loading..." << std::endl;
    m_langIdentifier = std::make_unique<chrome_lang_id::NNetLanguageIdentifier>(0, 1024);

    m_sqliteClient = std::make_unique<sqliteClient_t>(_sqliteFileName);

    for (std::size_t i = 0; i < _langCodes.size(); ++i) {
        m_dataProcessingSet.emplace(_langCodes[i], std::make_unique<dataProcessing_t>(i));
    }

    for (const auto &lc:_langCodes) {
        std::cout << "repository loading: lang_code=" << lc << std::endl;
        auto dp = m_dataProcessingSet.find(lc);
        uint64_t id = 0;
        if (m_sqliteClient->get(dp->second->langID, id) != sqliteClient_t::ret_t::OK) {
            throw std::runtime_error("failed to get last vector id value from the DB for language \"" + lc + "\"");
        }
        dp->second->lastId = id;
        std::cout << "repository loading: last vector id=" << id << std::endl;

        const auto w2vfn = _w2vFileNames.find(lc);
        if (w2vfn == _w2vFileNames.end()) {
            throw std::runtime_error("embedding model file is not defined for language \"" + lc + "\"");
        }
        dp->second->embedder = std::make_unique<embedder_t>(w2vfn->second);
        std::cout << "repository loading: embedding model is loaded" << std::endl;

        const auto ndm = _newsDetectionModels.find(lc);
        if (ndm == _newsDetectionModels.end()) {
            throw std::runtime_error("news detection model file is not defined for language \"" + lc + "\"");
        }
        dp->second->newsDetector = std::make_unique<newsDetector_t>(ndm->second);
        std::cout << "repository loading: news detection model is loaded" << std::endl;

        const auto cdm = _categoryDetectionModels.find(lc);
        if (cdm == _categoryDetectionModels.end()) {
            throw std::runtime_error("category detection model file is not defined for language \"" + lc + "\"");
        }
        dp->second->categorizer = std::make_unique<categorizer_t>(cdm->second);
        std::cout << "repository loading: categorizing model is loaded" << std::endl;

        const auto wdm = _weightDetectionModels.find(lc);
        if (wdm == _weightDetectionModels.end()) {
            throw std::runtime_error("weight detection model file is not defined for language \"" + lc + "\"");
        }
        dp->second->ranker = std::make_unique<ranker_t>(wdm->second);
        std::cout << "repository loading: weight model is loaded" << std::endl;

        const auto sth = _similarityThreshold.find(lc);
        if (sth == _similarityThreshold.end()) {
            throw std::runtime_error("similarity threshold value is not defined for language \"" + lc + "\"");
        }
        dp->second->similarityThreshold = sth->second;

        const auto ifn = _indexFileNames.find(lc);
        if (ifn == _indexFileNames.end()) {
            throw std::runtime_error("index file name is not defined for language \"" + lc + "\"");
        }
        dp->second->index = std::make_unique<w2v::d2vModel_t>(dp->second->embedder->vectorSize());
        dp->second->indexFileName = ifn->second;
        dp->second->index->load(dp->second->indexFileName);
        std::cout << "repository loading: index is loaded" << std::endl;
    }

    m_saver = std::thread(&repository_t::worker, this);
}

repository_t::~repository_t() {
    m_stopFlag = true;
    m_saver.join();
    sync();
}

uint16_t repository_t::onPut(const std::string &_name,
                             uint32_t _ttl,
                             const std::vector<uint8_t> &_data,
                             std::string &_description,
                             void *_ctx) {
    auto repository = static_cast<repository_t *>(_ctx);

    try {
        auto busyFlag = std::make_unique<busyFlag_t>(repository->m_busy);

        // check ttl
        if (_ttl == 0) {
            _description = "Bad Request";
            return 400;
        }

        // text extraction from HTML
        document_t doc;
        {
            textExtractor_t textExtractor;
            if (!textExtractor(_data, doc)) {
                _description = "Internal Server Error";
                return 500;
            }
        }

        // language detection
        std::string langCode;
        uint8_t langID = 0;
        {
            std::string text(doc.title);
            if (!doc.text.empty()) {
                if (text.empty()) {
                    text = doc.text;
                } else {
                    text += ". " + doc.text;
                }
            }
            if (text.empty()) {
                _description = "No Content";
                return 204;
            }

            chrome_lang_id::NNetLanguageIdentifier::Result r;
            {
                std::unique_lock<std::mutex> lck(repository->m_langIdentifierMtx);
                r = repository->m_langIdentifier->FindLanguage(text);
            }
            auto l = repository->m_dataProcessingSet.find(r.language);
            if (l == repository->m_dataProcessingSet.end()) {
                _description = "Ignored";
                return 202; // correct reply
            }
            langCode = l->first;
            langID = l->second->langID;
        }

        // embedding
        std::vector<std::vector<float>> docVecs;
        {
            std::vector<document_t> dv{doc};
            (*repository->m_dataProcessingSet.at(langCode)->embedder)(dv, 0, 1, docVecs);
            if (docVecs.size() != 1) {
                _description = "Ignored";
                return 202; // correct reply
            }
        }

        // news detection
        {
            std::vector<bool> result;
            {
                std::unique_lock<std::mutex> lck(repository->m_dataProcessingSet.at(langCode)->newsDetectorMtx);
                (*repository->m_dataProcessingSet.at(langCode)->newsDetector)(docVecs, 0, 1, result);
            }
            if ((result.size() != 1) || !result[0]) {
                _description = "Ignored";
                return 202; // correct reply
            }
        }

        // categorizing
        std::vector<categories_t> categories;
        {
            {
                std::unique_lock<std::mutex> lck(repository->m_dataProcessingSet.at(langCode)->categorizerMtx);
                (*repository->m_dataProcessingSet.at(langCode)->categorizer)(docVecs, 0, 1, categories);
            }
            if (categories.size() != 1) {
                _description = "Ignored";
                return 202; // correct reply
            }

        }

        uint64_t id = ++(repository->m_dataProcessingSet.at(langCode)->lastId);
        std::tuple<uint64_t, uint8_t> result {0, 0};
        auto ret = repository->m_sqliteClient->add(
                std::make_tuple(
                        id,
                        langID,
                        static_cast<uint8_t>(categories[0]),
                        _name,
                        doc.title,
                        doc.site,
                        doc.time,
                        _ttl),
                result
        );

        w2v::vector_t vector(docVecs[0]);
        {
            std::unique_lock lck(repository->m_dataProcessingSet.at(langCode)->indexMtx);
            if (std::get<0>(result) != 0) {
                repository->m_dataProcessingSet.at(langCode)->index->erase(std::get<0>(result));
            }
            repository->m_dataProcessingSet.at(langCode)->index->set(id, vector);
        }
        repository->m_synced = false;
        switch (ret) {
            case sqliteClient_t::ret_t::OK: {
                _description = "Created";
                return 201;
            }
            case sqliteClient_t::ret_t::REPLACED: {
                _description = "Created";
                return 204;
            }
            case sqliteClient_t::ret_t::FAILED: {
                break;
            }
        }
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
    }

    _description = "Internal Server Error";
    return 500;
}

uint16_t repository_t::onDelete(const std::string &_name, std::string &_description, void *_ctx) {
    auto repository = static_cast<repository_t *>(_ctx);

    auto busyFlag = std::make_unique<busyFlag_t>(repository->m_busy);

    std::tuple<uint64_t, uint8_t> removed {0, 0};
    if (repository->m_sqliteClient->remove(_name, removed) == sqliteClient_t::ret_t::OK) {
        if (std::get<0>(removed) == 0) {
            _description = "No Content";
            return 404;
        }
        for (const auto &i:repository->m_dataProcessingSet) {
            if (i.second->langID == std::get<1>(removed)) {
                try {
                    {
                        std::unique_lock lck(i.second->indexMtx);
                        i.second->index->erase(std::get<0>(removed));
                    }
                    repository->m_synced = false;
                    _description = "No Content";
                    return 204;
                } catch (const std::exception &_e) {
                    std::cerr << _e.what() << std::endl;
                } catch (...) {
                    std::cerr << "unknown error" << std::endl;
                }
            }
        }
    }

    _description = "Internal Server Error";
    return 500;
}

uint16_t repository_t::onGet(uint32_t _period,
                             const std::string &_langCode,
                             const std::string &_category,
                             std::string &_description,
                             std::string &_reply,
                             void *_ctx) {
    auto repository = static_cast<repository_t *>(_ctx);

    auto busyFlag = std::make_unique<busyFlag_t>(repository->m_busy);

    const auto dataProcessingIter = repository->m_dataProcessingSet.find(_langCode);

    uint8_t category = 255;
    for (const auto &i:repository->m_categoryNames) {
        if (i.second == _category) {
            category = static_cast<uint8_t>(i.first);
            break;
        }
    }
    if (((category == 255) && (_category != "any"))
        || (dataProcessingIter == repository->m_dataProcessingSet.end())) {

        _description = "Bad Request";
        return 400;
    }

    sqliteClient_t::ret_t ret = sqliteClient_t::ret_t::FAILED;
    std::vector<std::tuple<uint64_t, uint8_t, std::string, std::string, std::string, uint64_t>> result;
    if (category == 255) {
        auto params = std::make_tuple(dataProcessingIter->second->langID, _period);
        ret = repository->m_sqliteClient->get(params, result);
    } else {
        auto params = std::make_tuple(dataProcessingIter->second->langID, category, _period);
        ret = repository->m_sqliteClient->get(params, result);
    }

    if (ret != sqliteClient_t::ret_t::OK) {
        _description = "Internal Server Error";
        return 500;
    }

    // document vectors grouped by categories
    std::map<categories_t, std::vector<std::vector<float>>> docVecByCategory;
    // result indices  grouped by categories
    std::map<categories_t, std::vector<std::size_t>> resultIdxByCategory;
    { // fill out docVecByCategory & resultIdxByCategory arrays
        std::shared_lock lck(dataProcessingIter->second->indexMtx);
        for (std::size_t i = 0; i < result.size(); ++i) {
            const auto tmpVec = dataProcessingIter->second->index->vector(std::get<0>(result[i]));
            if (tmpVec == nullptr) {
                continue;;
            }
            auto c = static_cast<categories_t>(std::get<1>(result[i]));

            auto dvc = docVecByCategory.find(c);
            if (dvc == docVecByCategory.end()) {
                docVecByCategory.emplace(c, std::vector<std::vector<float>>(1, *tmpVec));
            } else {
                dvc->second.emplace_back(*tmpVec);
            }

            auto ric = resultIdxByCategory.find(c);
            if (ric == resultIdxByCategory.end()) {
                resultIdxByCategory.emplace(c, std::vector<std::size_t>(1, i));
            } else {
                ric->second.push_back(i);
            }
        }
    }

    // clustered documents, ordered by their relevance inside of each cluster
    extClusterSet_t clusters;
    std::mutex mtx;
    dlib::parallel_for(repository->m_threads,
                       static_cast<std::size_t>(categories_t::SOCIETY),
                       static_cast<std::size_t>(categories_t::OTHER) + 1,
                       [&](std::size_t i) {
        auto curCat = static_cast<categories_t>(i);
        const auto ci = docVecByCategory.find(curCat);
        if (ci == docVecByCategory.end()) {
            return;
        }
        // get clusters for each category
        dbscan_t dbscan(ci->second, dataProcessingIter->second->similarityThreshold, 32);
        extClusterSet_t tmpClusters(dbscan.size(), extCluster_t(curCat));
        for (const auto &j:dbscan()) {
            // j<0> // cluster id
            // j<1> // idx inside cluster
            // j<2> // weight
            auto ric = resultIdxByCategory.find(ci->first);
            if (ric == resultIdxByCategory.end()) {
                continue;
            }

            // cluster IDs start from 1
            tmpClusters[std::get<0>(j) - 1].extDocAttrs.emplace_back(extDocAttr_t(
                    std::get<2>(result[ric->second[std::get<1>(j)]]),
                    std::get<4>(result[ric->second[std::get<1>(j)]]),
                    std::get<3>(result[ric->second[std::get<1>(j)]]),
                    std::get<5>(result[ric->second[std::get<1>(j)]]),
                    std::get<2>(j),
                    ci->second[std::get<1>(j)]));
        }

        std::size_t maxclustersize = 0;
        for (auto &j:tmpClusters) {
            if (maxclustersize < j.extDocAttrs.size()) {
                maxclustersize = j.extDocAttrs.size();
            }
        }

        for (auto &j:tmpClusters) {
            j.rank = (*dataProcessingIter->second->ranker)(j, maxclustersize);
        }

        std::unique_lock<std::mutex> lck(mtx);
        clusters.insert(clusters.end(),
                        std::make_move_iterator(tmpClusters.begin()),
                        std::make_move_iterator(tmpClusters.end()));
    });

    std::sort(clusters.begin(), clusters.end(), [](const extCluster_t &_l, const extCluster_t &_r) {
        return (_l.rank > _r.rank);
    });

    rapidjson::Document json;
    json.SetObject();
    rapidjson::Value jsonThreadArray(rapidjson::kArrayType);
    for (const auto &i:clusters) {
        if (i.extDocAttrs.empty()) {
            continue;
        }

        std::string title = i.extDocAttrs[0].title;
        rapidjson::Value jsonGroupObject(rapidjson::kObjectType);
        rapidjson::Value jsonArticleArray(rapidjson::kArrayType);
        for (const auto &a:i.extDocAttrs) {
            if ((i.extDocAttrs[0].weight == a.weight) && (a.title.length() < title.length())) {
                title = a.title;
            }
            rapidjson::Value jsonArticle;
            jsonArticle.SetString(a.name.c_str(), a.name.length(),
                                  json.GetAllocator());
            jsonArticleArray.PushBack(jsonArticle, json.GetAllocator());
        }

        rapidjson::Value jsonTitle;
        jsonTitle.SetString(title.c_str(), title.length(), json.GetAllocator());
        jsonGroupObject.AddMember("title", jsonTitle, json.GetAllocator());

        if (category == 255) {
            rapidjson::Value jsonCategoryName;
            auto cn = repository->m_categoryNames.at(i.category);
            jsonCategoryName.SetString(cn.c_str(), cn.length(), json.GetAllocator());
            jsonGroupObject.AddMember("category", jsonCategoryName, json.GetAllocator());
        }

        jsonGroupObject.AddMember("articles", jsonArticleArray, json.GetAllocator());
        jsonThreadArray.PushBack(jsonGroupObject, json.GetAllocator());
    }
    json.AddMember("threads", jsonThreadArray, json.GetAllocator());

    rapidjson::StringBuffer jsonStr;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(jsonStr);
    writer.SetIndent(' ', 2);
    json.Accept(writer);
    _reply = jsonStr.GetString();

    _description = "OK";
    return 200;
}

void repository_t::sync() {
    // remove expired records
    std::cout << "data syncing..." << std::endl;
    for (const auto &dp:m_dataProcessingSet) {
        std::vector<uint64_t> removed;
        if (m_sqliteClient->remove(dp.second->langID, removed) == sqliteClient_t::ret_t::OK) {
            std::unique_lock lck(dp.second->indexMtx);
            for (const auto &i:removed) {
                dp.second->index->erase(i);
            }
            dp.second->index->save(dp.second->indexFileName);
            std::cout << removed.size() << " records removed" << std::endl;
        }
    }
    std::cout << "data synced" << std::endl;
}

void repository_t::worker() {
    while (!m_stopFlag) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!m_busy && !m_synced) {
            sync();
            m_synced = true;
        }
    }
}
