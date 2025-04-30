    
    project(Engine)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    
    file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)
    
    add_library(Engine STATIC ${ENGINE_SRC})
    
    target_include_directories(Engine PUBLIC
        src
        ${VULKAN_SDK_INCLUDE}
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(Engine PRIVATE GX_DEBUG)
    endif()