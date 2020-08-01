/**
 * @file dataLoader/fileEnumerator.cpp
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <fts.h>

#include <stdexcept>

#include "fileEnumerator.h"

const std::vector<std::pair<std::string, std::string>> &fileEnumerator_t::operator()(char  *const *_path) {
    m_fileList.clear();

    auto fileSystem = fts_open(_path, FTS_COMFOLLOW | FTS_NOCHDIR, nullptr);
    if (fileSystem == nullptr) {
        throw std::runtime_error("Failed to open specified source path");
    }

    FTSENT *parent = nullptr;
    while (true) {
        auto child = fts_children(fileSystem, 0);
        while (child != nullptr) {
            if (child->fts_info & FTS_F) {
                m_fileList.emplace_back(std::pair<std::string, std::string>(child->fts_path, child->fts_name));
            }
            child = child->fts_link;
        }
        parent = fts_read(fileSystem);
        if (parent == nullptr) {
            break;
        }
    }
    fts_close(fileSystem);

    return m_fileList;
}
