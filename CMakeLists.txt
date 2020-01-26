cmake_minimum_required(VERSION 3.11)
project(Forge VERSION 0.0.0 LANGUAGES CXX)

add_executable(${PROJECT_NAME} "")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_DEBUG=$<CONFIG:Debug>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_PLATFORM_WINDOWS=$<PLATFORM_ID:Windows>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_PLATFORM_MACOS=$<PLATFORM_ID:Darwin>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_PLATFORM_LINUX=$<PLATFORM_ID:Linux>)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")

include("${PROJECT_SOURCE_DIR}/Source.cmake")
