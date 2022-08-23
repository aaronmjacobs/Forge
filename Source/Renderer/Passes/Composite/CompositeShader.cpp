#include "Renderer/Passes/Composite/CompositeShader.h"

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"
#include "Graphics/SpecializationInfo.h"

namespace
{
   struct CompositeSpecializationValues
   {
      CompositeShader::Mode mode = CompositeShader::Mode::Passthrough;

      uint32_t getIndex() const
      {
         return static_cast<uint32_t>(mode);
      }
   };

   SpecializationInfo<CompositeSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<CompositeSpecializationValues> builder;

      builder.registerMember(&CompositeSpecializationValues::mode);

      for (int i = 0; i < CompositeShader::kNumModes; ++i)
      {
         builder.addPermutation(CompositeSpecializationValues{ static_cast<CompositeShader::Mode>(i) });
      }

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Composite.frag.spv";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 1> CompositeShader::getBindings()
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

// static
const vk::DescriptorSetLayoutCreateInfo& CompositeShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<CompositeShader>();
}

// static
vk::DescriptorSetLayout CompositeShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<CompositeShader>(context);
}

CompositeShader::CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void CompositeShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> CompositeShader::getStages(Mode mode) const
{
   CompositeSpecializationValues specializationValues;
   specializationValues.mode = mode;

   return getStagesForPermutation(specializationValues.getIndex());
}

std::vector<vk::DescriptorSetLayout> CompositeShader::getSetLayouts() const
{
   return { getLayout(context) };
}
