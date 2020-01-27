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

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
