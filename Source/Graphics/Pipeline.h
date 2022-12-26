#pragma once

#include "Graphics/GraphicsResource.h"

#include <span>
#include <vector>

struct AttachmentFormats;

enum class PipelinePassType
{
   Mesh,
   Screen
};

struct PipelineInfo
{
   PipelinePassType passType = PipelinePassType::Mesh;

   bool enableDepthTest = false;
   bool writeDepth = false;
   bool enableDepthBias = false;

   bool positionOnly = false;
   bool twoSided = false;
   bool swapFrontFace = false;
};

struct PipelineData
{
   vk::PipelineLayout layout;
   vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

   vk::Format depthStencilFormat = vk::Format::eUndefined;
   std::span<const vk::Format> colorFormats;

   std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
   std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates;

   PipelineData() = default;
   PipelineData(const AttachmentFormats& attachmentFormats);
};

class Pipeline : public GraphicsResource
{
public:
   Pipeline(const GraphicsContext& graphicsContext, const PipelineInfo& pipelineInfo, const PipelineData& pipelineData);
   Pipeline(Pipeline&& other);
   ~Pipeline();

   const PipelineInfo& getInfo() const
   {
      return info;
   }

   vk::Pipeline getVkPipeline() const
   {
      return pipeline;
   }

   vk::PipelineLayout getLayout() const
   {
      return layout;
   }

private:
   const PipelineInfo info;
   vk::Pipeline pipeline;
   vk::PipelineLayout layout;
};
