/**
 * @file config.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_TYPES_H
#define TGNEWS_TYPES_H

#include <string>
#include <vector>
#include <unordered_map>

// type of CLI command
enum class cmd_t {
    LNG,
    NWS,
    CTG,
    THR,
    SRV
};

// parsed HTML data
struct document_t {
    std::string name;
    std::string site;
    std::string title;
    uint64_t time = 0;
    std::string text;

    document_t() = default;
    document_t(std::string _name, std::string _site, std::string _title, uint64_t _time):
            name(std::move(_name)), site(std::move(_site)), title(std::move(_title)), time(_time) {}
};

// file name and its parsed content
using docSet_t = std::unordered_map<std::string, document_t>;

// language code and its documents
using langDocSet_t = std::unordered_map<std::string, docSet_t>;

// file name and its document victor
using vecSet_t = std::unordered_map<std::string, std::vector<float>>;

// language code and its document vectors
using langVecSet_t = std::unordered_map<std::string, vecSet_t>;

enum class categories_t {
    SOCIETY = 0,
    ECONOMY = 1,
    TECHNOLOGY = 2,
    SPORTS = 3,
    ENTERTAINMENT = 4,
    SCIENCE = 5,
    OTHER = 6
};

// category and its document file names
using groupSet_t = std::unordered_map<categories_t, std::unordered_map<std::string, std::string>>;

// file name and its language code
using fileWithLang_t = std::pair<std::string, std::string>;

// cluster and its files
using cluster_t = std::vector<fileWithLang_t>;

// clusters set
using clusterSet_t = std::vector<std::pair<cluster_t, categories_t>>;

#endif //TGNEWS_TYPES_H
