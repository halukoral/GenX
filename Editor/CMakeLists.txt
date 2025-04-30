
project(Editor)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)

add_executable(Editor ${EDITOR_SRC})

target_include_directories(Editor PUBLIC
    src
    ../Engine/src
    ${VULKAN_SDK_INCLUDE}
)

target_link_libraries(Editor PUBLIC Engine )


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Editor PRIVATE GX_DEBUG)
endif()

if(WIN32)
    target_link_directories(Editor PUBLIC ${VULKAN_SDK_LIB})
    target_link_libraries(Editor PUBLIC vulkan-1)
endif()
