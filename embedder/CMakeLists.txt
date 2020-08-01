project(embedder)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/embedder.h
        ${PROJECT_SOURCE_DIR}/embedder.cpp
        )

add_library(${EMBEDDER_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${EMBEDDER_LIB}
        ${ICU_LDFLAGS}
        ${ICU_LIBRARIES}
        ${LIB_W2V}
        ${LIB_FAISS}
        ${LIBS}
        )
