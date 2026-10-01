cmake_minimum_required(VERSION 3.28)

# Prevent in-source builds
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    message(FATAL_ERROR "In-source builds are not permitted! Make sure to use a separate build directory")
endif ()

# define a macro that helps defining an option
macro(zancle_set_option var default type docstring)
    if(NOT DEFINED ${var})
        set(${var} ${default})
    endif()
    set(${var} ${${var}} CACHE ${type} ${docstring} FORCE)
endmacro()

# these options have to be set before CMake detects/configures the toolchain

# When configuring with `--fresh`, also do a fresh configure for fetched dependencies
cmake_policy(SET CMP0168 NEW)

# determine whether to create a debug or release build
zancle_set_option(CMAKE_BUILD_TYPE Release STRING "Choose the type of build (Debug or Release)")
zancle_set_option(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" STRING "The minimal iOS version that will be able to run the built binaries. Cannot be lower than 13.0")

# project name
project(Zancle VERSION 3.1.0)

set(VERSION_IS_RELEASE OFF)


# Platform + compiler detection
include(cmake/Config.cmake)

# we use the paths from the cmake GNUInstallDirs module as defaults
# you can override these if you like
# https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
include(GNUInstallDirs)

# define the path of our additional CMake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/")

# Find Threads once; consumers link `Threads::Threads` as needed.
find_package(Threads REQUIRED)

# disable static libraries on Android
if(ZA_OS_ANDROID)
    set(BUILD_SHARED_LIBS ON)
endif()


# INTERFACE target carrying all build-time toggles; PUBLIC-linked by every
# Zancle target. Definitions are populated below once toggles are resolved.
add_library(zancle-build-options INTERFACE)
target_compile_features(zancle-build-options INTERFACE cxx_std_23)

# Math wrappers (`za::sqrt`, ...) are inline and `gnu::const`: without this flag, e.g. `za::sqrt`
# compiles to a compare, a branch, and a libm call (only to set `errno`) instead of a lone `sqrtss`.
if(ZA_COMPILER_GCC OR ZA_COMPILER_CLANG)
    target_compile_options(zancle-build-options INTERFACE -fno-math-errno)
endif()


# CPM (third-party dependency manager)
include(cmake/CPM.cmake)


# User-facing build options
zancle_set_option(ZA_ENABLE_STACK_TRACES $<CONFIG:Debug> BOOL "TRUE to enable stack traces on assertions, FALSE to disable them")
if(ZA_ENABLE_STACK_TRACES AND NOT ZA_OS_EMSCRIPTEN)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/extlibs/libbacktrace)
endif()

# add an option to enable lifetime tracking
zancle_set_option(ZA_ENABLE_LIFETIME_TRACKING $<CONFIG:Debug> BOOL "TRUE to enable lifetime tracking, FALSE to disable it")

# option to enable precompiled headers
zancle_set_option(ZA_ENABLE_PCH OFF BOOL "TRUE to enable precompiled headers for Zancle builds -- only supported on Windows/Linux and for static library builds")

# add options to select which modules to build
zancle_set_option(ZA_BUILD_GLUTILS ON BOOL "ON to build Zancle's GLUtils module.")
zancle_set_option(ZA_BUILD_WINDOW ON BOOL "ON to build Zancle's Window module. This setting is ignored, if the graphics module is built.")
zancle_set_option(ZA_BUILD_GRAPHICS ON BOOL "ON to build Zancle's Graphics module.")
zancle_set_option(ZA_BUILD_IMGUI ON BOOL "ON to build Zancle's ImGui module.")
zancle_set_option(ZA_BUILD_AUDIO ON BOOL "ON to build Zancle's Audio module.")
zancle_set_option(ZA_BUILD_NETWORK ON BOOL "ON to build Zancle's Network module.")

# add an option for making OpenGL errors fatal
zancle_set_option(ZA_FATAL_OPENGL_ERRORS OFF BOOL "ON to make Zancle OpenGL errors fatal, OFF to simply warn about them")

