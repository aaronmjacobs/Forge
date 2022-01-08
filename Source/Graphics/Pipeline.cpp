#include "Graphics/Pipeline.h"

#include "Graphics/Mesh.h"
#include "Graphics/GraphicsContext.h"

#include <utility>

PipelineData::PipelineData(const PipelineInfo& info, std::vector<vk::PipelineShaderStageCreateInfo> shaderStages, std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates)
{
   viewport = vk::Viewport()
      .setX(0.0f)
      .setY(0.0f)
      .setWidth(1.0f)
      .setHeight(1.0f)
      .setMinDepth(0.0f)
      .setMaxDepth(1.0f);

   scissor = vk::Rect2D()
      .setExtent(vk::Extent2D(1, 1));

   shaderStageCreateInfo = std::move(shaderStages);
   colorBlendAttachmentStates = std::move(colorBlendStates);

   if (info.passType == PipelinePassType::Mesh)
   {
      vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
         .setVertexBindingDescriptions(Vertex::getBindingDescriptions())
         .setVertexAttributeDescriptions(Vertex::getAttributeDescriptions(info.positionOnly));
   }

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
      .setRasterizationSamples(info.sampleCount)
      .setSampleShadingEnable(true)
      .setMinSampleShading(0.2f);

   depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
      .setDepthTestEnable(info.enableDepthTest)
      .setDepthWriteEnable(info.writeDepth)
      .setDepthCompareOp(info.writeDepth ? vk::CompareOp::eLess : vk::CompareOp::eLessOrEqual)
      .setDepthBoundsTestEnable(false)
      .setStencilTestEnable(false);

   colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
      .setLogicOpEnable(false)
      .setAttachments(colorBlendAttachmentStates);

   dynamicStates = { vk::DynamicState::eScissor, vk::DynamicState::eViewport };
   dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo()
      .setDynamicStates(dynamicStates);

   graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
      .setStages(shaderStageCreateInfo)
      .setPVertexInputState(&vertexInputStateCreateInfo)
      .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
      .setPViewportState(&viewportStateCreateInfo)
      .setPRasterizationState(&rasterizationStateCreateInfo)
      .setPMultisampleState(&multisampleStateCreateInfo)
      .setPDepthStencilState(&depthStencilStateCreateInfo)
      .setPColorBlendState(&colorBlendStateCreateInfo)
      .setPDynamicState(&dynamicStateCreateInfo)
      .setLayout(info.layout)
      .setRenderPass(info.renderPass)
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

void PipelineData::enableDepthBias()
{
   rasterizationStateCreateInfo.setDepthBiasEnable(true);

   dynamicStates.push_back(vk::DynamicState::eDepthBias);
   dynamicStateCreateInfo.setDynamicStates(dynamicStates);
}

void PipelineData::setFrontFace(vk::FrontFace frontFace)
{
   rasterizationStateCreateInfo.setFrontFace(frontFace);
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
      .setPDynamicState(&dynamicStateCreateInfo);
}
