set(INCROOT ${PROJECT_SOURCE_DIR}/include/Zancle/Window)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/Zancle/Window)

file(GLOB SRC CONFIGURE_DEPENDS "${INCROOT}/*.hpp" "${INCROOT}/*.inl"  "${SRCROOT}/*.hpp" "${SRCROOT}/*.inl" "${SRCROOT}/*.cpp" "${SRCROOT}/Stub/*.hpp" "${SRCROOT}/Stub/*.cpp")

set(SDL_STATIC ON)
set(SDL_SHARED OFF)

set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)

set(SDL_CCACHE ON)

set(SDL_AUDIO OFF)
set(SDL_CAMERA OFF)
set(SDL_DIALOG OFF)
set(SDL_FILESYSTEM OFF)
set(SDL_FSOPS OFF)
set(SDL_GPU OFF)
set(SDL_HIDAPI OFF)
set(SDL_LOCALE OFF)
set(SDL_MISC OFF)
set(SDL_POWER OFF)
set(SDL_PROCESS OFF)
set(SDL_RENDER OFF)
set(SDL_SENSOR OFF)
set(SDL_STORAGE OFF)
set(SDL_TRAY OFF)

if(EXISTS "${PROJECT_SOURCE_DIR}/../SDL/CMakeLists.txt")
    message(STATUS "Found local SDL dependency. Adding it from ../SDL.")
    add_subdirectory("${PROJECT_SOURCE_DIR}/../SDL" sdl SYSTEM)
else()
    CPMAddPackage(
        NAME sdl
        GITHUB_REPOSITORY vittorioromeo/SDL
        GIT_TAG 6b010e2856c98ab6743d9dc6244cffd987151c32
        SYSTEM TRUE
    )
endif()

if (TARGET SDL3-static)
    get_property(sdl3_include_dirs TARGET SDL3-static PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    set_property(TARGET SDL3-static PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${sdl3_include_dirs}")
    target_compile_options(SDL3-static PRIVATE "-w" "-pthread")
    set_target_properties(SDL3-static PROPERTIES UNITY_BUILD OFF)
    target_link_libraries(SDL3-static PRIVATE Threads::Threads)
endif()

if (TARGET SDL3-dynamic)
    get_property(sdl3_include_dirs TARGET SDL3-dynamic PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    set_property(TARGET SDL3-dynamic PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${sdl3_include_dirs}")
    target_compile_options(SDL3-dynamic PRIVATE "-w" "-pthread")
    set_target_properties(SDL3-dynamic PROPERTIES UNITY_BUILD OFF)
    target_link_libraries(SDL3-dynamic PRIVATE Threads::Threads)
endif()



# add platform specific sources
if(ZA_OS_WINDOWS)
    set(PLATFORM_SRC
        ${SRCROOT}/Win32/Utils.hpp
        ${SRCROOT}/Win32/Win32ImplUnity.cpp
    )
elseif(ZA_OS_IOS)
    enable_language(OBJCXX)
    set(PLATFORM_SRC
        ${SRCROOT}/iOS/SensorImpl.mm
        ${SRCROOT}/iOS/SensorImpl.hpp
        ${SRCROOT}/iOS/ObjCType.hpp
        ${SRCROOT}/iOS/SFAppDelegate.hpp
        ${SRCROOT}/iOS/SFAppDelegate.mm
        ${SRCROOT}/iOS/SFView.hpp
        ${SRCROOT}/iOS/SFView.mm
        ${SRCROOT}/iOS/SFViewController.hpp
        ${SRCROOT}/iOS/SFViewController.mm
        ${SRCROOT}/iOS/SFMain.hpp
        ${SRCROOT}/iOS/SFMain.mm
    )
elseif(ZA_OS_ANDROID)
    set(PLATFORM_SRC
        ${SRCROOT}/Android/SensorImpl.cpp
        ${SRCROOT}/Android/SensorImpl.hpp
    )
endif()

# define the zancle-window target
zancle_add_library(Window
                 SOURCES ${SRC} ${PLATFORM_SRC}
                 DEPENDENCIES "Dependencies.cmake.in")

# DRM libraries
if(ZA_OS_LINUX OR ZA_OS_FREEBSD OR ZA_OS_OPENBSD OR ZA_OS_NETBSD)
    if(ZA_USE_DRM)
        find_package(DRM REQUIRED)
        find_package(GBM REQUIRED)
        target_link_libraries(zancle-window PRIVATE DRM::DRM GBM::GBM)
    endif()
endif()


# SDL3
target_link_libraries(zancle-window PUBLIC Zancle::System PRIVATE $<BUILD_INTERFACE:SDL3::SDL3> Zancle::GLUtils)


target_link_libraries(zancle-window PRIVATE $<BUILD_INTERFACE:zancle-glad-headers>)

# When static linking on macOS, we need to add this flag for objective C to work
# https://developer.apple.com/library/archive/qa/qa1490/_index.html
if((NOT BUILD_SHARED_LIBS) AND ZA_OS_MACOS)
    target_link_libraries(zancle-window PRIVATE -ObjC)
endif()

# Vulkan headers
target_include_directories(zancle-window SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/extlibs/headers/vulkan")

# GL backend comes from cmake/PlatformLibs.cmake.
target_link_libraries(zancle-window PRIVATE ${ZA_GL_BACKEND_LIBS})

if(ZA_OS_WINDOWS AND NOT ZA_COMPILER_MSVC)
    include(CheckIncludeFileCXX)
    check_include_file_cxx(dinput.h DINPUT_H_FOUND)
    if(NOT DINPUT_H_FOUND)
        target_include_directories(zancle-window SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/extlibs/headers/mingw")
    endif()
endif()

if(ZA_OS_LINUX)
    find_package(UDev REQUIRED)
    target_link_libraries(zancle-window PRIVATE UDev::UDev dl)
elseif(ZA_OS_WINDOWS)
    target_link_libraries(zancle-window PRIVATE ${ZA_WINDOWS_GL_LIBS})
elseif(ZA_OS_FREEBSD)
    target_link_libraries(zancle-window PRIVATE usbhid)
elseif(ZA_OS_MACOS)
    target_link_libraries(zancle-window PRIVATE ${ZA_APPLE_FRAMEWORK_LIBS})
elseif(ZA_OS_IOS)
    target_link_libraries(zancle-window PUBLIC ${ZA_APPLE_FRAMEWORK_LIBS})
elseif(ZA_OS_ANDROID)
    target_link_libraries(zancle-window PRIVATE android)
endif()

# on some platforms (e.g. Raspberry Pi 3 armhf), GCC requires linking libatomic to use <atomic> features
# that aren't supported by native CPU instructions (64-bit atomic operations on 32-bit architecture)
if(ZA_COMPILER_GCC)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("#include <atomic>
        int main(){std::atomic_ullong x(1); return x.fetch_add(1);}" ATOMIC_TEST)
    if(NOT ATOMIC_TEST)
        target_link_libraries(zancle-window PRIVATE atomic)
    endif()
endif()