if(ZA_BUILD_WINDOW)
    # add an option for choosing the OpenGL implementation
    zancle_set_option(ZA_OPENGL_ES ${ZA_OPENGL_ES_DEFAULT} BOOL "ON to use an OpenGL ES implementation, OFF to use a desktop OpenGL implementation")

    # add an option for choosing whether to use the DRM windowing backend
    if(ZA_OS_LINUX)
        zancle_set_option(ZA_USE_DRM OFF BOOL "ON to use DRM windowing backend")
    endif()
endif()

# macOS specific options
if(ZA_OS_MACOS OR ZA_OS_IOS)
    # add an option to build frameworks instead of dylibs (release only)
    zancle_set_option(ZA_BUILD_FRAMEWORKS OFF BOOL "ON to build Zancle as frameworks libraries (release only), OFF to build according to BUILD_SHARED_LIBS")
endif()

# Android options
if(ZA_OS_ANDROID)
    zancle_set_option(ZA_ANDROID_USE_SUSPEND_AWARE_CLOCK OFF BOOL "ON to base za::Clock on CLOCK_BOOTTIME, which keeps advancing while the device is suspended, OFF to use CLOCK_MONOTONIC")

    # avoid missing libraries when building Zancle for Android with NDK r19c and later
    list(PREPEND CMAKE_FIND_ROOT_PATH "${PROJECT_SOURCE_DIR}")

    # install everything in $NDK/sources/ because this path is appended by the NDK (convenient)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_ANDROID_NDK}/sources/third_party/zancle)

    # we install libs in a subdirectory named after the ABI
    set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${CMAKE_ANDROID_ARCH_ABI}")
endif()

# Derived configuration (install dirs, force-build chains, system-deps default)
# Install directories
# For miscellaneous files
if(ZA_OS_WINDOWS OR ZA_OS_IOS)
    set(DEFAULT_INSTALL_MISC_DIR .)
elseif(ZA_OS_LINUX OR ZA_OS_FREEBSD OR ZA_OS_OPENBSD OR ZA_OS_NETBSD)
    set(DEFAULT_INSTALL_MISC_DIR share/Zancle)
elseif(ZA_OS_MACOS)
    set(DEFAULT_INSTALL_MISC_DIR /usr/local/share/Zancle)
elseif(ZA_OS_ANDROID)
    set(DEFAULT_INSTALL_MISC_DIR ${CMAKE_ANDROID_NDK}/sources/third_party/zancle)
endif()

# force building zancle-window, if zancle-graphics module is built
if(ZA_BUILD_GRAPHICS AND NOT ZA_BUILD_WINDOW)
    message(WARNING "You're trying to build Zancle's Graphics module without the Window module. Forcing building of the Window module as a dependency.")
    set(ZA_BUILD_WINDOW ON)
endif()

# force building zancle-graphics, if zancle-imgui module is built
if(ZA_BUILD_IMGUI AND NOT ZA_BUILD_GRAPHICS)
    message(WARNING "You're trying to build Zancle's ImGui module without the Graphics module. Forcing building of the Graphics module as a dependency.")
    set(ZA_BUILD_GRAPHICS TRUE)
endif()

# use bundled dependencies by default on these systems and system dependencies by default on all others (maintains old behavior)
if(ZA_OS_WINDOWS OR ZA_OS_LINUX OR ZA_OS_MACOS OR ZA_OS_IOS OR ZA_OS_ANDROID OR ZA_OS_EMSCRIPTEN)
    set(ZA_USE_SYSTEM_DEPS_DEFAULT OFF)
else()
    set(ZA_USE_SYSTEM_DEPS_DEFAULT ON)
endif()

zancle_set_option(ZA_USE_SYSTEM_DEPS ${ZA_USE_SYSTEM_DEPS_DEFAULT} BOOL "ON to use system dependencies, OFF to use the bundled ones.")

