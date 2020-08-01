/**
 * @file cli/cli.cpp
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#include <memory>
#include <iostream>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

#include "types.h"
#include "dataLoader/dataLoader.h"
#include "newsCluster/newsCluster.h"
#include "categoryCluster/categoryCluster.h"
#include "similarityCluster/similarityCluster.h"

#include "cli.h"

cli_t::cli_t(const std::vector<std::string> &_langCodes,
             const std::unordered_map<std::string, std::string> &_w2vModels,
             const std::unordered_map<std::string, std::string> &_newsDetectionModels,
             const std::unordered_map<std::string, std::string> &_categoryDetectionModels,
             const std::unordered_map<categories_t, std::string> &_categoryNames,
             const std::unordered_map<std::string, float> &_similarityThreshold):
        m_langCodes(_langCodes),
        m_w2vModels(_w2vModels),
        m_newsDetectionModels(_newsDetectionModels),
        m_categoryDetectionModels(_categoryDetectionModels),
        m_categoryNames(_categoryNames),
        m_similarityThreshold(_similarityThreshold) {
}

//#include <fstream>
void cli_t::operator()(uint8_t _threads, cmd_t _cmd, char  *const *_path) {
// init JSON document
    rapidjson::Document json;

// Parsing, languages detection...
    dataLoader_t dataLoader(_threads, m_langCodes, _path, (_cmd == cmd_t::LNG));
//    dataLoader_t dataLoader(_threads, m_langCodes, _path);
    if (_cmd == cmd_t::LNG) {
        json.SetArray();
        for (const auto &ld:dataLoader.langDocSet()) {
/*
            // get file names
            std::ofstream ofs(ld.first);
            for (const auto &a:ld.second) {
                ofs << a.first << std::endl;
            }
            ofs.close();
*/
            rapidjson::Value jsonLangObject(rapidjson::kObjectType);

            rapidjson::Value jsonLangCode;
            jsonLangCode.SetString(ld.first.c_str(), ld.first.length(), json.GetAllocator());
            jsonLangObject.AddMember("lang_code", jsonLangCode, json.GetAllocator());
