set(LIB_DIR "${PROJECT_SOURCE_DIR}/Libraries")

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build package with shared libraries.")

# Assimp
set(ASSIMP_NO_EXPORT ON CACHE INTERNAL "Disable Assimp's export functionality.")
set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "If the test suite for Assimp is built in addition to the library.")
set(ASSIMP_INSTALL OFF CACHE INTERNAL "Disable this if you want to use assimp as a submodule.")
set(ASSIMP_IGNORE_GIT_HASH ON CACHE INTERNAL "Don't call git to get the hash.")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE INTERNAL "default value of all ASSIMP_BUILD_XXX_IMPORTER values")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE INTERNAL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE INTERNAL "")
add_subdirectory("${LIB_DIR}/assimp")
target_link_libraries(${PROJECT_NAME} PUBLIC assimp)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:assimp>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build package with shared libraries.")

# Boxer
add_subdirectory("${LIB_DIR}/Boxer")
target_link_libraries(${PROJECT_NAME} PUBLIC Boxer)

# Dear ImGui
set(IMGUI_DIR "${LIB_DIR}/imgui")
target_sources(${PROJECT_NAME} PRIVATE
   "${IMGUI_DIR}/imconfig.h"
   "${IMGUI_DIR}/imgui.cpp"
   "${IMGUI_DIR}/imgui.h"
   "${IMGUI_DIR}/imgui_demo.cpp"
   "${IMGUI_DIR}/imgui_draw.cpp"
   "${IMGUI_DIR}/imgui_internal.h"
   "${IMGUI_DIR}/imgui_tables.cpp"
   "${IMGUI_DIR}/imgui_widgets.cpp"
   "${IMGUI_DIR}/imstb_rectpack.h"
   "${IMGUI_DIR}/imstb_textedit.h"
   "${IMGUI_DIR}/imstb_truetype.h"
   "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
   "${IMGUI_DIR}/backends/imgui_impl_glfw.h"
   "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
   "${IMGUI_DIR}/backends/imgui_impl_vulkan.h"
)
target_include_directories(${PROJECT_NAME} PUBLIC "${IMGUI_DIR}")
target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_USER_CONFIG="UI/UIConfig.h")
source_group("Libraries\\imgui\\backends" "${IMGUI_DIR}/backends/*")
source_group("Libraries\\imgui" "${IMGUI_DIR}/*")

# EnTT
add_subdirectory("${LIB_DIR}/entt")
target_link_libraries(${PROJECT_NAME} PUBLIC EnTT)

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
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_RADIANS)
target_link_libraries(${PROJECT_NAME} PUBLIC glm)

# Kontroller
if(FORGE_WITH_MIDI)
   add_subdirectory("${LIB_DIR}/Kontroller")
   target_link_libraries(${PROJECT_NAME} PUBLIC Kontroller)
endif(FORGE_WITH_MIDI)

# VulkanMemoryAllocator
add_subdirectory("${LIB_DIR}/VulkanMemoryAllocator")
target_link_libraries(${PROJECT_NAME} PUBLIC VulkanMemoryAllocator)

# PlatformUtils
if(NOT TARGET PlatformUtils) # Transitively added by Kontroller (which means we're beholden to its version of it)
   add_subdirectory("${LIB_DIR}/PlatformUtils")
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC PlatformUtils)

# PPK_ASSERT
set(PPK_DIR "${LIB_DIR}/PPK_ASSERT")
target_sources(${PROJECT_NAME} PRIVATE "${PPK_DIR}/src/ppk_assert.h" "${PPK_DIR}/src/ppk_assert.cpp")
set_source_files_properties("${PPK_DIR}/src/ppk_assert.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS ON) # Don't use PCH for ppk_assert.cpp to avoid warnings
target_include_directories(${PROJECT_NAME} PUBLIC "${PPK_DIR}/src")
source_group("Libraries\\PPK_ASSERT" "${PPK_DIR}/src")

# stb
set(STB_DIR "${LIB_DIR}/stb")
target_sources(${PROJECT_NAME} PRIVATE "${STB_DIR}/stb_image.h")
target_include_directories(${PROJECT_NAME} PUBLIC "${STB_DIR}")
source_group("Libraries\\stb" "${STB_DIR}")

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
file(WRITE "${PROJECT_BINARY_DIR}/va_stdafx.h" "#define VULKAN_HPP_NAMESPACE vk\n") # Help out Visual Assist with the Vulkan-HPP namespace
