cmake_minimum_required(VERSION 3.23)   # FILE_SET HEADERS needs CMake 3.23

# SCAMPER SFE: Strengthened Fuzzy Extractor software (libraries and programs) developed in the VLAIO ICON project SCAMPER.
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release [-DCMAKE_INSTALL_PREFIX=PREFIX]
#   cmake --build build -j
#   cmake --install build
#
# Every module can also be built on its own from its folder, against the modules already installed (see README.md).

project(SCAMPER_SFE VERSION 1.0.0
        DESCRIPTION "Strengthened Fuzzy Extractor software for biometric template protection"
        LANGUAGES CXX)

include(cmake/SFEModule.cmake)

option(SFE_BUILD_TESTS "Build the unit tests (run them with ctest)" ON)
set(GMMLITE_BUILD_TESTS ${SFE_BUILD_TESTS} CACHE BOOL "Build the GMMLite unit tests" FORCE)
if (SFE_BUILD_TESTS)
    enable_testing()
endif ()

# Libraries, in dependency order.
add_subdirectory(libraries/GMMLite)
add_subdirectory(libraries/ThreadPool)
add_subdirectory(libraries/TurboCodes)
add_subdirectory(libraries/HSTCFE)
add_subdirectory(libraries/MHSTCFE)

# Programs.
add_subdirectory(applications/HybridTurboCodeStrengthenedFuzzyExtractor)
add_subdirectory(applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor)

# End-to-end test of both programs on synthetic data (Release and Debug builds).
if (SFE_BUILD_TESTS)
    # Same results from the enrolling HSTCFE object and from its reloaded template (in-memory API, Rust binding).
    add_executable(test_inmemory_reproduce tests/test_inmemory_reproduce.cpp)
    target_link_libraries(test_inmemory_reproduce PRIVATE HSTCFE)
    add_test(NAME inmemory_reproduce
             COMMAND test_inmemory_reproduce ${CMAKE_CURRENT_BINARY_DIR}/inmemory_reproduce
                     ${CMAKE_CURRENT_SOURCE_DIR}/libraries/TurboCodes/configFiles/best-found-turbo-code_rate_1_3.txt)

    find_program(SFE_PYTHON3 NAMES python3)
    if (SFE_PYTHON3)
        add_test(NAME end_to_end
                 COMMAND ${CMAKE_COMMAND} -E env
                         SFE_UNIMODAL=$<TARGET_FILE:HybridTurboCodeStrengthenedFuzzyExtractor>
                         SFE_MULTIMODAL=$<TARGET_FILE:MultimodalHybridStrengthenedTurboCodeFuzzyExtractor>
                         SFE_TURBOCODES_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/libraries/TurboCodes/configFiles
                         ${SFE_PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/tests/end_to_end.py ${CMAKE_CURRENT_BINARY_DIR}/end_to_end)
    else ()
        message(STATUS "python3 not found: the end_to_end test is not built")
    endif ()
endif ()

# License and README.
install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/SCAMPER_SFE)