if(ZA_COMPILER_MSVC)
    # add an option to choose whether PDB debug symbols should be generated (defaults to true when possible)
    zancle_set_option(ZA_GENERATE_PDB ON BOOL "True to generate PDB debug symbols, OFF otherwise.")

    # if building using a compiler launcher, embed the MSVC debugging information to allow for caching
    if(CMAKE_CXX_COMPILER_LAUNCHER)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
    endif()
endif()

# define an option for choosing between static and dynamic C runtime (Windows only)
if(ZA_OS_WINDOWS)
    zancle_set_option(ZA_USE_STATIC_STD_LIBS OFF BOOL "ON to statically link to the standard libraries, OFF to use them as DLLs")

    # the following combination of flags is not valid
    if(BUILD_SHARED_LIBS AND ZA_USE_STATIC_STD_LIBS)
        message(FATAL_ERROR "BUILD_SHARED_LIBS and ZA_USE_STATIC_STD_LIBS cannot be used together")
    endif()

    zancle_set_option(ZA_USE_MESA3D OFF BOOL "ON to use the Mesa 3D graphics library for rendering, OFF to use the system provided library for rendering")

    include(cmake/Mesa3D.cmake)
endif()

# setup macOS stuff
if(ZA_OS_MACOS)
    # ZA_BUILD_FRAMEWORKS needs two things:
    # first, it's available only for release
    #    (because cmake currently doesn't allow specifying a custom framework name so XXX-d is not possible)
    # secondly, it works only with BUILD_SHARED_LIBS enabled
    if(ZA_BUILD_FRAMEWORKS)
        # requirement #1 (single-config only; on Xcode the user picks the
        # config at build time and Release is enforced per-build).
        if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
            message(FATAL_ERROR "CMAKE_BUILD_TYPE should be \"Release\" when ZA_BUILD_FRAMEWORKS is ON")
            return()
        endif()

        # requirement #2
        if(NOT BUILD_SHARED_LIBS)
            message(FATAL_ERROR "BUILD_SHARED_LIBS should be ON when ZA_BUILD_FRAMEWORKS is ON")
            return()
        endif()
    endif()

    if(NOT ZA_COMPILER_CLANG)
        message(FATAL_ERROR "Clang is the only supported compiler on macOS")
    endif()
endif()

# Diagnostic / hardening flag toggles
option(ZA_ENABLE_SANITIZERS "Enable sanitizers" OFF)
if(ZA_ENABLE_SANITIZERS)
    string(APPEND CMAKE_CXX_FLAGS " -fno-omit-frame-pointer -fno-sanitize-recover=all -fsanitize=undefined")
endif()


option(ZA_ENABLE_STDLIB_ASSERTIONS "Enable standard library assertions" OFF)
if(ZA_ENABLE_STDLIB_ASSERTIONS)
    # intentionally using `add_compile_definitions` here to propagate defines to subdirectories
    add_compile_definitions(_GLIBCXX_ASSERTIONS=1)                                   # see https://gcc.gnu.org/wiki/LibstdcxxDebugMode

    # Xcode has a property to control this - adding the define ourselves causes multiple definitions
    if(XCODE)
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_STANDARD_LIBRARY_HARDENING "Yes (extensive)")
    else()
        add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE) # see https://libcxx.llvm.org/Hardening.html
    endif()
endif()

option(ZA_ENABLE_HARDENING_FLAGS "Enable hardening flags" OFF)
if(ZA_ENABLE_HARDENING_FLAGS AND NOT MSVC)
    add_compile_options(
        -ftrivial-auto-var-init=pattern
        -fstack-protector-strong
        # -fstack-clash-protection (doesn't work on libc++)
        -fstrict-flex-arrays=3
        # -fno-delete-null-pointer-checks (broken on GCC)
        -ftrapv
    )
endif()

# Output directory + IDE folders + pkg-config install
# set the output directory for Zancle DLLs and executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# enable project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

# on Linux and BSD-like OS, install pkg-config files by default
set(ZA_INSTALL_PKGCONFIG_DEFAULT OFF)

