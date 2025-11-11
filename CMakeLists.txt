cmake_minimum_required(VERSION 3.10)

project (GenX)

set(PROJECT_SOURCE_FILES
  source/main.cpp
)
# Add the application
include_directories(source)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

# Add engine library
add_subdirectory(engine "${CMAKE_CURRENT_BINARY_DIR}/engine_build")
target_link_libraries(${PROJECT_NAME} 
    Engine
)