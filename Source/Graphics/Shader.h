#pragma once

#include "Core/Delegate.h"
#include "Core/Features.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/ShaderPermutationManager.h"
#include "Graphics/SpecializationInfo.h"

#include "Resources/ResourceTypes.h"

#include <array>
#include <string>
#include <span>
#include <vector>

class ResourceManager;

class Shader : public GraphicsResource
{
public:
   struct ModuleInfo
   {
      ModuleInfo() = default;
      ModuleInfo(const char* vertName_, const char* fragName_)
         : vertName(vertName_)
         , fragName(fragName_)
      {
      }

      std::string vertName;
      std::string fragName;

      std::string vertEntryPoint = "main";
      std::string fragEntryPoint = "main";
   };

   Shader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const ModuleInfo& info, bool delayInitialization = false);
   virtual ~Shader();

   using InitializeDelegate = MulticastDelegate<void>;

   DelegateHandle addOnInitialize(InitializeDelegate::FuncType&& function);
   void removeOnInitialize(DelegateHandle& handle);

protected:
   void setSpecializationInfo(std::span<const vk::SpecializationInfo> specializations);
   std::vector<vk::PipelineShaderStageCreateInfo> getStagesForPermutation(uint32_t permutationIndex) const;

private:
   void initializeStageCreateInfo();

   InitializeDelegate onInitialize;

   ModuleInfo moduleInfo;
   std::span<const vk::SpecializationInfo> specializationInfo;

   StrongShaderModuleHandle vertShaderModuleHandle;
   StrongShaderModuleHandle fragShaderModuleHandle;

   std::vector<vk::PipelineShaderStageCreateInfo> vertStageCreateInfo;
   std::vector<vk::PipelineShaderStageCreateInfo> fragStageCreateInfo;

#if FORGE_WITH_SHADER_HOT_RELOADING
   DelegateHandle hotReloadDelegateHandle;
#endif // FORGE_WITH_SHADER_HOT_RELOADING
};

template<typename ConstantsType, typename... DescriptorTypes>
class ParameterizedShader : public Shader
{
public:
   static constexpr bool kHasConstants = !std::is_void_v<ConstantsType>;
   static constexpr std::size_t kNumDescriptorSets = sizeof...(DescriptorTypes);

   ParameterizedShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const ModuleInfo& info)
      : Shader(graphicsContext, resourceManager, info, kHasConstants)
   {
      if constexpr (kHasConstants)
      {
         ConstantsType::registerMembers(permutationManager);
         specializationInfo = permutationManager.buildSpecializationInfo();

         setSpecializationInfo(specializationInfo.getInfo());
      }
   }

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const requires !kHasConstants
   {
      return getStagesForPermutation(0);
   }

   template<typename C = ConstantsType>
   std::vector<vk::PipelineShaderStageCreateInfo> getStages(const C& shaderConstants) const requires (std::is_same_v<C, ConstantsType>&& kHasConstants)
   {
      return getStagesForPermutation(permutationManager.getIndex(shaderConstants));
   }

   std::array<vk::DescriptorSetLayout, kNumDescriptorSets> getDescriptorSetLayouts() const
   {
      return { context.getDescriptorSetLayout(TypedDescriptorSet<DescriptorTypes>::getCreateInfo())... };
   }

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorTypes&... descriptors)
   {
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptors.getCurrentSet()... }, {});
   }

private:
   ShaderPermutationManager<ConstantsType> permutationManager;
   SpecializationInfo<ConstantsType> specializationInfo;
};
