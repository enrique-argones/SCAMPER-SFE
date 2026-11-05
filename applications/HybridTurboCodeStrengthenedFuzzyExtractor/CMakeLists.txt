cmake_minimum_required(VERSION 3.23)

project(HybridTurboCodeStrengthenedFuzzyExtractor VERSION 1.0.0 LANGUAGES CXX)

# Unimodal SFE enrollment and evaluation program.
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/SFEModule.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(HybridTurboCodeStrengthenedFuzzyExtractor main.cpp)
sfe_module_target(HybridTurboCodeStrengthenedFuzzyExtractor)
set_target_properties(HybridTurboCodeStrengthenedFuzzyExtractor PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

sfe_link_modules(HybridTurboCodeStrengthenedFuzzyExtractor GMMLite TurboCodes ThreadPool HSTCFE)
sfe_link_armadillo(HybridTurboCodeStrengthenedFuzzyExtractor)
sfe_link_argagg(HybridTurboCodeStrengthenedFuzzyExtractor)
target_link_libraries(HybridTurboCodeStrengthenedFuzzyExtractor PRIVATE Threads::Threads stdc++fs)

install(TARGETS HybridTurboCodeStrengthenedFuzzyExtractor
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
