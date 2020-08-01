project(cli)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/cli.h
        ${PROJECT_SOURCE_DIR}/cli.cpp
        )

add_library(${CLI_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${CLI_LIB}
        ${DATA_LOADER_LIB}
        ${EMBEDDER_LIB}
        ${NEWS_LIB}
        ${CTGR_LIB}
        ${DBSCANN_LIB}
        ${NEWS_CLUSTER_LIB}
        ${CATEGORY_CLUSTER_LIB}
        ${SIMILARITY_CLUSTER_LIB}
        ${LIB_W2V}
        ${LIB_FAISS}
        ${GUMBO_LDFLAGS}
        ${GUMBO_LIBRARIES}
        ${LIB_CLD3}
        ${Protobuf_LIBRARIES}
        ${ICU_LDFLAGS}
        ${ICU_LIBRARIES}
        ${LIB_LAPACK}
        ${LIB_BLAS}
        ${LIB_DLIB}
        ${LIBS}
        )
