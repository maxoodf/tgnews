project(newsCluster)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/newsCluster.h
        ${PROJECT_SOURCE_DIR}/newsCluster.cpp
        )

add_library(${NEWS_CLUSTER_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${NEWS_CLUSTER_LIB}
        ${LIB_LAPACK}
        ${LIB_BLAS}
        ${LIB_DLIB}
        ${LIBS}
        )