/*
            // get soure data
            std::ofstream ofs(ld.first);
            for (const auto &a:ld.second) {
                auto txt = a.second.title + ". " + a.second.text;
                std::size_t pos = std::string::npos;
                while (true) {
                    pos = txt.find('\n', pos);
                    if (pos == std::string::npos) {
                        break;
                    }
                    txt.replace(pos, 1, " ");
                    pos += 1;
                }
                ofs << txt << std::endl;
            }
*/
            rapidjson::Value jsonArticleArray(rapidjson::kArrayType);
            for (const auto &a:ld.second) {
                rapidjson::Value jsonArticle;
                jsonArticle.SetString(a.second.name.c_str(), a.second.name.length(), json.GetAllocator());
                jsonArticleArray.PushBack(jsonArticle, json.GetAllocator());
            }
            jsonLangObject.AddMember("articles", jsonArticleArray, json.GetAllocator());
            json.PushBack(jsonLangObject, json.GetAllocator());
        }
    } else { // it's not the language detection task
// Normalizing, documents embedding, news detecting...
        newsCluster_t newsCluster(_threads, m_w2vModels, m_newsDetectionModels, dataLoader.langDocSet());
        if (_cmd == cmd_t::NWS) {
            json.SetObject();
            rapidjson::Value jsonArticleArray(rapidjson::kArrayType);
            for (const auto &lv:newsCluster.langVecSet()) {
                auto lds = dataLoader.langDocSet().find(lv.first);
                if (lds == dataLoader.langDocSet().end()) {
                    continue;
                }
                for (const auto &a:lv.second) {
                    auto doc = lds->second.find(a.first);
                    if (doc == lds->second.end()) {
                        continue;
                    }
                    rapidjson::Value jsonArticle;
                    jsonArticle.SetString(doc->second.name.c_str(), doc->second.name.length(), json.GetAllocator());
                    jsonArticleArray.PushBack(jsonArticle, json.GetAllocator());
                }
            }
            json.AddMember("articles", jsonArticleArray, json.GetAllocator());
        } else { // it's not the news isolation task
            json.SetArray();

// Category clustering...
            auto categoryCluster = std::make_unique<categoryCluster_t>(_threads,
                                                                       m_categoryDetectionModels,
                                                                       m_categoryNames,
                                                                       newsCluster.langVecSet());
            if (_cmd == cmd_t::CTG) {
                for (const auto &cc:categoryCluster->groupSet()) {
                    auto c = m_categoryNames.find(cc.first);
                    if (c == m_categoryNames.end()) {
                        continue;
                    }

                    rapidjson::Value jsonCategoryObject(rapidjson::kObjectType);

                    rapidjson::Value jsonCategoryName;
                    jsonCategoryName.SetString(c->second.c_str(), c->second.length(), json.GetAllocator());
                    jsonCategoryObject.AddMember("category", jsonCategoryName, json.GetAllocator());

                    rapidjson::Value jsonArticleArray(rapidjson::kArrayType);
                    for (const auto &a:cc.second) {
                        auto lds = dataLoader.langDocSet().find(a.second);
                        if (lds == dataLoader.langDocSet().end()) {
                            continue;
                        }
                        auto doc = lds->second.find(a.first);
                        if (doc == lds->second.end()) {
                            continue;
                        }
                        rapidjson::Value jsonArticle;
                        jsonArticle.SetString(doc->second.name.c_str(), doc->second.name.length(),
                                              json.GetAllocator());
                        jsonArticleArray.PushBack(jsonArticle, json.GetAllocator());
                    }
                    jsonCategoryObject.AddMember("articles", jsonArticleArray, json.GetAllocator());
                    json.PushBack(jsonCategoryObject, json.GetAllocator());
                }
            } else if (_cmd == cmd_t::THR) {
// Similarity clustering...
                similarityCluster_t similarityCluster(_threads,
                                                      m_similarityThreshold,
                                                      newsCluster.langVecSet(),
                                                      categoryCluster->groupSet());
                for (const auto &sc:similarityCluster.clusters()) {
                    auto lds = dataLoader.langDocSet().find(sc.first[0].second);
                    if (lds == dataLoader.langDocSet().end()) {
                        continue;
                    }
                    auto doc = lds->second.find(sc.first[0].first);
                    if (doc == lds->second.end()) {
                        continue;
                    }

                    rapidjson::Value jsonGroupObject(rapidjson::kObjectType);

                    rapidjson::Value jsonCategoryName;
                    jsonCategoryName.SetString(doc->second.title.c_str(), doc->second.title.length(), json.GetAllocator());
                    jsonGroupObject.AddMember("title", jsonCategoryName, json.GetAllocator());

                    rapidjson::Value jsonArticleArray(rapidjson::kArrayType);
                    for (const auto &a:sc.first) {
                        lds = dataLoader.langDocSet().find(a.second);
                        if (lds == dataLoader.langDocSet().end()) {
                            continue;
                        }
                        doc = lds->second.find(a.first);
                        if (doc == lds->second.end()) {
                            continue;
                        }
                        rapidjson::Value jsonArticle;
                        jsonArticle.SetString(doc->second.name.c_str(), doc->second.name.length(),
                                              json.GetAllocator());
                        jsonArticleArray.PushBack(jsonArticle, json.GetAllocator());
                    }
                    jsonGroupObject.AddMember("articles", jsonArticleArray, json.GetAllocator());
                    json.PushBack(jsonGroupObject, json.GetAllocator());
                }
            }
        }
    }

    rapidjson::StringBuffer jsonStr;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(jsonStr);
    writer.SetIndent(' ', 2);
    json.Accept(writer);
    std::cout << jsonStr.GetString() << std::endl;

/*
        char wb[65536];
        rapidjson::FileWriteStream os(stdout, wb, sizeof(wb));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        writer.SetIndent(' ', 2);
        json.Accept(writer);
*/
}
