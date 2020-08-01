/**
 * @file httpServer/httpServer.cpp
 * @brief
 * @author Max Fomichev
 * @date 25.05.2020
*/

#include <unistd.h>
#include <string.h>

#include <csignal>
#include <iostream>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

#include "httpServer.h"

httpServer_t::httpServer_t(uint8_t _threadPoolSize, const std::string &_ipV4, uint16_t _port) {
    m_eventBase = event_base_new();
    if (m_eventBase == nullptr) {
        throw std::runtime_error("event_base_new() call failed");
    }

    m_httpServer = evhttp_new(m_eventBase);
    if (m_httpServer == nullptr) {
        throw std::runtime_error("evhttp_new() call failed");
    }

    evhttp_set_allowed_methods(m_httpServer, EVHTTP_REQ_PUT | EVHTTP_REQ_DELETE | EVHTTP_REQ_GET);
    evhttp_set_gencb(m_httpServer, requestCallback, this);

    m_boundSocket = evhttp_bind_socket_with_handle(m_httpServer, _ipV4.c_str(), _port);
    if (m_boundSocket == nullptr) {
        throw std::runtime_error("evhttp_bind_socket_with_handle() call failed");
    }

    // set signal handlers
    auto es = evsignal_new(m_eventBase, SIGPIPE, [](evutil_socket_t, short, void *) {
    }, nullptr);
    if (!es || (event_add(es, nullptr) < 0)) {
        throw std::runtime_error("evsignal_new() failed");
    }
    m_events.emplace_back(es);

    es = evsignal_new(m_eventBase, SIGINT, [](evutil_socket_t, short, void *_ctx) {
        auto ctx = static_cast<httpServer_t *>(_ctx);
        event_base_loopexit(ctx->m_eventBase, nullptr); // exit from libevent loop
    }, static_cast<void *>(this));
    if (!es || (event_add(es, nullptr) < 0)) {
        throw std::runtime_error("evsignal_new() failed");
    }
    m_events.emplace_back(es);

    es = evsignal_new(m_eventBase, SIGQUIT, [](evutil_socket_t, short, void *_ctx) {
        auto ctx = static_cast<httpServer_t *>(_ctx);
        event_base_loopexit(ctx->m_eventBase, nullptr); // exit from libevent loop
    }, static_cast<void *>(this));
    if (!es || (event_add(es, nullptr) < 0)) {
        throw std::runtime_error("evsignal_new() failed");
    }
    m_events.emplace_back(es);

    es = evsignal_new(m_eventBase, SIGTERM, [](evutil_socket_t, short, void *_ctx) {
        auto ctx = static_cast<httpServer_t *>(_ctx);
        event_base_loopexit(ctx->m_eventBase, nullptr); // exit from libevent loop
    }, static_cast<void *>(this));
    if (!es || (event_add(es, nullptr) < 0)) {
        throw std::runtime_error("evsignal_new() failed");
    }
    m_events.emplace_back(es);

    // set processed data callback
    es = evsignal_new(m_eventBase, SIGUSR1, sendReplies, this);
    if (!es || (event_add(es, nullptr) < 0)) {
        throw std::runtime_error("evsignal_new() failed");
    }
    m_events.emplace_back(es);

    for (uint8_t i = 0; i < _threadPoolSize; ++i) {
        m_workers.emplace_back(std::thread(&httpServer_t::worker, this));
    }
}

httpServer_t::~httpServer_t() {
    {
        std::unique_lock<std::mutex> lck(m_mtxRequests);
        m_checkRequestsFlag = true;
        m_stopFlag = true;
        m_cvRequests.notify_all();
    }

    // wait for all threads completion
    for (auto &i:m_workers) {
        i.join();
    }

    if (m_eventBase != nullptr) {
        event_base_loopexit(m_eventBase, nullptr); // exit from libevent loop
    }

    // release http server and its socket
    if (m_httpServer != nullptr) {
        evhttp_free(m_httpServer);
        m_httpServer = nullptr;
    }
    m_boundSocket = nullptr;

    // release custom callbacks
    for (const auto i:m_events) {
        event_free(i);
    }

    // release libevent
    if (m_eventBase != nullptr) {
        event_base_free(m_eventBase);
        m_eventBase = nullptr;
    }
}

void httpServer_t::dispatch(putCallback_t _putCallback,
                            deleteCallback_t _deleteCallback,
                            getCallback_t _getCallback,
                            void *_callbackContext) noexcept {
    m_putCallback = std::move(_putCallback);
    m_deleteCallback = std::move(_deleteCallback);
    m_getCallback = std::move(_getCallback);
    m_callbackContext = _callbackContext;

    event_base_dispatch(m_eventBase);
}

