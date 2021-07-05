#include "Graphics/Pipeline.h"

#include "Graphics/Mesh.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Swapchain.h"

#include <utility>

PipelineData::PipelineData(const GraphicsContext& context, vk::PipelineLayout layout, vk::RenderPass renderPass, std::vector<vk::PipelineShaderStageCreateInfo> shaderStages, std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates, vk::SampleCountFlagBits sampleCount)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();

   viewport = vk::Viewport()
      .setX(0.0f)
      .setY(0.0f)
      .setWidth(static_cast<float>(swapchainExtent.width))
      .setHeight(static_cast<float>(swapchainExtent.height))
      .setMinDepth(0.0f)
      .setMaxDepth(1.0f);

   scissor = vk::Rect2D()
      .setExtent(swapchainExtent);

   shaderStageCreateInfo = std::move(shaderStages);
   colorBlendAttachmentStates = std::move(colorBlendStates);

   bool hasColorOutput = !colorBlendAttachmentStates.empty();
   const std::vector<vk::VertexInputBindingDescription>& vertexBindingDescriptions = Vertex::getBindingDescriptions();
   const std::vector<vk::VertexInputAttributeDescription>& vertexAttributeDescriptions = Vertex::getAttributeDescriptions(!hasColorOutput);
   vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
      .setVertexBindingDescriptions(vertexBindingDescriptions)
      .setVertexAttributeDescriptions(vertexAttributeDescriptions);

   inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
      .setTopology(vk::PrimitiveTopology::eTriangleList);

   viewportStateCreateInfo = vk::PipelineViewportStateCreateInfo()
      .setViewports(viewport)
      .setScissors(scissor);

   rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo()
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.0f)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eCounterClockwise);

   multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
      .setRasterizationSamples(sampleCount)
      .setSampleShadingEnable(true)
      .setMinSampleShading(0.2f);

   depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
      .setDepthTestEnable(true)
      .setDepthWriteEnable(!hasColorOutput)
      .setDepthCompareOp(hasColorOutput ? vk::CompareOp::eLessOrEqual : vk::CompareOp::eLess)
      .setDepthBoundsTestEnable(false)
      .setStencilTestEnable(false);

   colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
      .setLogicOpEnable(false)
      .setAttachments(colorBlendAttachmentStates);

   graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
      .setStages(shaderStageCreateInfo)
      .setPVertexInputState(&vertexInputStateCreateInfo)
      .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
      .setPViewportState(&viewportStateCreateInfo)
      .setPRasterizationState(&rasterizationStateCreateInfo)
      .setPMultisampleState(&multisampleStateCreateInfo)
      .setPDepthStencilState(&depthStencilStateCreateInfo)
      .setPColorBlendState(&colorBlendStateCreateInfo)
      .setPDynamicState(nullptr)
      .setLayout(layout)
      .setRenderPass(renderPass)
      .setSubpass(0)
      .setBasePipelineHandle(nullptr)
      .setBasePipelineIndex(-1);
}

PipelineData::PipelineData(const PipelineData& other)
   : PipelineDataBase(other)
{
   updatePointers();
}

PipelineData::PipelineData(PipelineData&& other)
   : PipelineDataBase(std::move(other))
{
   updatePointers();
}

PipelineData& PipelineData::operator=(const PipelineData& other)
{
   PipelineDataBase::operator=(other);
   updatePointers();

   return *this;
}

PipelineData& PipelineData::operator=(PipelineData&& other)
{
   PipelineDataBase::operator=(std::move(other));
   updatePointers();

   return *this;
}

void PipelineData::setShaderStages(std::vector<vk::PipelineShaderStageCreateInfo> shaderStages)
{
   shaderStageCreateInfo = std::move(shaderStages);
   graphicsPipelineCreateInfo.setStages(shaderStageCreateInfo);
}

void PipelineData::setColorBlendAttachmentStates(std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates)
{
   colorBlendAttachmentStates = std::move(colorBlendStates);
   colorBlendStateCreateInfo.setAttachments(colorBlendAttachmentStates);
}

void PipelineData::updatePointers()
{
   colorBlendStateCreateInfo.setAttachments(colorBlendAttachmentStates);

   graphicsPipelineCreateInfo
      .setStages(shaderStageCreateInfo)
      .setPVertexInputState(&vertexInputStateCreateInfo)
      .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
      .setPViewportState(&viewportStateCreateInfo)
      .setPRasterizationState(&rasterizationStateCreateInfo)
      .setPMultisampleState(&multisampleStateCreateInfo)
      .setPDepthStencilState(&depthStencilStateCreateInfo)
      .setPColorBlendState(&colorBlendStateCreateInfo)
      .setPDynamicState(nullptr);
}
