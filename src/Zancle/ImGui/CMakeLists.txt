set(INCROOT ${PROJECT_SOURCE_DIR}/include/Zancle/ImGui)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/Zancle/ImGui)

file(GLOB SRC CONFIGURE_DEPENDS "${INCROOT}/*.hpp" "${INCROOT}/*.inl"  "${SRCROOT}/*.hpp" "${SRCROOT}/*.inl" "${SRCROOT}/*.cpp")

# define the zancle-imgui target
zancle_add_library(ImGui SOURCES ${SRC})

target_link_libraries(zancle-imgui
    PUBLIC  Zancle::Graphics
    PRIVATE $<BUILD_INTERFACE:zancle-glad-headers>
            ${ZA_GL_BACKEND_LIBS})

CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY vittorioromeo/imgui
    GIT_TAG 26485f3ed9c2a1b2bb9288bbc04cba5156844c70
    SYSTEM TRUE
)

file(GLOB imgui_sources "${imgui_SOURCE_DIR}/*.cpp")

add_library(imgui STATIC ${imgui_sources})
add_library(ImGui::ImGui ALIAS imgui)

target_sources(imgui PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS ${imgui_SOURCE_DIR}
    FILES ${imgui_SOURCE_DIR}/imgui.h)

target_include_directories(imgui SYSTEM PUBLIC
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(imgui
    PUBLIC  $<BUILD_INTERFACE:zancle-build-options>
    PRIVATE Zancle::Graphics)

zancle_tame_thirdparty(TARGETS imgui)

# Match the EXPORT name to the active config (Static/Shared) so consumers of
# the Shared zancle-imgui find imgui in the same export set.
if(BUILD_SHARED_LIBS)
    set(_imgui_config_name "Shared")
else()
    set(_imgui_config_name "Static")
endif()

install(TARGETS imgui EXPORT ZancleImGui${_imgui_config_name}Targets FILE_SET public_headers)

target_link_libraries(zancle-imgui PRIVATE ImGui::ImGui)
target_compile_definitions(zancle-imgui PUBLIC IMGUI_USER_CONFIG="${PROJECT_SOURCE_DIR}/include/Zancle/ImGui/ImConfigZancle.hpp")
