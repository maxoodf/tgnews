cmake_minimum_required(VERSION 3.5)
project(tgnews)

set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_BINARY_DIR ${PROJECT_ROOT_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Werror -fPIC")
if (${WITH_FAISS})
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_FAISS")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")

set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -funroll-loops -ftree-vectorize -DNDEBUG")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(LIBS "-lz -lm -ldl -lpthread")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif()

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig")

find_package(OpenMP REQUIRED)
if (${OPENMP_LIBRARIES})
    link_directories("${OPENMP_LIBRARIES}")
endif()
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

pkg_check_modules(GUMBO REQUIRED gumbo)
include_directories(${GUMBO_INCLUDE_DIRS})

pkg_check_modules(ICU REQUIRED icu-i18n)
include_directories(${ICU_INCLUDE_DIRS})

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

find_path(CLD3_INCLUDE_DIR cld_3/nnet_language_identifier.h
        /usr/local/include/google
        /usr/local/include
        /usr/include/google
        /usr/include
        )
include_directories(${CLD3_INCLUDE_DIR})
find_library(LIB_CLD3 cld3)

find_path(W2V_INCLUDE_DIR word2vec.hpp
        /usr/local/opt/include
        /usr/local/include
        /usr/include
        )
include_directories(${W2V_INCLUDE_DIR})
find_library(LIB_W2V word2vec)

if (${WITH_FAISS})
    find_path(FAISS_INCLUDE_DIR faiss/IndexIVFPQ.h
            /usr/local/opt/faiss/include
            /usr/local/include
            /usr/include
            )
    include_directories(${FAISS_INCLUDE_DIR})
    find_library(LIB_FAISS faiss)
endif()

find_path(DLIB_INCLUDE_DIR dlib/svm_threaded.h
        /usr/local/include
        /usr/include
        )
include_directories(${DLIB_INCLUDE_DIR})
find_library(LIB_LAPACK lapack)
find_library(LIB_BLAS blas)
find_library(LIB_DLIB dlib)

find_package(RapidJSON REQUIRED)
include_directories(${RapidJSON_INCLUDES})

find_package(Libevent REQUIRED)
include_directories(${LIBEVENT_INCLUDE_DIRS})

find_package(SQLite3)
include_directories(${SQLite3_INCLUDE_DIRS})
if (NOT ${SQLite3_FOUND})
    pkg_check_modules(SQLite3 REQUIRED sqlite3)
    include_directories(${SQLITE3_INCLUDE_DIRS})
    SET(SQLite3_LIBRARIES ${SQLITE3_LIBRARIES})
endif ()

set(LOCAL_INCLUDE_DIR ${PROJECT_ROOT_DIR})
include_directories(${LOCAL_INCLUDE_DIR})

set(DATA_LOADER_LIB ${PROJECT_NAME}_dtld)
set(EMBEDDER_LIB ${PROJECT_NAME}_embd)
set(NEWS_LIB ${PROJECT_NAME}_news)
set(CTGR_LIB ${PROJECT_NAME}_ctgr)
set(DBSCANN_LIB ${PROJECT_NAME}_dbsn)
set(NEWS_CLUSTER_LIB ${PROJECT_NAME}_nscr)
set(CATEGORY_CLUSTER_LIB ${PROJECT_NAME}_crcr)
set(SIMILARITY_CLUSTER_LIB ${PROJECT_NAME}_smcr)
set(CLI_LIB ${PROJECT_NAME}_cli)
set(HTTP_LIB ${PROJECT_NAME}_http)
set(REPO_LIB ${PROJECT_NAME}_repo)

add_subdirectory(dataLoader)
add_subdirectory(embedder)
add_subdirectory(newsDetector)
add_subdirectory(categorizer)
add_subdirectory(dbscan)
add_subdirectory(newsCluster)
add_subdirectory(categoryCluster)
add_subdirectory(similarityCluster)
add_subdirectory(cli)
add_subdirectory(httpServer)
add_subdirectory(repository)

set(TGNEWS ${PROJECT_NAME})
set(TGNEWS_FILES
        ${PROJECT_SOURCE_DIR}/config.h
        ${PROJECT_SOURCE_DIR}/types.h
        ${PROJECT_SOURCE_DIR}/main.cpp
        )
add_executable(${TGNEWS} ${TGNEWS_FILES})
target_link_libraries(${TGNEWS}
        ${DATA_LOADER_LIB}
        ${EMBEDDER_LIB}
        ${NEWS_LIB}
        ${CTGR_LIB}
        ${DBSCANN_LIB}
        ${NEWS_CLUSTER_LIB}
        ${CATEGORY_CLUSTER_LIB}
        ${SIMILARITY_CLUSTER_LIB}
        ${CLI_LIB}
        ${HTTP_LIB}
        ${REPO_LIB}
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
        ${LIBEVENT_STATIC_LIBRARIES}
        ${SQLite3_LIBRARIES}
        ${LIBS}
        )
