cmake_minimum_required(VERSION 3.23)   # FILE_SET HEADERS needs CMake 3.23

project(HSTCFE VERSION 1.0.0 LANGUAGES CXX)

# HSTCFE: Hybrid Strengthened Turbo-Code Fuzzy Extractor (unimodal).
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/SFEModule.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(HSTCFE SHARED
    HSTCFE_global.h
    hybridstrengthenedturbocodefuzzyextractor.h
    hybridstrengthenedturbocodefuzzyextractor.cpp
)
sfe_module_target(HSTCFE)
target_compile_definitions(HSTCFE PRIVATE HSTCFE_LIBRARY)

target_sources(HSTCFE
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS "./"
        FILES HSTCFE_global.h hybridstrengthenedturbocodefuzzyextractor.h
)

sfe_link_modules(HSTCFE GMMLite TurboCodes ThreadPool)
sfe_link_armadillo(HSTCFE)
target_link_libraries(HSTCFE PUBLIC Threads::Threads stdc++fs)

install(TARGETS HSTCFE
        FILE_SET HEADERS
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/HSTCFE
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Example of the general configuration file read by HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData.
install(DIRECTORY configFiles
        DESTINATION ${CMAKE_INSTALL_DATADIR}/HSTCFE
)
