cmake_minimum_required(VERSION 3.23)   # FILE_SET HEADERS needs CMake 3.23

project(TurboCodes VERSION 1.0.0 LANGUAGES CXX)

# TurboCodes: soft-decoded Turbo Codes, PSK / 2-PAM constellations and the text file readers.
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/SFEModule.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(TurboCodes SHARED
    constellation.h pam2.h psk.h turbocode.h turbo-codes_global.h turbo-codes_types.h vectorfiles.h
    constellation.cpp pam2.cpp psk.cpp turbocode.cpp vectorfiles.cpp
)
sfe_module_target(TurboCodes)
target_compile_definitions(TurboCodes PRIVATE TURBOCODES_LIBRARY)

target_sources(TurboCodes
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS "./"
        FILES constellation.h pam2.h psk.h turbocode.h turbo-codes_global.h turbo-codes_types.h vectorfiles.h
)

sfe_link_modules(TurboCodes GMMLite)
sfe_link_armadillo(TurboCodes)

install(TARGETS TurboCodes
        FILE_SET HEADERS
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/TurboCodes
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Turbo Code configurations (generators of the best codes found for rates 1/3 to 1/6).
install(DIRECTORY configFiles
        DESTINATION ${CMAKE_INSTALL_DATADIR}/TurboCodes
)
