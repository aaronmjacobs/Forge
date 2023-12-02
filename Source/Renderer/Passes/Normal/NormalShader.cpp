#include "Renderer/Passes/Normal/NormalShader.h"

#include "Graphics/SpecializationInfo.h"

#include "Renderer/UniformData.h"

namespace
{
   struct NormalSpecializationValues
   {
      VkBool32 withTextures = false;
      VkBool32 masked = false;

      uint32_t getIndex() const
      {
         return (withTextures << 1) | (masked << 0);
      }
   };

   SpecializationInfo<NormalSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<NormalSpecializationValues> builder;

      builder.registerMember(&NormalSpecializationValues::withTextures);
      builder.registerMember(&NormalSpecializationValues::masked);

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModuleName = "Normal";
      info.fragShaderModuleName = "Normal";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

NormalShader::NormalShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> NormalShader::getStages(bool withTextures, bool masked) const
{
   NormalSpecializationValues specializationValues;
   specializationValues.withTextures = withTextures;
   specializationValues.masked = masked;

   return getStagesForPermutation(specializationValues.getIndex());
}

std::vector<vk::PushConstantRange> NormalShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
