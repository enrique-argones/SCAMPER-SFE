cmake_minimum_required(VERSION 3.23)   # FILE_SET HEADERS needs CMake 3.23

project(MHSTCFE VERSION 1.0.0 LANGUAGES CXX)

# MHSTCFE: Multimodal Hybrid Strengthened Turbo-Code Fuzzy Extractor (feature-level fusion).
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/SFEModule.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(MHSTCFE SHARED
    MHSTCFE_global.h
    mhstce.h
    mhstce.cpp
)
sfe_module_target(MHSTCFE)
target_compile_definitions(MHSTCFE PRIVATE MHSTCE_LIBRARY)

target_sources(MHSTCFE
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS "./"
        FILES MHSTCFE_global.h mhstce.h
)

sfe_link_modules(MHSTCFE GMMLite TurboCodes ThreadPool HSTCFE)
sfe_link_armadillo(MHSTCFE)
target_link_libraries(MHSTCFE PUBLIC Threads::Threads stdc++fs)

install(TARGETS MHSTCFE
        FILE_SET HEADERS
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MHSTCFE
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
