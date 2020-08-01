/**
 * @file cli/cli.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#ifndef TGNEWS_CLI_H
#define TGNEWS_CLI_H

class cli_t {
public:
    cli_t(const std::vector<std::string> &_langCodes,
          const std::unordered_map<std::string, std::string> &_w2vModels,
          const std::unordered_map<std::string, std::string> &_newsDetectionModels,
          const std::unordered_map<std::string, std::string> &_categoryDetectionModels,
          const std::unordered_map<categories_t, std::string> &_categoryNames,
          const std::unordered_map<std::string, float> &_similarityThreshold);
    ~cli_t() = default;

    void operator()(uint8_t _threads, cmd_t _cmd, char  *const *_path);

private:
    const std::vector<std::string> &m_langCodes;
    const std::unordered_map<std::string, std::string> &m_w2vModels;
    const std::unordered_map<std::string, std::string> &m_newsDetectionModels;
    const std::unordered_map<std::string, std::string> &m_categoryDetectionModels;
    const std::unordered_map<categories_t, std::string> &m_categoryNames;
    const std::unordered_map<std::string, float> &m_similarityThreshold;
};

#endif //TGNEWS_CLI_H
