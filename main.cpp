/**
 * @file main.cpp
 * @brief Data Clustering Contest
 * @author Max Fomichev
 * @date 02.12.2019
*/

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include <chrono>

#include "config.h"
#include "types.h"
#include "cli/cli.h"
#include "httpServer/httpServer.h"
#include "repository/repository.h"

static void usage(const char *_name) {
    std::cout  << _name << " [command] [param]" << std::endl
               << "  Commands:" << std::endl
               << "    languages" << std::endl
               << "      Isolate articles in English and Russian from [param] folder" << std::endl
               << "    news" << std::endl
               << "      Isolate news articles from [param] folder" << std::endl
               << "    categories" << std::endl
               << "      Group news articles by category from [param] folder" << std::endl
               << "    threads" << std::endl
               << "      Group similar news into threads from [param] folder" << std::endl
               << "    server <port>" << std::endl
               << "      Run as an HTTP server on port [param]" << std::endl;
}

int main(int argc, char *argv[]) {
    try {
        if (argc != 3) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }

        cmd_t cmd;
        {
            std::string command = argv[1];
            if (command == "languages") {
                cmd = cmd_t::LNG;
            } else if (command == "news") {
                cmd = cmd_t::NWS;
            } else if (command == "categories") {
                cmd = cmd_t::CTG;
            } else if (command == "threads") {
                cmd = cmd_t::THR;
            } else if (command == "server") {
                cmd = cmd_t::SRV;
            } else {
                usage(argv[0]);
                return EXIT_FAILURE;
            }
        }

// Prepare settings data
        std::vector<std::string> langCodes;
        std::unordered_map<std::string, std::string> w2vModels;
        std::unordered_map<std::string, std::string> newsDetectionModels;
        std::unordered_map<std::string, std::string> categoryDetectionModels;
        std::unordered_map<std::string, std::string> weightDetectionModels;
        std::unordered_map<std::string, float> similarityThreshold;
        std::unordered_map<std::string, std::string> indexFiles;
        std::size_t i = 0;
        while ((g_langCodes[i] != nullptr) && (g_w2vModels[i] != nullptr)) {
            langCodes.emplace_back(g_langCodes[i]);
            w2vModels.emplace(g_langCodes[i], g_w2vModels[i]);
            newsDetectionModels.emplace(g_langCodes[i], g_newsDetectionModels[i]);
            categoryDetectionModels.emplace(g_langCodes[i], g_categoryDetectionModels[i]);
            weightDetectionModels.emplace(g_langCodes[i], g_weightDetectionModels[i]);
            similarityThreshold.emplace(g_langCodes[i], g_similarityThreshold[i]);
            indexFiles.emplace(g_langCodes[i], g_indexFiles[i]);
            ++i;
        }
        const std::unordered_map<categories_t, std::string> categoryNames {
                {categories_t::SOCIETY, "society"},
                {categories_t::ECONOMY, "economy"},
                {categories_t::TECHNOLOGY, "technology"},
                {categories_t::SPORTS, "sports"},
                {categories_t::ENTERTAINMENT, "entertainment"},
                {categories_t::SCIENCE, "science"},
                {categories_t::OTHER, "other"},
        };

        auto threads = std::thread::hardware_concurrency();
        if (threads < g_threads) {
            threads = g_threads;
        }

        if (cmd == cmd_t::SRV) {
            std::unique_ptr<repository_t> repository;
            try {
                repository = std::make_unique<repository_t>(threads,
                                                            langCodes,
                                                            w2vModels,
                                                            newsDetectionModels,
                                                            categoryDetectionModels,
                                                            weightDetectionModels,
                                                            categoryNames,
                                                            similarityThreshold,
                                                            g_sqliteFile,
                                                            indexFiles);
            } catch (const std::exception &_e) {
                std::cerr << _e.what() << std::endl;
                return EXIT_FAILURE;
            } catch (...) {
                std::cerr << "filed to create repository, unknown error" << std::endl;
                return EXIT_FAILURE;
            }

            std::unique_ptr<httpServer_t> httpServer;
            try {
                httpServer = std::make_unique<httpServer_t>(threads, "0.0.0.0", std::stoi(argv[2]));
            } catch (const std::exception &_e) {
                std::cerr << _e.what() << std::endl;
                return EXIT_FAILURE;
            } catch (...) {
                std::cerr << "filed to create HTTP server, unknown error" << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << "server is running on " << argv[2] << " port" << std::endl;
            httpServer->dispatch(repository_t::onPut, repository_t::onDelete, repository_t::onGet, repository.get());
            std::cout << "server is shutting down" << std::endl;
        } else {
            const auto processingStarted = std::chrono::high_resolution_clock::now();
            cli_t cli(langCodes,
                      w2vModels,
                      newsDetectionModels,
                      categoryDetectionModels,
                      categoryNames,
                      similarityThreshold);
            cli(threads, cmd, argv + 2);
            auto processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - processingStarted
            ).count();
            std::cerr << std::endl << "Processed in " << processingTime << " ms" << std::endl;
        }

        return EXIT_SUCCESS;
    } catch(const std::exception &_e) {
        std::cerr << _e.what() << std::endl;
    } catch(...) {
        std::cerr << "Unknown error" << std::endl;
    }

    return EXIT_FAILURE;
}
