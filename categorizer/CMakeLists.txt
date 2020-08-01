project(categorizer)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/categorizer.h
        ${PROJECT_SOURCE_DIR}/categorizer.cpp
        )

add_library(${CTGR_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${CTGR_LIB}
        ${LIB_LAPACK}
        ${LIB_BLAS}
        ${LIB_DLIB}
        ${LIBS}
        )
