#pragma once

#include "Core/Delegate.h"
#include "Core/Features.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceTypes.h"

#include <array>
#include <string>
#include <span>
#include <vector>

class ResourceManager;

class Shader : public GraphicsResource
{
public:
   struct InitializationInfo
   {
      std::string vertShaderModuleName;
      std::string fragShaderModuleName;

      std::string vertShaderModuleEntryPoint = "main";
      std::string fragShaderModuleEntryPoint = "main";

      std::span<const vk::SpecializationInfo> specializationInfo;
   };

   Shader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const InitializationInfo& info);
   virtual ~Shader();

   using InitializeDelegate = MulticastDelegate<void>;

   DelegateHandle addOnInitialize(InitializeDelegate::FuncType&& function);
   void removeOnInitialize(DelegateHandle& handle);

protected:
   std::vector<vk::PipelineShaderStageCreateInfo> getStagesForPermutation(uint32_t permutationIndex) const;

private:
   void initializeStageCreateInfo();

   InitializationInfo initializationInfo;
   InitializeDelegate onInitialize;

   StrongShaderModuleHandle vertShaderModuleHandle;
   StrongShaderModuleHandle fragShaderModuleHandle;

   std::vector<vk::PipelineShaderStageCreateInfo> vertStageCreateInfo;
   std::vector<vk::PipelineShaderStageCreateInfo> fragStageCreateInfo;

#if FORGE_WITH_SHADER_HOT_RELOADING
   DelegateHandle hotReloadDelegateHandle;
#endif // FORGE_WITH_SHADER_HOT_RELOADING
};

template<typename... DescriptorTypes>
class ShaderWithDescriptors : public Shader
{
public:
   static constexpr std::size_t kNumDescriptorSets = sizeof...(DescriptorTypes);

   ShaderWithDescriptors(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const InitializationInfo& info)
      : Shader(graphicsContext, resourceManager, info)
   {
   }

   std::array<vk::DescriptorSetLayout, kNumDescriptorSets> getDescriptorSetLayouts() const
   {
      return { context.getDescriptorSetLayout(TypedDescriptorSet<DescriptorTypes>::getCreateInfo())... };
   }

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorTypes&... descriptors)
   {
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptors.getCurrentSet()... }, {});
   }
};
