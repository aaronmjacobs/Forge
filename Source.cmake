set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")

target_sources(${PROJECT_NAME} PRIVATE
   "${SRC_DIR}/ForgeApplication.cpp"
   "${SRC_DIR}/ForgeApplication.h"
   "${SRC_DIR}/Main.cpp"
   "${SRC_DIR}/PCH.h"

   "${SRC_DIR}/Core/Assert.h"
   "${SRC_DIR}/Core/Containers/FrameVector.h"
   "${SRC_DIR}/Core/Containers/GenerationalArray.h"
   "${SRC_DIR}/Core/Containers/GenerationalArrayHandle.h"
   "${SRC_DIR}/Core/Containers/ReflectedMap.h"
   "${SRC_DIR}/Core/Containers/StaticVector.h"
   "${SRC_DIR}/Core/Delegate.h"
   "${SRC_DIR}/Core/DelegateHandle.cpp"
   "${SRC_DIR}/Core/DelegateHandle.h"
   "${SRC_DIR}/Core/Enum.h"
   "${SRC_DIR}/Core/Features.h"
   "${SRC_DIR}/Core/Hash.h"
   "${SRC_DIR}/Core/Log.cpp"
   "${SRC_DIR}/Core/Log.h"
   "${SRC_DIR}/Core/Macros.h"
   "${SRC_DIR}/Core/Memory/FrameAllocator.cpp"
   "${SRC_DIR}/Core/Memory/FrameAllocator.h"
   "${SRC_DIR}/Core/Types.h"

   "${SRC_DIR}/Graphics/BlendMode.h"
   "${SRC_DIR}/Graphics/Buffer.cpp"
   "${SRC_DIR}/Graphics/Buffer.h"
   "${SRC_DIR}/Graphics/Command.cpp"
   "${SRC_DIR}/Graphics/Command.h"
   "${SRC_DIR}/Graphics/DebugUtils.cpp"
   "${SRC_DIR}/Graphics/DebugUtils.h"
   "${SRC_DIR}/Graphics/DelayedObjectDestroyer.cpp"
   "${SRC_DIR}/Graphics/DelayedObjectDestroyer.h"
   "${SRC_DIR}/Graphics/DescriptorSet.cpp"
   "${SRC_DIR}/Graphics/DescriptorSet.h"
   "${SRC_DIR}/Graphics/DescriptorSetLayoutCache.cpp"
   "${SRC_DIR}/Graphics/DescriptorSetLayoutCache.h"
   "${SRC_DIR}/Graphics/DynamicDescriptorPool.cpp"
   "${SRC_DIR}/Graphics/DynamicDescriptorPool.h"
   "${SRC_DIR}/Graphics/FrameData.h"
   "${SRC_DIR}/Graphics/GraphicsContext.cpp"
   "${SRC_DIR}/Graphics/GraphicsContext.h"
   "${SRC_DIR}/Graphics/GraphicsResource.cpp"
   "${SRC_DIR}/Graphics/GraphicsResource.h"
   "${SRC_DIR}/Graphics/Material.cpp"
   "${SRC_DIR}/Graphics/Material.h"
   "${SRC_DIR}/Graphics/Memory.cpp"
   "${SRC_DIR}/Graphics/Memory.h"
   "${SRC_DIR}/Graphics/Mesh.cpp"
   "${SRC_DIR}/Graphics/Mesh.h"
   "${SRC_DIR}/Graphics/Pipeline.cpp"
   "${SRC_DIR}/Graphics/Pipeline.h"
   "${SRC_DIR}/Graphics/RenderPass.cpp"
   "${SRC_DIR}/Graphics/RenderPass.h"
   "${SRC_DIR}/Graphics/Shader.cpp"
   "${SRC_DIR}/Graphics/Shader.h"
   "${SRC_DIR}/Graphics/ShaderModule.cpp"
   "${SRC_DIR}/Graphics/ShaderModule.h"
   "${SRC_DIR}/Graphics/SpecializationInfo.h"
   "${SRC_DIR}/Graphics/Swapchain.cpp"
   "${SRC_DIR}/Graphics/Swapchain.h"
   "${SRC_DIR}/Graphics/Texture.cpp"
   "${SRC_DIR}/Graphics/Texture.h"
   "${SRC_DIR}/Graphics/TextureInfo.cpp"
   "${SRC_DIR}/Graphics/TextureInfo.h"
   "${SRC_DIR}/Graphics/UniformBuffer.h"
   "${SRC_DIR}/Graphics/Vulkan.cpp"
   "${SRC_DIR}/Graphics/Vulkan.h"

   "${SRC_DIR}/Math/Bounds.cpp"
   "${SRC_DIR}/Math/Bounds.h"
   "${SRC_DIR}/Math/MathUtils.h"
   "${SRC_DIR}/Math/Transform.cpp"
   "${SRC_DIR}/Math/Transform.h"

   "${SRC_DIR}/Platform/InputManager.cpp"
   "${SRC_DIR}/Platform/InputManager.h"
   "${SRC_DIR}/Platform/InputTypes.h"
   "${SRC_DIR}/Platform/Window.cpp"
   "${SRC_DIR}/Platform/Window.h"

   "${SRC_DIR}/Renderer/ForwardLighting.cpp"
   "${SRC_DIR}/Renderer/ForwardLighting.h"
   "${SRC_DIR}/Renderer/Passes/Composite/CompositePass.cpp"
   "${SRC_DIR}/Renderer/Passes/Composite/CompositePass.h"
   "${SRC_DIR}/Renderer/Passes/Composite/CompositeShader.cpp"
   "${SRC_DIR}/Renderer/Passes/Composite/CompositeShader.h"
   "${SRC_DIR}/Renderer/Passes/Depth/DepthPass.cpp"
   "${SRC_DIR}/Renderer/Passes/Depth/DepthPass.h"
   "${SRC_DIR}/Renderer/Passes/Depth/DepthMaskedShader.cpp"
   "${SRC_DIR}/Renderer/Passes/Depth/DepthMaskedShader.h"
   "${SRC_DIR}/Renderer/Passes/Depth/DepthShader.cpp"
   "${SRC_DIR}/Renderer/Passes/Depth/DepthShader.h"
   "${SRC_DIR}/Renderer/Passes/Forward/ForwardPass.cpp"
   "${SRC_DIR}/Renderer/Passes/Forward/ForwardPass.h"
   "${SRC_DIR}/Renderer/Passes/Forward/ForwardShader.cpp"
   "${SRC_DIR}/Renderer/Passes/Forward/ForwardShader.h"
   "${SRC_DIR}/Renderer/Passes/Forward/SkyboxShader.cpp"
   "${SRC_DIR}/Renderer/Passes/Forward/SkyboxShader.h"
   "${SRC_DIR}/Renderer/Passes/Normal/NormalPass.cpp"
   "${SRC_DIR}/Renderer/Passes/Normal/NormalPass.h"
   "${SRC_DIR}/Renderer/Passes/Normal/NormalShader.cpp"
   "${SRC_DIR}/Renderer/Passes/Normal/NormalShader.h"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.cpp"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Bloom/BloomPass.cpp"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Bloom/BloomPass.h"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.cpp"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Tonemap/TonemapPass.cpp"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Tonemap/TonemapPass.h"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Tonemap/TonemapShader.cpp"
   "${SRC_DIR}/Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"
   "${SRC_DIR}/Renderer/Passes/SceneRenderPass.h"
   "${SRC_DIR}/Renderer/Passes/SSAO/SSAOBlurShader.cpp"
   "${SRC_DIR}/Renderer/Passes/SSAO/SSAOBlurShader.h"
   "${SRC_DIR}/Renderer/Passes/SSAO/SSAOPass.cpp"
   "${SRC_DIR}/Renderer/Passes/SSAO/SSAOPass.h"
   "${SRC_DIR}/Renderer/Passes/SSAO/SSAOShader.cpp"
   "${SRC_DIR}/Renderer/Passes/SSAO/SSAOShader.h"
   "${SRC_DIR}/Renderer/Passes/UI/UIPass.cpp"
   "${SRC_DIR}/Renderer/Passes/UI/UIPass.h"
   "${SRC_DIR}/Renderer/PhysicallyBasedMaterial.cpp"
   "${SRC_DIR}/Renderer/PhysicallyBasedMaterial.h"
   "${SRC_DIR}/Renderer/Renderer.cpp"
   "${SRC_DIR}/Renderer/Renderer.h"
   "${SRC_DIR}/Renderer/RenderSettings.cpp"
   "${SRC_DIR}/Renderer/RenderSettings.h"
   "${SRC_DIR}/Renderer/SceneRenderInfo.h"
   "${SRC_DIR}/Renderer/UniformData.h"
   "${SRC_DIR}/Renderer/View.cpp"
   "${SRC_DIR}/Renderer/View.h"
   "${SRC_DIR}/Renderer/ViewInfo.cpp"
   "${SRC_DIR}/Renderer/ViewInfo.h"

   "${SRC_DIR}/Resources/DDSImage.cpp"
   "${SRC_DIR}/Resources/DDSImage.h"
   "${SRC_DIR}/Resources/ForEachResourceType.inl"
   "${SRC_DIR}/Resources/Image.h"
   "${SRC_DIR}/Resources/MaterialLoader.cpp"
   "${SRC_DIR}/Resources/MaterialLoader.h"
   "${SRC_DIR}/Resources/MeshLoader.cpp"
   "${SRC_DIR}/Resources/MeshLoader.h"
   "${SRC_DIR}/Resources/ResourceContainer.h"
   "${SRC_DIR}/Resources/ResourceLoader.cpp"
   "${SRC_DIR}/Resources/ResourceLoader.h"
   "${SRC_DIR}/Resources/ResourceManager.cpp"
   "${SRC_DIR}/Resources/ResourceManager.h"
   "${SRC_DIR}/Resources/ResourceTypes.cpp"
   "${SRC_DIR}/Resources/ResourceTypes.h"
   "${SRC_DIR}/Resources/ShaderModuleLoader.cpp"
   "${SRC_DIR}/Resources/ShaderModuleLoader.h"
   "${SRC_DIR}/Resources/STBImage.cpp"
   "${SRC_DIR}/Resources/STBImage.h"
   "${SRC_DIR}/Resources/TextureLoader.cpp"
   "${SRC_DIR}/Resources/TextureLoader.h"

   "${SRC_DIR}/Scene/Components/CameraComponent.h"
   "${SRC_DIR}/Scene/Components/LightComponent.h"
   "${SRC_DIR}/Scene/Components/MeshComponent.h"
   "${SRC_DIR}/Scene/Components/NameComponent.h"
   "${SRC_DIR}/Scene/Components/OscillatingMovementComponent.h"
   "${SRC_DIR}/Scene/Components/SkyboxComponent.h"
   "${SRC_DIR}/Scene/Components/TransformComponent.cpp"
   "${SRC_DIR}/Scene/Components/TransformComponent.h"
   "${SRC_DIR}/Scene/Entity.cpp"
   "${SRC_DIR}/Scene/Entity.h"
   "${SRC_DIR}/Scene/Scene.cpp"
   "${SRC_DIR}/Scene/Scene.h"
   "${SRC_DIR}/Scene/System.h"
   "${SRC_DIR}/Scene/Systems/CameraSystem.cpp"
   "${SRC_DIR}/Scene/Systems/CameraSystem.h"
   "${SRC_DIR}/Scene/Systems/OscillatingMovementSystem.cpp"
   "${SRC_DIR}/Scene/Systems/OscillatingMovementSystem.h"

   "${SRC_DIR}/UI/UI.cpp"
   "${SRC_DIR}/UI/UI.h"
   "${SRC_DIR}/UI/UIConfig.h"
)

if(FORGE_WITH_MIDI)
   target_sources(${PROJECT_NAME} PRIVATE
      "${SRC_DIR}/Platform/Midi.cpp"
      "${SRC_DIR}/Platform/Midi.h"
   )
endif(FORGE_WITH_MIDI)

target_include_directories(${PROJECT_NAME} PUBLIC "${SRC_DIR}")

get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
source_group(TREE "${SRC_DIR}" PREFIX Source FILES ${SOURCE_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC "${SRC_DIR}/PCH.h")
