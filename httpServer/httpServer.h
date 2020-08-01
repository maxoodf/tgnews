/**
 * @file httpServer/httpServer.h
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/


#ifndef TGNEWS_HTTPSERVER_H
#define TGNEWS_HTTPSERVER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

struct event_base;
struct evhttp;
struct evhttp_bound_socket;
struct event;
struct evhttp_request;

class httpServer_t {
public:
    using putCallback_t = std::function<uint16_t(const std::string &,
                                                 uint32_t,
                                                 const std::vector<uint8_t> &,
                                                 std::string &,
                                                 void *)>;
    using deleteCallback_t = std::function<uint16_t(const std::string &,
                                                    std::string &,
                                                    void *)>;
    using getCallback_t = std::function<uint16_t(uint32_t,
                                                 const std::string &,
                                                 const std::string &,
                                                 std::string &,
                                                 std::string &,
                                                 void *)>;

    httpServer_t(uint8_t _threadPoolSize, const std::string &_ipV4, uint16_t _port);
    ~httpServer_t();

    void dispatch(putCallback_t _putCallback,
                  deleteCallback_t _deleteCallback,
                  getCallback_t _getCallback,
                  void *_callbackContext) noexcept;

private:
    event_base *m_eventBase = nullptr;
    evhttp *m_httpServer = nullptr;
    evhttp_bound_socket *m_boundSocket = nullptr;
    std::vector<struct event *> m_events;

    std::vector<std::thread> m_workers;

    enum class requestType_t {
        UNKNOWN,
        PUT,
        DELETE,
        GET
    };
    struct request_t {
        evhttp_request *request = nullptr;
        requestType_t requestType = requestType_t::UNKNOWN;
        std::unordered_map<std::string, std::string> params;
        std::vector<uint8_t> data;

        request_t() = default;
        request_t(evhttp_request *_request,
                  requestType_t _requestType,
                  std::unordered_map<std::string, std::string> &_params): request(_request),
                                                                          requestType(_requestType),
                                                                          params(std::move(_params)) {}
        request_t(evhttp_request *_request,
                  requestType_t _requestType,
                  std::unordered_map<std::string, std::string> &_params,
                  std::vector<uint8_t> &_data): request(_request),
                                                requestType(_requestType),
                                                params(std::move(_params)),
                                                data(std::move(_data)) {}
        ~request_t() = default;
    };
    std::queue<request_t> m_requests;
    std::mutex m_mtxRequests;
    std::condition_variable m_cvRequests;
    std::atomic<bool> m_checkRequestsFlag {false};

    struct reply_t {
        evhttp_request *request = nullptr;
        uint16_t code = 0;
        std::string description;
        std::string data;

        reply_t() = default;
        reply_t(evhttp_request *_request,
                uint16_t _code,
                std::string &_description): request(_request),
                                            code(_code),
                                            description(std::move(_description)) {}
        reply_t(evhttp_request *_request,
                uint16_t _code,
                std::string &_description,
                std::string &_data): request(_request),
                                     code(_code),
                                     description(std::move(_description)),
                                     data(std::move(_data)) {}
        ~reply_t() = default;
    };
    std::queue<reply_t> m_replies;
    std::mutex m_mtxReplies;

    std::atomic<bool> m_stopFlag {false};

    putCallback_t m_putCallback = nullptr;
    deleteCallback_t m_deleteCallback = nullptr;
    getCallback_t m_getCallback = nullptr;
    void *m_callbackContext = nullptr;

    static void requestCallback(struct evhttp_request *_req, void *_data) noexcept;
    static void sendReplies(int, short, void *_ctx) noexcept;

    void worker() noexcept;
};

#endif //TGNEWS_HTTPSERVER_H
