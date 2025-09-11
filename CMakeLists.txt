cmake_minimum_required(VERSION 3.30)

project(sdlcube LANGUAGES CXX)

add_library(cxx_setup INTERFACE)
target_compile_features(cxx_setup INTERFACE cxx_std_20)
target_include_directories(cxx_setup INTERFACE ${PROJECT_SOURCE_DIR})
target_compile_options(cxx_setup INTERFACE -Wall -Wpedantic -Wextra)

find_package(SDL3 REQUIRED)
find_package(glm REQUIRED)

add_executable(${PROJECT_NAME})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3 glm::glm cxx_setup)
