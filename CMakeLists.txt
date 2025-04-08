
cmake_minimum_required(VERSION 3.15)
project(GenX)

set(CMAKE_CXX_STANDARD 17)

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

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ThirdParty/glm)

# Add main projects
add_subdirectory(Engine)
add_subdirectory(Editor)

# Set default startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)
