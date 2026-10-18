cmake_minimum_required(VERSION 3.19)
project(ModGear)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(MODGEAR_PLATFORM "omp" CACHE STRING "omp or samp")
set(MODGEAR_EDITION "full" CACHE STRING "full or alfa")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# mbedTLS: HTTPS (медиасервер, Twitch, внешний IP)
include(FetchContent)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
set(GEN_FILES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(mbedtls
    URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.2/mbedtls-3.6.2.tar.bz2
    URL_HASH SHA256=8b54fb9bcf4d5a7078028e0520acddefb7900b3e66fec7f7175bb5b7d85ccdca
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(mbedtls)

if(MODGEAR_PLATFORM STREQUAL "omp")
    add_subdirectory(lib/omp-sdk)
    add_subdirectory(lib/omp-net)
    set_property(TARGET OMP-SDK PROPERTY FOLDER "Server")
    set_property(TARGET OMP-Network PROPERTY FOLDER "Server")
endif()

# RakNet open.mp для медиасервера вебкамеры AZRTC (lib/raknet, без submodule)
if(NOT MODGEAR_EDITION STREQUAL "alfa")
    add_subdirectory(lib/raknet)
    set_property(TARGET raknet-azrtc PROPERTY FOLDER "Server")
endif()

add_subdirectory(server)
set_property(TARGET ModGear-server PROPERTY FOLDER "Server")
