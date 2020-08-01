/**
 * @file dataLoader/fileEnumerator.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_FILEENUMERATOR_H
#define TGNEWS_FILEENUMERATOR_H

#include <string>
#include <vector>

class fileEnumerator_t final {
private:
    std::vector<std::pair<std::string, std::string>> m_fileList;

public:
    const std::vector<std::pair<std::string, std::string>> &operator()(char  *const *_path);
};

#endif //TGNEWS_FILEENUMERATOR_H
