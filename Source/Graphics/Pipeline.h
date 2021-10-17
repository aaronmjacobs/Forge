#pragma once

#include "Graphics/Vulkan.h"

#include <vector>

class GraphicsContext;

enum class PipelinePassType
{
   Mesh,
   Screen
};

class PipelineDataBase
{
protected:
   vk::Viewport viewport;
   vk::Rect2D scissor;

   std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfo;
   std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates;
   std::vector<vk::DynamicState> dynamicStates;

   vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
   vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
   vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
   vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
   vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
   vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
   vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
   vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;

   vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
};

class PipelineData : public PipelineDataBase
{
public:
   PipelineData(const GraphicsContext& context, vk::PipelineLayout layout, vk::RenderPass renderPass, PipelinePassType passType, std::vector<vk::PipelineShaderStageCreateInfo> shaderStages, std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates, vk::SampleCountFlagBits sampleCount, bool writeDepth);
   PipelineData(const PipelineData& other);
   PipelineData(PipelineData&& other);

   ~PipelineData() = default;

   PipelineData& operator=(const PipelineData& other);
   PipelineData& operator=(PipelineData&& other);

   const vk::GraphicsPipelineCreateInfo& getCreateInfo() const
   {
      return graphicsPipelineCreateInfo;
   }

   void setShaderStages(std::vector<vk::PipelineShaderStageCreateInfo> shaderStages);
   void setColorBlendAttachmentStates(std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates);

   void enableDepthBias();
   void setFrontFace(vk::FrontFace frontFace);

private:
   void updatePointers();
};
