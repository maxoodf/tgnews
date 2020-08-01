project(httpServer)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/httpServer.h
        ${PROJECT_SOURCE_DIR}/httpServer.cpp
        )

add_library(${HTTP_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${HTTP_LIB}
        ${LIBEVENT_STATIC_LIBRARIES}
        ${LIBS})
