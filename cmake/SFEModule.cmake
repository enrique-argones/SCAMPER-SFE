# SFEModule.cmake - settings and helpers shared by every module of SCAMPER SFE.
#
# Every module CMakeLists.txt includes this file. A module can be configured in two ways:
#
#  * as part of the whole repository (top-level CMakeLists.txt, add_subdirectory): the SFE modules it depends on are
#    CMake targets of the same build tree and are linked directly;
#  * on its own (cmake -S libraries/HSTCFE ...): the SFE modules it depends on must already be installed, and they are
#    located with find_library / find_path under CMAKE_PREFIX_PATH, the install prefix and the system folders.
#
# sfe_link_modules() hides the difference between both cases.

include_guard(GLOBAL)
include(GNUInstallDirs)

# Debug libraries and programs get the _Debug suffix, so Release and Debug builds can be installed in the same prefix.
set(CMAKE_DEBUG_POSTFIX _Debug)

# Installed libraries and programs find the SFE libraries next to them ($ORIGIN/../lib) and the external libraries they
# were linked with, so they run from any install prefix without setting LD_LIBRARY_PATH.
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release or Debug)" FORCE)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SFE_LIBRARY_SUFFIX ${CMAKE_DEBUG_POSTFIX})
else ()
    set(SFE_LIBRARY_SUFFIX "")
endif ()

# Header used to locate an installed SFE module (the folder that contains it is added to the include path).
set(SFE_HEADER_GMMLite    GMMLite/gmm.h)
set(SFE_HEADER_ThreadPool ThreadPool/ThreadPool.hpp)
set(SFE_HEADER_TurboCodes TurboCodes/turbocode.h)
set(SFE_HEADER_HSTCFE     HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h)
set(SFE_HEADER_MHSTCFE    MHSTCFE/mhstce.h)

# sfe_module_target(<target>)
#   Common properties of an SFE library or program: DEBUG is defined in Debug builds (verbose tracing), and the parent
#   folder of the module is a build-tree include directory, so <Module/header.h> resolves inside the repository too.
function(sfe_module_target TARGET)
    target_compile_definitions(${TARGET} PRIVATE $<$<CONFIG:Debug>:DEBUG>)
    get_target_property(TYPE ${TARGET} TYPE)
    if (TYPE STREQUAL "SHARED_LIBRARY")
        get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
        target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${PARENT_DIR}>)
    endif ()
endfunction()

# sfe_link_modules(<target> <module>...)
#   Links <target> with the given SFE modules (GMMLite, ThreadPool, TurboCodes, HSTCFE, MHSTCFE): the targets of the same
#   build tree when they exist, otherwise the installed libraries (with the _Debug suffix in Debug builds).
function(sfe_link_modules TARGET)
    foreach (MODULE IN LISTS ARGN)
        if (TARGET ${MODULE})
            target_link_libraries(${TARGET} PUBLIC ${MODULE})
        else ()
            string(TOUPPER ${MODULE} UPPER)
            find_library(${UPPER}_LIBRARY NAMES ${MODULE}${SFE_LIBRARY_SUFFIX} REQUIRED)
            find_path(${UPPER}_INCLUDE_DIR NAMES ${SFE_HEADER_${MODULE}} REQUIRED)
            target_link_libraries(${TARGET} PUBLIC ${${UPPER}_LIBRARY})
            target_include_directories(${TARGET} PUBLIC ${${UPPER}_INCLUDE_DIR})
        endif ()
    endforeach ()
endfunction()

# sfe_link_armadillo(<target>)
#   Armadillo (linear algebra) located with CMake's FindArmadillo module.
function(sfe_link_armadillo TARGET)
    find_package(Armadillo REQUIRED)
    target_include_directories(${TARGET} PUBLIC ${ARMADILLO_INCLUDE_DIRS})
    target_link_libraries(${TARGET} PUBLIC ${ARMADILLO_LIBRARIES})
endfunction()

# sfe_link_argagg(<target>)
#   argagg, the header-only command-line parser used by the programs (argagg/argagg.hpp).
function(sfe_link_argagg TARGET)
    find_path(ARGAGG_INCLUDE_DIR NAMES argagg/argagg.hpp REQUIRED)
    target_include_directories(${TARGET} PRIVATE ${ARGAGG_INCLUDE_DIR})
endfunction()
