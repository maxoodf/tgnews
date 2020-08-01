/**
 * @file dataLoader/textExtractor.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_TEXTEXTRACTOR_H
#define TGNEWS_TEXTEXTRACTOR_H

#include <gumbo.h>

#include "types.h"

class textExtractor_t final {
public:
    bool operator()(const std::vector<uint8_t> &_html, document_t &_data) noexcept;

private:
    static void parse(const GumboNode *_node, document_t &_data);
    static void getText(const GumboNode *_node, std::string &_text);
    static bool getMeta(const GumboNode *_node, std::string &_key,std::string &_value);
    static bool str2time(const std::string &_str, uint64_t &_t);
};

#endif //TGNEWS_TEXTEXTRACTOR_H