void httpServer_t::requestCallback(struct evhttp_request *_req, void *_ctx) noexcept {
    auto httpServer = static_cast<httpServer_t *>(_ctx);

    auto uri = evhttp_request_get_uri(_req);

    auto parsedURI = evhttp_uri_parse(uri);
    if (!parsedURI) {
        std::cerr << "failed to parse URI" << std::endl;
        evhttp_send_error(_req, HTTP_BADREQUEST, "failed to parse URI");
        return;
    }

    std::string decodedPath;
    {
        auto decodedPathTmp = evhttp_uridecode(evhttp_uri_get_path(parsedURI), 0, nullptr);
        if ((decodedPathTmp == nullptr) || (strnlen(decodedPathTmp, 2) != 2)) {
            std::cerr << "evhttp_uridecode() call failed" << std::endl;
            evhttp_send_error(_req, HTTP_BADREQUEST, "failed to parse URI");
            return;
        }
        decodedPath = decodedPathTmp + 1;
        free(decodedPathTmp);
    }

    {
        auto connection = evhttp_request_get_connection(_req);
        if (connection == nullptr) {
            std::cerr << "evhttp_request_get_connection() call failed" << std::endl;
            evhttp_send_error(_req, HTTP_INTERNAL, "internal error");
            return;
        }
    }

    switch (evhttp_request_get_command(_req)) {
        case EVHTTP_REQ_PUT: {
            auto evBuf =  evhttp_request_get_input_buffer(_req);
            if (evBuf == nullptr) {
                std::cerr << "evhttp_request_get_input_buffer() call failed" << std::endl;
                evhttp_send_error(_req, HTTP_INTERNAL, "internal error");
                return;
            }
            std::vector<uint8_t> readBuf(evbuffer_get_length(evBuf));
            evbuffer_remove(evBuf, readBuf.data(), readBuf.size());
            if (readBuf.empty()) {
                std::cerr << "input buffer is empty" << std::endl;
                evhttp_send_error(_req, HTTP_NOCONTENT, "content is empty");
                return;
            }

            std::unordered_map<std::string, std::string> params;
            params.emplace("name", decodedPath);

            auto hdrs = evhttp_request_get_input_headers(_req);
            if (hdrs != nullptr) {
                auto ttlStr = evhttp_find_header(hdrs, "Cache-Control");
                if (ttlStr != nullptr) {
                    std::string tmp(ttlStr);
                    std::string key = "max-age=";
                    auto pos = tmp.find(key);
                    if (pos != std::string::npos) {
                        tmp = tmp.substr(pos + key.length(), key.length() - 1);
                        if (tmp.length() > 0) {
                            params.emplace("ttl", tmp);
                        }
                    }
                }
            }
            std::unique_lock<std::mutex> lck(httpServer->m_mtxRequests);
            httpServer->m_requests.emplace(request_t(_req, requestType_t::PUT, params, readBuf));
            httpServer->m_cvRequests.notify_all();

            break;
        }
        case EVHTTP_REQ_DELETE: {
            std::unordered_map<std::string, std::string> params;
            params.emplace("name", decodedPath);
            std::unique_lock<std::mutex> lck(httpServer->m_mtxRequests);
            httpServer->m_requests.emplace(request_t(_req, requestType_t::DELETE, params));
            httpServer->m_cvRequests.notify_all();

            break;
        }
        case EVHTTP_REQ_GET: {
            if (decodedPath != "threads") {
                std::cerr << "unknown request" << std::endl;
                evhttp_send_error(_req, HTTP_BADREQUEST, "unknown request");
                return;
            }

            auto query = evhttp_uri_get_query(parsedURI);
            if (query == nullptr) {
                std::cerr << "failed to parse URL" << std::endl;
                evhttp_send_error(_req, HTTP_BADREQUEST, "failed to parse URL");
                return;
            }
            struct evkeyvalq queryParams{};
            if (evhttp_parse_query_str(query, &queryParams) != 0) {
                std::cerr << "failed to parse request" << std::endl;
                evhttp_send_error(_req, HTTP_BADREQUEST, "failed to parse request");
                return;
            }

            auto period = evhttp_find_header(&queryParams, "period");
            auto langCode = evhttp_find_header(&queryParams, "lang_code");
            auto category = evhttp_find_header(&queryParams, "category");
            if ((period == nullptr) || (langCode == nullptr) || (category == nullptr)) {
                std::cerr << "some of query parameters are missed" << std::endl;
                evhttp_send_error(_req, HTTP_BADREQUEST, "some of query parameters are missed");
                return;
            }

            std::unordered_map<std::string, std::string> params;
            params.emplace("period", period);
            params.emplace("lang_code", langCode);
            params.emplace("category", category);
            std::unique_lock<std::mutex> lck(httpServer->m_mtxRequests);
            httpServer->m_requests.emplace(request_t(_req, requestType_t::GET, params));
            httpServer->m_cvRequests.notify_all();

            break;
        }
        default: {
            return;
        }
    }
}