if(ZA_OS_LINUX OR ZA_OS_FREEBSD OR ZA_OS_OPENBSD OR ZA_OS_NETBSD)
    set(ZA_INSTALL_PKGCONFIG_DEFAULT ON)
endif()

zancle_set_option(ZA_INSTALL_PKGCONFIG_FILES ${ZA_INSTALL_PKGCONFIG_DEFAULT} BOOL "ON to automatically install pkg-config files so other projects can find Zancle")

if(ZA_INSTALL_PKGCONFIG_FILES)
    # account for CMAKE_INSTALL_LIBDIR potentially being an absolute path
    file(RELATIVE_PATH ZA_RELATIVE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_FULL_LIBDIR})

    # set pkgconfig install directory
    # this could be e.g. macports on mac or msys2 on windows etc.
    set(ZA_PKGCONFIG_DIR "${ZA_RELATIVE_INSTALL_LIBDIR}/pkgconfig")

    if(ZA_OS_FREEBSD OR ZA_OS_OPENBSD OR ZA_OS_NETBSD)
        set(ZA_PKGCONFIG_DIR "libdata/pkgconfig")
    endif()

    zancle_set_option(ZA_PKGCONFIG_INSTALL_DIR "${ZA_PKGCONFIG_DIR}" PATH "Install directory for Zancle's pkg-config .pc files")

    if(ZA_BUILD_AUDIO AND ZA_BUILD_NETWORK AND ZA_BUILD_GRAPHICS AND ZA_BUILD_WINDOW)
        configure_file(
            "tools/pkg-config/zancle-all.pc.in"
            "tools/pkg-config/zancle-all.pc"
            @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tools/pkg-config/zancle-all.pc"
            DESTINATION "${ZA_PKGCONFIG_INSTALL_DIR}")
    endif()
endif()

# Sanity checks
if(ZA_ENABLE_PCH AND BUILD_SHARED_LIBS)
    message(FATAL_ERROR "Precompiled headers are currently not supported for shared library builds")
endif()

if(ZA_ENABLE_PCH AND ZA_OS_MACOS)
    message(FATAL_ERROR "Precompiled headers are currently not supported in macOS builds")
endif()


# Populate zancle-build-options. Order matches the option declarations above.
target_compile_definitions(zancle-build-options INTERFACE
    _LIBCPP_REMOVE_TRANSITIVE_INCLUDES                                          # avoid transitive includes with libc++
    $<$<BOOL:${ZA_OS_WINDOWS}>:_CRT_SECURE_NO_WARNINGS>                         # suppress CRT warnings on Windows
    $<$<BOOL:${ZA_ENABLE_STACK_TRACES}>:ZA_ENABLE_STACK_TRACES>
    $<$<BOOL:${ZA_ENABLE_LIFETIME_TRACKING}>:ZA_ENABLE_LIFETIME_TRACKING>
    $<$<BOOL:${ZA_ENABLE_PCH}>:ZA_ENABLE_PCH>
    $<$<BOOL:${ZA_BUILD_GLUTILS}>:ZA_BUILD_GLUTILS>
    $<$<BOOL:${ZA_BUILD_WINDOW}>:ZA_BUILD_WINDOW>
    $<$<BOOL:${ZA_BUILD_GRAPHICS}>:ZA_BUILD_GRAPHICS>
    $<$<BOOL:${ZA_BUILD_IMGUI}>:ZA_BUILD_IMGUI>
    $<$<BOOL:${ZA_BUILD_AUDIO}>:ZA_BUILD_AUDIO>
    $<$<BOOL:${ZA_BUILD_NETWORK}>:ZA_BUILD_NETWORK>
    $<$<BOOL:${ZA_OPENGL_ES}>:ZA_OPENGL_ES>
    $<$<BOOL:${ZA_USE_DRM}>:ZA_USE_DRM>
    $<$<BOOL:${ZA_ANDROID_USE_SUSPEND_AWARE_CLOCK}>:ZA_ANDROID_USE_SUSPEND_AWARE_CLOCK>
    $<$<BOOL:${ZA_BUILD_EXAMPLES}>:ZA_BUILD_EXAMPLES>
    $<$<BOOL:${ZA_BUILD_TEST_SUITE}>:ZA_BUILD_TEST_SUITE>
)

