#include "Core/Enum.h"

#include "Graphics/GraphicsResource.h"

#include <array>
#include <vector>

class DescriptorSet;
class ResourceManager;

class CompositeShader : public GraphicsResource
{
public:
   enum class Mode
   {
      Passthrough = 0,
      LinearToSrgb = 1,
      SrgbToLinear = 2
   };

   static constexpr int kNumModes = Enum::cast(Mode::SrgbToLinear) + 1;

   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(Mode mode) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;

private:
   std::array<vk::PipelineShaderStageCreateInfo, 3> vertStageCreateInfo;
   std::array<vk::PipelineShaderStageCreateInfo, 3> fragStageCreateInfo;
};
