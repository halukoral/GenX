
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

# Add third-party libraries
add_subdirectory(ThirdParty/glfw)

file(GLOB IMGUI_SRC
    ThirdParty/imgui/*.cpp
    ThirdParty/imgui/backends/imgui_impl_glfw.cpp
    ThirdParty/imgui/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SRC})

target_include_directories(imgui PUBLIC
    ThirdParty/imgui
    ThirdParty/imgui/backends
    ThirdParty/glfw/include
    ${VULKAN_SDK_INCLUDE} 
)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ThirdParty/stb_image)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ThirdParty/tinyobjloader)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ThirdParty/glm)

# Add main projects
add_subdirectory(Engine)
add_subdirectory(Editor)

FetchContent_Declare(GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v4.2.0"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(GSL)
target_link_libraries(Engine Microsoft.GSL::GSL)

FetchContent_Declare(spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG "v1.15.2"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(Engine spdlog)

# Set default startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)

# Shaders
file(GLOB_RECURSE ShaderSources CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
)

add_shaders(VulkanEngineShaders ${ShaderSources})
add_dependencies(Engine VulkanEngineShaders)