# Utility modules aggregated into `zancle-system`. Ordered bottom-up by level
# (Base = 0). Read by the install helper and the framework branch.
set(UTILITY_MODULES
    Base
    Trait
    Math
    Diagnostic
    Vocabulary
    Random
    Container
    Algorithm
    String
    Geometry
    Chrono
    Concurrency
    Fmt
    IO
    Scn
    Err
    Lifetime
    Mixin
)

# Multimedia modules. Each is gated on ZA_BUILD_<UPPER_NAME>.
set(MULTIMEDIA_MODULES
    GLUtils
    Window
    Network
    Graphics
    ImGui
    Audio
)


# Platform libraries + Zancle subdirectory
# Resolve platform-library lists (GL backend, Apple frameworks) once.
include(cmake/PlatformLibs.cmake)

# add the subdirectories
add_subdirectory(src/Zancle)


# Install + export (and the macOS framework variant)
if(NOT ZA_BUILD_FRAMEWORKS)
    # Public headers are attached to each module target via FILE_SET HEADERS
    # in _zancle_install_and_export (cmake/Macros.cmake) and installed
    # transitively; no `install(DIRECTORY include/ ...)` needed.
    install(DIRECTORY cmake/Modules/
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Zancle
            COMPONENT devel)

    if(ZA_GENERATE_PDB)
        install(DIRECTORY ${PROJECT_BINARY_DIR}/lib/
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT devel
                FILES_MATCHING PATTERN "*.pdb")
    endif()
else()
    # Generator expressions in the post-build steps below confuse Xcode's
    # EFFECTIVE_PLATFORM_NAME handling; suppress it.
    set_property(GLOBAL PROPERTY XCODE_EMIT_EFFECTIVE_PLATFORM_NAME OFF)

    # An empty translation unit is enough to satisfy CMake's requirement that
    # a library target have at least one source.
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp
                       COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp
                       VERBATIM)
    add_library(Zancle ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp)

    target_compile_features(Zancle PUBLIC cxx_std_23)
    zancle_set_stdlib(Zancle)

    set_target_properties(Zancle PROPERTIES
        FRAMEWORK                             ON
        FRAMEWORK_VERSION                     ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_IDENTIFIER           org.zancle.Zancle
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_BUNDLE_VERSION       ${PROJECT_VERSION}
        # Let dylibs/frameworks ship inside user app bundles.
        INSTALL_NAME_DIR                      "@rpath"
    )
    if(NOT CMAKE_SKIP_BUILD_RPATH)
        set_target_properties(Zancle PROPERTIES BUILD_WITH_INSTALL_NAME_DIR ON)
    endif()

    # Assemble the framework header set:
    #   - umbrella headers at include/Zancle/
    #   - all 18 utility modules (always built, aggregated under System)
    #   - each enabled multimedia module
    # BASE_DIRS = include/Zancle means relative paths preserve the
    # `<Module>/Foo.hpp` layout that consumers expect via the framework-style
    # `#include <Zancle/<Module>/Foo.hpp>` import (resolved by macOS as
    # Zancle.framework/Headers/<Module>/Foo.hpp).
    # System (umbrella + 18 utility modules) + each enabled multimedia module.
    _zancle_collect_module_headers(System _zancle_framework_headers)
    foreach(_mod IN LISTS MULTIMEDIA_MODULES)
        string(TOUPPER "${_mod}" _flag)
        if(ZA_BUILD_${_flag})
            _zancle_collect_module_headers(${_mod} _h)
            list(APPEND _zancle_framework_headers ${_h})
        endif()
    endforeach()

    target_sources(Zancle PUBLIC
        FILE_SET zancle_framework_headers
        TYPE HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include/Zancle"
        FILES ${_zancle_framework_headers})

    install(TARGETS Zancle
            FRAMEWORK     DESTINATION "."     COMPONENT devel
            FILE_SET      zancle_framework_headers)
