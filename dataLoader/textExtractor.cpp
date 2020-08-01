/**
 * @file dataLoader/textExtractor.cpp
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <regex>
#include <iostream>

#include "textExtractor.h"

bool textExtractor_t::operator()(const std::vector<uint8_t> &_html, document_t &_data) noexcept {
    auto tree = gumbo_parse_with_options(&kGumboDefaultOptions,
                                         reinterpret_cast<const char *>(_html.data()),
                                         _html.size());
    bool ret = true;
    try {
        parse(tree->root, _data);
    } catch (const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
        ret = false;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        ret = false;
    }

    gumbo_destroy_output(&kGumboDefaultOptions, tree);

    return ret;
}

void textExtractor_t::parse(const GumboNode *_node, document_t &_data) {
    if (_node->type != GUMBO_NODE_ELEMENT) {
        return;
    }

    if (_node->v.element.tag == GUMBO_TAG_H1) {
        if (_data.title.empty()) {
            getText(_node, _data.title);
        }
    } else if (_node->v.element.tag == GUMBO_TAG_TIME) {
        if (_data.time == 0) {
            std::string tmp;
            getText(_node, tmp);
            str2time(tmp, _data.time);
        }
    } else if ((_node->v.element.tag == GUMBO_TAG_P) || (_node->v.element.tag == GUMBO_TAG_LI)) {
        getText(_node, _data.text);
    } else if (_node->v.element.tag == GUMBO_TAG_META) {
        std::string key;
        std::string value;
        if (getMeta(_node, key, value)) {
            if (key == "og:site_name") {
                _data.site = std::move(value);
            } else if (key == "article:published_time") {
                str2time(value, _data.time);
            } else if (key == "og:title") {
                _data.title = std::move(value);
            }
        }

    }

    auto children = &_node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        parse(static_cast<GumboNode *>(children->data[i]), _data);
    }
}

void textExtractor_t::getText(const GumboNode *_node, std::string &_text) {
    if (_node->type == GUMBO_NODE_TEXT) {
        _text = _text + (_text.empty()?"":" ") +  _node->v.text.text;
    } else if (_node->type == GUMBO_NODE_ELEMENT &&
               _node->v.element.tag != GUMBO_TAG_SCRIPT &&
               _node->v.element.tag != GUMBO_TAG_STYLE) {
        auto children = &_node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            getText(static_cast<GumboNode *>(children->data[i]), _text);
        }
    }
}

bool textExtractor_t::getMeta(const GumboNode *_node, std::string &_key, std::string &_value) {
    if (_node->v.element.original_tag.length > 0) {
        std::string tag(_node->v.element.original_tag.data, _node->v.element.original_tag.length);

        std::regex rgx("^<meta property=\"(.+)\" content=\"(.+)\"/>$");
        std::smatch match;
        if (std::regex_search(tag, match, rgx) && (match.size() == 3)) {
            _key = match[1];
            _value = match[2];

            return true;
        }
    }

    return false;
}

bool textExtractor_t::str2time(const std::string &_str, uint64_t &_t) {
    std::regex rgx(R"(^(\d+)-(\d+)-(\d+)T(\d+):(\d+):(\d+)(.)(\d+):(\d+)$)");
    std::smatch match;
    if (std::regex_search(_str, match, rgx) && (match.size() == 10)) {
        std::tm tm {};
        tm.tm_year = std::stoi(match[1]) - 1900;
        tm.tm_mon = std::stoi(match[2]) - 1;
        tm.tm_mday = std::stoi(match[3]);
        tm.tm_hour = std::stoi(match[4]);
        tm.tm_min = std::stoi(match[5]);
        tm.tm_sec = std::stoi(match[6]);
        auto c = match[7];
        auto off = (std::stoi(match[8]) * 60 + std::stoi(match[9])) * 60;
        tm.tm_gmtoff = (c == "+")?off:-off;

        _t = std::mktime(&tm);

        return true;
    }

    return false;
}
