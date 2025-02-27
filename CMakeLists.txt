cmake_minimum_required(VERSION 3.16)
project(Forge VERSION 0.0.0 LANGUAGES CXX C)

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   set(FORGE_PLATFORM_DEFAULT_RHI "Vulkan") # TODO D3D12
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   set(FORGE_PLATFORM_DEFAULT_RHI "Vulkan") # TODO Metal
else()
   set(FORGE_PLATFORM_DEFAULT_RHI "Vulkan")
endif()

option(FORGE_WITH_MIDI "Enable MIDI" ON)
option(FORGE_FORCE_ENABLE_DEBUG_UTILS "Force enable debug utils" OFF)
option(FORGE_FORCE_DISABLE_DEBUG_UTILS "Force disable debug utils" OFF)
option(FORGE_WITH_STATIC_RHI "Compile using a static RHI (will only support the default RHI)" OFF)

set(FORGE_DEFAULT_RHI ${FORGE_PLATFORM_DEFAULT_RHI} CACHE STRING "Default RHI")
set(FORGE_DEFAULT_RHI_OPTIONS "Null" "OpenGL" "Vulkan" "D3D12" "Metal")
set_property(CACHE FORGE_DEFAULT_RHI PROPERTY STRINGS ${FORGE_DEFAULT_RHI_OPTIONS})

add_executable(${PROJECT_NAME} "")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_DEBUG=$<CONFIG:Debug>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_WITH_DEBUG_UTILS=$<AND:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<BOOL:${FORGE_FORCE_ENABLE_DEBUG_UTILS}>>,$<NOT:$<BOOL:${FORGE_FORCE_DISABLE_DEBUG_UTILS}>>>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_PLATFORM_WINDOWS=$<PLATFORM_ID:Windows>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_PLATFORM_MACOS=$<PLATFORM_ID:Darwin>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_PLATFORM_LINUX=$<PLATFORM_ID:Linux>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_PROJECT_NAME="${PROJECT_NAME}")
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_VERSION_MINOR=${PROJECT_VERSION_MINOR})
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_VERSION_PATCH=${PROJECT_VERSION_PATCH})
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_VERSION_TWEAK=${PROJECT_VERSION_TWEAK})
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_WITH_STATIC_RHI=$<BOOL:${FORGE_WITH_STATIC_RHI}>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_WITH_VULKAN=$<OR:$<NOT:$<BOOL:${FORGE_WITH_STATIC_RHI}>>,$<STREQUAL:"${FORGE_DEFAULT_RHI}","Vulkan">>)
target_compile_definitions(${PROJECT_NAME} PUBLIC FORGE_WITH_MIDI=$<BOOL:${FORGE_WITH_MIDI}>)
target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   set_target_properties(${PROJECT_NAME} PROPERTIES DISABLE_PRECOMPILE_HEADERS ON) # Xcode gets angry about the PCH format for some reason, so disable PCH usage on macOS for now
endif()

set(RES_DIR "${PROJECT_SOURCE_DIR}/Resources")
include("${PROJECT_SOURCE_DIR}/Source.cmake")
include("${PROJECT_SOURCE_DIR}/Shaders.cmake")
include("${PROJECT_SOURCE_DIR}/Libraries.cmake")
