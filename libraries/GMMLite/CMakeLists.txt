cmake_minimum_required(VERSION 3.23)   # FILE_SET HEADERS needs CMake 3.23

project(GMMLite VERSION 1.0.0 LANGUAGES CXX)

# GMMLite: minimal diagonal-covariance Gaussian Mixture Models (C++ standard library only).
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/SFEModule.cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GMMLITE_BUILD_TESTS "Build the GMMLite unit tests (run them with ctest)" OFF)

add_library(GMMLite SHARED
    GMMLite_global.h
    gmm.h
    gmm.cpp
)
sfe_module_target(GMMLite)

# Only the symbols marked with GMMLITE_EXPORT are exported (-fvisibility=hidden).
target_compile_options(GMMLite PRIVATE -fvisibility=hidden -Wall -Wextra)
target_compile_definitions(GMMLite PRIVATE GMMLITE_LIBRARY)
set_target_properties(GMMLite PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

target_sources(GMMLite
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS "./"
        FILES GMMLite_global.h gmm.h
)

# The headers are installed to PREFIX/include/GMMLite, so users include <GMMLite/gmm.h>.
install(TARGETS GMMLite
        FILE_SET HEADERS
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GMMLite
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (GMMLITE_BUILD_TESTS)
    enable_testing()
    add_executable(test_gmmlite tests/test_gmmlite.cpp)
    target_link_libraries(test_gmmlite GMMLite)
    target_include_directories(test_gmmlite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME test_gmmlite COMMAND test_gmmlite)

    # Optional bit-exactness test against the GMM class of the CHMM library. CHMM is not part of SCAMPER SFE: the test
    # is only built when an installed CHMM (and Armadillo) is found through CMAKE_PREFIX_PATH.
    find_package(Armadillo QUIET)
    find_library(CHMM_LIBRARY NAMES CHMM)
    find_path(CHMM_INCLUDE_DIR NAMES chmm-library/gmm_pthreaded.hh)
    if (CHMM_LIBRARY AND CHMM_INCLUDE_DIR AND Armadillo_FOUND)
        add_executable(compare_with_chmm tests/compare_with_chmm.cpp)
        target_include_directories(compare_with_chmm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CHMM_INCLUDE_DIR} ${ARMADILLO_INCLUDE_DIRS})
        target_link_libraries(compare_with_chmm GMMLite ${CHMM_LIBRARY} ${ARMADILLO_LIBRARIES})
        add_test(NAME compare_with_chmm COMMAND compare_with_chmm)
    else ()
        message(STATUS "CHMM not found: the optional compare_with_chmm test is not built")
    endif ()
endif ()
