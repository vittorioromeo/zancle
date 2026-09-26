# OS-conditional platform-library lists previously inlined into every
# multimedia module's CMakeLists.txt. Each consumer now does a single
#   target_link_libraries(zancle-X PRIVATE ${ZA_GL_BACKEND_LIBS})
# instead of repeating the if/elseif/else chain.
#
# Variables (rather than IMPORTED INTERFACE targets) are used here so the
# install/export footprint is unchanged: the underlying libraries (OpenGL::GL,
# EGL::EGL, ...) appear directly in each module's exported link interface,
# and consumers continue to resolve them via Dependencies.cmake.in's
# `find_dependency(...)` calls.

# -- OpenGL / OpenGL ES backend -----------------------------------------------
#
# Mirrors the historical chain:
#   - iOS                                  -> -framework OpenGLES
#   - Android / Emscripten / ZA_OPENGL_ES  -> EGL::EGL + GLES::GLES
#   - everything else                      -> OpenGL::GL
#
# `find_package` calls cache, so calling them once here is equivalent to (and
# cheaper than) the previous per-module repetition.

if(ZA_OS_IOS)
    set(ZA_GL_BACKEND_LIBS "-framework OpenGLES")
elseif(ZA_OS_ANDROID OR ZA_OS_EMSCRIPTEN OR ZA_OPENGL_ES)
    find_package(EGL REQUIRED)
    find_package(GLES REQUIRED)
    set(ZA_GL_BACKEND_LIBS EGL::EGL GLES::GLES)
else()
    find_package(OpenGL REQUIRED COMPONENTS OpenGL)
    set(ZA_GL_BACKEND_LIBS OpenGL::GL)
endif()

# -- Apple frameworks ----------------------------------------------------------
#
# Foundation/AppKit/IOKit/Carbon on macOS, Foundation/UIKit/CoreGraphics/
# QuartzCore/CoreMotion on iOS. Empty list otherwise so consumers can splice
# unconditionally without an `if(APPLE)` guard.

set(ZA_APPLE_FRAMEWORK_LIBS "")
if(ZA_OS_MACOS)
    set(ZA_APPLE_FRAMEWORK_LIBS
        "-framework Foundation"
        "-framework AppKit"
        "-framework IOKit"
        "-framework Carbon"
    )
elseif(ZA_OS_IOS)
    set(ZA_APPLE_FRAMEWORK_LIBS
        "-framework Foundation"
        "-framework UIKit"
        "-framework CoreGraphics"
        "-framework QuartzCore"
        "-framework CoreMotion"
    )
endif()

# -- Windows system libs needed by OpenGL/windowing code ----------------------
set(ZA_WINDOWS_GL_LIBS "")
if(ZA_OS_WINDOWS)
    set(ZA_WINDOWS_GL_LIBS winmm gdi32)
endif()

# -- Bundled glad headers ------------------------------------------------------
#
# Six modules use the glad GL loader headers from extlibs. PRIVATE link wrapped
# in $<BUILD_INTERFACE:...> so the install/export set is unaffected (consumers
# bundle the headers via FILE_SET HEADERS on each module).
add_library(zancle-glad-headers INTERFACE)

target_include_directories(zancle-glad-headers SYSTEM INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extlibs/headers/glad/include>)

# In shared builds, the GLAD entities are defined in (and exported from) `zancle-glutils`,
# which additionally defines `GLAD_API_CALL_EXPORT_BUILD`
if(BUILD_SHARED_LIBS)
    target_compile_definitions(zancle-glad-headers INTERFACE GLAD_API_CALL_EXPORT)
endif()
