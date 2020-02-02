set(LIB_DIR "${PROJECT_SOURCE_DIR}/Libraries")

# Boxer
add_subdirectory("${LIB_DIR}/Boxer")
target_link_libraries(${PROJECT_NAME} PUBLIC Boxer)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${LIB_DIR}/glfw")
target_compile_definitions(${PROJECT_NAME} PUBLIC GLFW_INCLUDE_NONE)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

# GLM
set(GLM_INSTALL_ENABLE OFF CACHE INTERNAL "GLM install")
set(GLM_TEST_ENABLE OFF CACHE INTERNAL "Build unit tests")
add_subdirectory("${LIB_DIR}/glm")
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_CTOR_INIT)
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_link_libraries(${PROJECT_NAME} PUBLIC glm)

# PPK_ASSERT
set(PPK_DIR "${LIB_DIR}/PPK_ASSERT")
target_sources(${PROJECT_NAME} PRIVATE "${PPK_DIR}/src/ppk_assert.h" "${PPK_DIR}/src/ppk_assert.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC "${PPK_DIR}/src")
source_group("Libraries\\PPK_ASSERT" "${PPK_DIR}/src")

# templog
set(TEMPLOG_DIR "${LIB_DIR}/templog")
target_sources(${PROJECT_NAME} PRIVATE
   "${TEMPLOG_DIR}/config.h"
   "${TEMPLOG_DIR}/logging.h"
   "${TEMPLOG_DIR}/templ_meta.h"
   "${TEMPLOG_DIR}/tuples.h"
   "${TEMPLOG_DIR}/type_lists.h"
   "${TEMPLOG_DIR}/imp/logging.cpp"
)
target_include_directories(${PROJECT_NAME} PUBLIC "${TEMPLOG_DIR}")
source_group("Libraries\\templog" "${TEMPLOG_DIR}")

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

# Vulkan-Hpp
set(VULKAN_HPP_DIR "${LIB_DIR}/Vulkan-Hpp")
target_sources(${PROJECT_NAME} PRIVATE "${VULKAN_HPP_DIR}/vulkan/vulkan.hpp")
target_include_directories(${PROJECT_NAME} PUBLIC "${VULKAN_HPP_DIR}")
source_group("Libraries\\Vulkan-Hpp" "${VULKAN_HPP_DIR}")
