
cmake_minimum_required(VERSION 3.15)
project(GenX)

find_package(Vulkan REQUIRED)

include(FetchContent)
include(cmake/Shaders.cmake)

set(CMAKE_CXX_STANDARD 20)

# Build Types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Vulkan SDK path
if(NOT DEFINED VULKAN_SDK AND NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK not set. Use -DVULKAN_SDK=... or set ENV variable.")
endif()

if(NOT DEFINED VULKAN_SDK)
    set(VULKAN_SDK $ENV{VULKAN_SDK})
endif()

set(VULKAN_SDK_INCLUDE "${VULKAN_SDK}/Include")
set(VULKAN_SDK_LIB "${VULKAN_SDK}/Lib")

# Add main projects
add_subdirectory(Engine)
add_subdirectory(Editor)

target_include_directories(Engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/tinyobjloader
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/stb_image
)

# GLFW
FetchContent_Declare(glfw
GIT_REPOSITORY "https://github.com/glfw/glfw.git"
GIT_TAG "3.4"
GIT_SHALLOW ON
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(glm
GIT_REPOSITORY "https://github.com/g-truc/glm.git"
GIT_TAG "1.0.1"
GIT_SHALLOW ON
)
FetchContent_MakeAvailable(glm)

# imgui
FetchContent_Declare(imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "v1.91.9b-docking"
    GIT_SHALLOW OFF
)
FetchContent_MakeAvailable(imgui)

add_library(imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

target_link_libraries(Editor
    PRIVATE
        glm
        imgui
)

target_link_libraries(Engine
    PUBLIC
        glm
    PRIVATE
        glfw
        imgui
)


# Set default startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)

# Shaders
file(GLOB_RECURSE ShaderSources CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/shaders/*.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine/shaders/*.frag"
)

add_shaders(VulkanEngineShaders ${ShaderSources})
add_dependencies(Engine VulkanEngineShaders)
add_dependencies(Editor VulkanEngineShaders)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Engine/assets/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")