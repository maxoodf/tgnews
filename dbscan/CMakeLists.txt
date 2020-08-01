project(dbscan)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/dbscan.h
        ${PROJECT_SOURCE_DIR}/dbscan.cpp
        )

add_library(${DBSCANN_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${DBSCANN_LIB}
        ${LIBS}
        )
