set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")

target_sources(${PROJECT_NAME} PRIVATE
   "${SRC_DIR}/ForgeApplication.cpp"
   "${SRC_DIR}/ForgeApplication.h"
   "${SRC_DIR}/Main.cpp"

   "${SRC_DIR}/Core/Assert.h"
   "${SRC_DIR}/Core/Log.cpp"
   "${SRC_DIR}/Core/Log.h"
   "${SRC_DIR}/Core/Containers/GenerationalArray.h"
   "${SRC_DIR}/Core/Containers/ReflectedMap.h"

   "${SRC_DIR}/Graphics/Buffer.cpp"
   "${SRC_DIR}/Graphics/Buffer.h"
   "${SRC_DIR}/Graphics/Command.cpp"
   "${SRC_DIR}/Graphics/Command.h"
   "${SRC_DIR}/Graphics/GraphicsContext.cpp"
   "${SRC_DIR}/Graphics/GraphicsContext.h"
   "${SRC_DIR}/Graphics/GraphicsResource.h"
   "${SRC_DIR}/Graphics/Memory.cpp"
   "${SRC_DIR}/Graphics/Memory.h"
   "${SRC_DIR}/Graphics/Mesh.cpp"
   "${SRC_DIR}/Graphics/Mesh.h"
   "${SRC_DIR}/Graphics/ShaderModule.cpp"
   "${SRC_DIR}/Graphics/ShaderModule.h"
   "${SRC_DIR}/Graphics/Swapchain.cpp"
   "${SRC_DIR}/Graphics/Swapchain.h"
   "${SRC_DIR}/Graphics/Texture.cpp"
   "${SRC_DIR}/Graphics/Texture.h"
   "${SRC_DIR}/Graphics/UniformBuffer.h"
   "${SRC_DIR}/Graphics/UniformData.h"
   "${SRC_DIR}/Graphics/Vulkan.h"
   "${SRC_DIR}/Graphics/Shaders/SimpleShader.cpp"
   "${SRC_DIR}/Graphics/Shaders/SimpleShader.h"

   "${SRC_DIR}/Resources/LoadedImage.cpp"
   "${SRC_DIR}/Resources/LoadedImage.h"
   "${SRC_DIR}/Resources/MeshResourceManager.cpp"
   "${SRC_DIR}/Resources/MeshResourceManager.h"
   "${SRC_DIR}/Resources/ResourceManagerBase.cpp"
   "${SRC_DIR}/Resources/ResourceManagerBase.h"
   "${SRC_DIR}/Resources/ShaderModuleResourceManager.cpp"
   "${SRC_DIR}/Resources/ShaderModuleResourceManager.h"
   "${SRC_DIR}/Resources/TextureResourceManager.cpp"
   "${SRC_DIR}/Resources/TextureResourceManager.h"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${SRC_DIR}")

get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
source_group(TREE "${SRC_DIR}" PREFIX Source FILES ${SOURCE_FILES})
