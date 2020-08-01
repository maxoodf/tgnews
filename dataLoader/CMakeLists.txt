project(dataLoader)

set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PRJ_SRCS
        ${PROJECT_SOURCE_DIR}/fileEnumerator.h
        ${PROJECT_SOURCE_DIR}/fileEnumerator.cpp
        ${PROJECT_SOURCE_DIR}/textExtractor.h
        ${PROJECT_SOURCE_DIR}/textExtractor.cpp
        ${PROJECT_SOURCE_DIR}/dataLoader.h
        ${PROJECT_SOURCE_DIR}/dataLoader.cpp
        )

add_library(${DATA_LOADER_LIB} STATIC ${PRJ_SRCS})
target_link_libraries(${DATA_LOADER_LIB}
        ${GUMBO_LDFLAGS}
        ${GUMBO_LIBRARIES}
        ${LIBS})
