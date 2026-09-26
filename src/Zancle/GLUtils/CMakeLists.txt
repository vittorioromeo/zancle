set(INCROOT ${PROJECT_SOURCE_DIR}/include/Zancle/GLUtils)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/Zancle/GLUtils)

file(GLOB SRC CONFIGURE_DEPENDS "${INCROOT}/*.hpp" "${INCROOT}/*.inl"  "${SRCROOT}/*.hpp" "${SRCROOT}/*.inl" "${SRCROOT}/*.cpp")

# add platform specific sources
if(ZA_OS_WINDOWS)
    if(ZA_OPENGL_ES)
        list(APPEND PLATFORM_SRC
            ${SRCROOT}/EGL/EGLCheck.cpp
            ${SRCROOT}/EGL/EGLCheck.hpp
            ${SRCROOT}/EGL/EGLGlad.cpp
            ${SRCROOT}/EGL/EGLGlad.hpp
        )
    endif()
elseif(ZA_OS_LINUX OR ZA_OS_FREEBSD OR ZA_OS_OPENBSD OR ZA_OS_NETBSD)
    if(ZA_USE_DRM)
        set(PLATFORM_SRC
            ${SRCROOT}/EGL/EGLCheck.cpp
            ${SRCROOT}/EGL/EGLCheck.hpp
        )
    else()
        if(ZA_OPENGL_ES)
            list(APPEND PLATFORM_SRC
                ${SRCROOT}/EGL/EGLCheck.cpp
                ${SRCROOT}/EGL/EGLCheck.hpp
                ${SRCROOT}/EGL/EGLGlad.cpp
                ${SRCROOT}/EGL/EGLGlad.hpp
            )
        endif()
    endif()
elseif(ZA_OS_IOS)
    enable_language(OBJCXX)
elseif(ZA_OS_ANDROID)
    set(PLATFORM_SRC
        ${SRCROOT}/EGL/EGLCheck.cpp
        ${SRCROOT}/EGL/EGLCheck.hpp
        ${SRCROOT}/EGL/EGLContext.cpp
        ${SRCROOT}/EGL/EGLContext.hpp
    )
endif()

# define the zancle-glutils target
zancle_add_library(GLUtils
                 SOURCES ${SRC} ${PLATFORM_SRC}
                 DEPENDENCIES "Dependencies.cmake.in")

# DRM libraries
if(ZA_OS_LINUX OR ZA_OS_FREEBSD OR ZA_OS_OPENBSD OR ZA_OS_NETBSD)
    if(ZA_USE_DRM)
        find_package(DRM REQUIRED)
        find_package(GBM REQUIRED)
        target_link_libraries(zancle-glutils PRIVATE DRM::DRM GBM::GBM)
    endif()
endif()


target_link_libraries(zancle-glutils PUBLIC Zancle::System)
target_link_libraries(zancle-glutils PRIVATE $<BUILD_INTERFACE:zancle-glad-headers>)

# When static linking on macOS, we need to add this flag for objective C to work
# https://developer.apple.com/library/archive/qa/qa1490/_index.html
if((NOT BUILD_SHARED_LIBS) AND ZA_OS_MACOS)
    target_link_libraries(zancle-glutils PRIVATE -ObjC)
endif()

# Vulkan headers
target_include_directories(zancle-glutils SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/extlibs/headers/vulkan")

# GL backend + Apple frameworks come from cmake/PlatformLibs.cmake.
target_link_libraries(zancle-glutils PRIVATE ${ZA_GL_BACKEND_LIBS})

if(ZA_OS_WINDOWS)
    target_link_libraries(zancle-glutils PRIVATE ${ZA_WINDOWS_GL_LIBS})
elseif(ZA_OS_MACOS)
    target_link_libraries(zancle-glutils PRIVATE ${ZA_APPLE_FRAMEWORK_LIBS})
elseif(ZA_OS_IOS)
    target_link_libraries(zancle-glutils PUBLIC ${ZA_APPLE_FRAMEWORK_LIBS})
elseif(ZA_OS_ANDROID)
    target_link_libraries(zancle-glutils PRIVATE android)
endif()

if(ZA_FATAL_OPENGL_ERRORS)
    target_compile_definitions(zancle-glutils PRIVATE "ZA_FATAL_OPENGL_ERRORS")
endif()
