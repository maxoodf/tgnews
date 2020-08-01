/**
 * @file config.h
 * @brief
 * @author Max Fomichev
 * @date 02.12.2019
*/

#ifndef TGNEWS_CONFIG_H
#define TGNEWS_CONFIG_H

#include <cstdint>

static const uint8_t g_threads = 8;

static const char *g_sqliteFile = "../db/tgnews.sqlite";

static const char *g_langCodes[] = {
        "en",
        "ru",
        nullptr
};

static const char *g_w2vModels[] = {
#ifdef WITH_FAISS
        "../models/en_cb_ns_s512_w5_ivfpq256.faiss",
        "../models/ru_cb_ns_s512_w5_ivfpq256.faiss",
#else
        "../models/en_cb_ns_s512_w5.w2v",
        "../models/ru_cb_ns_s512_w5.w2v",
#endif
        nullptr
};

static const char *g_newsDetectionModels[] = {
        "../models/en_binary.dlib",
        "../models/ru_binary.dlib",
        nullptr
};

static const char *g_categoryDetectionModels[] = {
        "../models/en_multi.dlib",
        "../models/ru_multi.dlib",
        nullptr
};

static const char *g_weightDetectionModels[] = {
        "../models/en_weight.dlib",
        "../models/ru_weight.dlib",
        nullptr
};

static float g_similarityThreshold[] = {
        0.895f,
        0.89f
};

static const char *g_indexFiles[] = {
        "../db/en.d2v",
        "../db/ru.d2v",
        nullptr
};

#endif //TGNEWS_CONFIG_H
