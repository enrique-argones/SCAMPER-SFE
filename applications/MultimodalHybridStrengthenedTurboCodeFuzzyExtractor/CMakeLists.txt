cmake_minimum_required(VERSION 3.23)

project(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor VERSION 1.0.0 LANGUAGES CXX)

# Multimodal (chimeric subjects) SFE enrollment and evaluation program.
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/SFEModule.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor main.cpp)
sfe_module_target(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor)
set_target_properties(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

sfe_link_modules(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor GMMLite TurboCodes ThreadPool HSTCFE MHSTCFE)
sfe_link_armadillo(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor)
sfe_link_argagg(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor)
target_link_libraries(MultimodalHybridStrengthenedTurboCodeFuzzyExtractor PRIVATE Threads::Threads stdc++fs)

install(TARGETS MultimodalHybridStrengthenedTurboCodeFuzzyExtractor
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Example of the configuration file of the program.
install(DIRECTORY configFiles
        DESTINATION ${CMAKE_INSTALL_DATADIR}/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor
)
