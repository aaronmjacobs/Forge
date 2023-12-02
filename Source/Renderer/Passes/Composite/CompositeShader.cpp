#include "Renderer/Passes/Composite/CompositeShader.h"

#include "Core/Enum.h"

#include "Graphics/SpecializationInfo.h"

namespace
{
   struct CompositeSpecializationValues
   {
      CompositeShader::Mode mode = CompositeShader::Mode::Passthrough;

      uint32_t getIndex() const
      {
         return (static_cast<uint32_t>(mode) << 0);
      }
   };

   SpecializationInfo<CompositeSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<CompositeSpecializationValues> builder;

      builder.registerMember(&CompositeSpecializationValues::mode, CompositeShader::Mode::Passthrough, CompositeShader::Mode::SrgbToLinear);

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModuleName = "Screen";
      info.fragShaderModuleName = "Composite";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::vector<vk::DescriptorSetLayoutBinding> CompositeDescriptorSet::getBindings()
{
   return
   {
      vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

CompositeShader::CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> CompositeShader::getStages(Mode mode) const
{
   CompositeSpecializationValues specializationValues;
   specializationValues.mode = mode;

   return getStagesForPermutation(specializationValues.getIndex());
}
