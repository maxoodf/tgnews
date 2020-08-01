project(similarityCluster)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/similarityCluster.h
        ${PROJECT_SOURCE_DIR}/similarityCluster.cpp
        )

add_library(${SIMILARITY_CLUSTER_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${SIMILARITY_CLUSTER_LIB}
        ${DBSCANN_LIB}
        ${LIB_LAPACK}
        ${LIB_BLAS}
        ${LIB_DLIB}
        ${LIBS}
        )
