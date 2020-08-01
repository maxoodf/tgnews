project(repository)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/sqliteClient.h
        ${PROJECT_SOURCE_DIR}/sqliteClient.cpp
        ${PROJECT_SOURCE_DIR}/extDocAttr.h
        ${PROJECT_SOURCE_DIR}/repository.h
        ${PROJECT_SOURCE_DIR}/repository.cpp
        ${PROJECT_SOURCE_DIR}/ranker.h
        ${PROJECT_SOURCE_DIR}/ranker.cpp
        )

add_library(${REPO_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${REPO_LIB}
        ${DATA_LOADER_LIB}
        ${EMBEDDER_LIB}
        ${NEWS_LIB}
        ${CTGR_LIB}
        ${DBSCANN_LIB}
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
        ${SQLite3_LIBRARIES}
        ${LIBS}
        )