void httpServer_t::worker() noexcept {
    while (!m_stopFlag) {
        try {
            request_t request;
            bool hasRec = false;
            {
                // wait for check queue notification (m_checkFlag)
                // and check queue each 200 ms to be sure that we did not miss any notifications
                // while all processing threads were busy
                std::unique_lock<std::mutex> lck(m_mtxRequests);
                while (!m_checkRequestsFlag) {
                    if (m_cvRequests.wait_for(lck, std::chrono::milliseconds(200)) == std::cv_status::timeout) {
                        break;
                    }
                }
                // get first queue record
                if (!m_requests.empty()) {
                    request = std::move(m_requests.front());
                    m_requests.pop();
                    if (m_requests.empty()) {
                        m_checkRequestsFlag = false;
                    }
                    hasRec = true;
                }
            }
            // do we have a new record?
            if (!hasRec) {
                continue;
            }

            switch (request.requestType) {
                case requestType_t::PUT: {
                    std::string name;
                    uint32_t ttl = 0;
                    std::vector<uint8_t> data;
                    auto i = request.params.find("name");
                    if (i != request.params.end()) {
                        name = std::move(i->second);
                    }
                    i = request.params.find("ttl");
                    if (i != request.params.end()) {
                        ttl = std::stoi(i->second);
                    }
                    data = std::move(request.data);

                    {
                        std::string description;
                        auto ret = m_putCallback(name, ttl, data, description, m_callbackContext);
                        std::unique_lock<std::mutex> lck(m_mtxReplies);
                        m_replies.emplace(reply_t(request.request, ret, description));
                    }
                    kill(getpid(), SIGUSR1);

                    break;
                }
                case requestType_t::DELETE: {
                    std::string name;
                    auto i = request.params.find("name");
                    if (i != request.params.end()) {
                        name = std::move(i->second);
                    }

                    {
                        std::string description;
                        auto ret = m_deleteCallback(name, description, m_callbackContext);
                        std::unique_lock<std::mutex> lck(m_mtxReplies);
                        m_replies.emplace(reply_t(request.request, ret, description));
                    }
                    kill(getpid(), SIGUSR1);

                    break;
                }
                case requestType_t::GET: {
                    uint32_t period = 0;
                    std::string langCode;
                    std::string category;
                    auto i = request.params.find("period");
                    if (i != request.params.end()) {
                        period = std::stoi(i->second);
                    }
                    i = request.params.find("lang_code");
                    if (i != request.params.end()) {
                        langCode = std::move(i->second);
                    }
                    i = request.params.find("category");
                    if (i != request.params.end()) {
                        category = std::move(i->second);
                    }

                    {
                        std::string description;
                        std::string data;
                        auto ret = m_getCallback(period, langCode, category, description, data, m_callbackContext);
                        std::unique_lock<std::mutex> lck(m_mtxReplies);
                        m_replies.emplace(reply_t(request.request, ret, description, data));
                    }
                    kill(getpid(), SIGUSR1);

                    break;
                }
                default: {
                }
            }
        } catch (const std::exception &_e) {
            std::cerr << _e.what() << std::endl;
        } catch (...) {
            std::cerr << "unknown error" << std::endl;
        }
    }
}

void httpServer_t::sendReplies(evutil_socket_t, short, void *_ctx) noexcept {
    auto httpServer = static_cast<httpServer_t *>(_ctx);
    // iterate m_replies queue
    while (true) {
        // get first queue record
        reply_t reply;
        {
            std::unique_lock<std::mutex> lck(httpServer->m_mtxReplies);
            if (httpServer->m_replies.empty()) {
                break;
            }
            reply = std::move(httpServer->m_replies.front());
            httpServer->m_replies.pop();
        }

        struct evbuffer *evb = evbuffer_new();
        if (!reply.data.empty()) {
            evhttp_add_header(evhttp_request_get_output_headers(reply.request),
                              "Content-Type", "application/json; charset=UTF-8");
            evbuffer_add(evb, reply.data.c_str(), reply.data.length());
        }
        evhttp_send_reply(reply.request, reply.code, reply.description.c_str(), evb);
        evbuffer_free(evb);
    }
}