endif()

install(FILES license.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES readme.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

zancle_export_targets()


# Extras (doc, examples, tests, format/tidy, CPack) -- top-level builds only
# configure extras by default when building Zancle directly, otherwise hide them
zancle_set_option(ZA_CONFIGURE_EXTRAS ${PROJECT_IS_TOP_LEVEL} BOOL "ON to configure extras, OFF to ignore them")

if(NOT ZA_CONFIGURE_EXTRAS)
    return()
endif()

set(CPACK_PACKAGE_NAME_SUMMARY "Simple and Fast Multimedia Library")
set(CPACK_PACKAGE_VENDOR "Zancle Team")
set(CPACK_PACKAGE_FILE_NAME "Zancle-${PROJECT_VERSION}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-${CMAKE_BUILD_TYPE}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/readme.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/license.md")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Zancle ${PROJECT_VERSION}")
set(CPACK_MONOLITHIC_INSTALL ON)

# NSIS configurations
set(CPACK_NSIS_DISPLAY_NAME "Zancle ${PROJECT_VERSION} (${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION})")
set(CPACK_NSIS_CONTACT "team@zancle.org")
set(NSIS_IMAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/nsis/")
string(REGEX REPLACE "/" "\\\\\\\\" NSIS_IMAGE_PATH ${NSIS_IMAGE_PATH})
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "!define MUI_WELCOMEFINISHPAGE_BITMAP \\\"${NSIS_IMAGE_PATH}sidebar.bmp\\\"\n!define MUI_HEADERIMAGE_BITMAP \\\"${NSIS_IMAGE_PATH}header.bmp\\\"\n!define MUI_ICON \\\"${NSIS_IMAGE_PATH}zancle.ico\\\"")

include(CPack)

# add an option for building the API documentation
zancle_set_option(ZA_BUILD_DOC OFF BOOL "ON to generate the API documentation, OFF to ignore it")
if(ZA_BUILD_DOC)
    add_subdirectory(doc)
endif()

# add an option for building the examples
zancle_set_option(ZA_BUILD_EXAMPLES OFF BOOL "ON to build the Zancle examples, OFF to ignore them")
if(ZA_BUILD_EXAMPLES AND NOT ZA_OS_ANDROID)
    add_subdirectory(examples)
endif()

# add an option for building the test suite
zancle_set_option(ZA_BUILD_TEST_SUITE OFF BOOL "ON to build the Zancle test suite, OFF to ignore it")

# add an option for enabling coverage reporting
zancle_set_option(ZA_ENABLE_COVERAGE OFF BOOL "ON to enable coverage reporting, OFF to ignore it")

if(ZA_BUILD_TEST_SUITE)
    if(ZA_BUILD_WINDOW AND ZA_BUILD_GRAPHICS AND ZA_BUILD_IMGUI AND ZA_BUILD_NETWORK AND ZA_BUILD_AUDIO)
        enable_testing()
        add_subdirectory(test)
    else()
        message(WARNING "Cannot build unit testing unless all modules are enabled")
    endif()
endif()

zancle_set_option(CLANG_FORMAT_EXECUTABLE clang-format STRING "Override clang-format executable, requires version 17")
add_custom_target(format
    COMMAND ${CMAKE_COMMAND} -DCLANG_FORMAT_EXECUTABLE=${CLANG_FORMAT_EXECUTABLE} -P ./cmake/Format.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} VERBATIM)

zancle_set_option(CLANG_TIDY_EXECUTABLE clang-tidy STRING "Override clang-tidy executable, requires minimum version 14")
add_custom_target(tidy
    COMMAND ${CMAKE_COMMAND} -DCLANG_TIDY_EXECUTABLE=${CLANG_TIDY_EXECUTABLE} -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR} -P ./cmake/Tidy.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} VERBATIM)
