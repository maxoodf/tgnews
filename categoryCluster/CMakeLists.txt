project(categoryCluster)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/categoryCluster.h
        ${PROJECT_SOURCE_DIR}/categoryCluster.cpp
        )

add_library(${CATEGORY_CLUSTER_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${CATEGORY_CLUSTER_LIB}
        ${LIB_LAPACK}
        ${LIB_BLAS}
        ${LIB_DLIB}
        ${LIBS}
        )
