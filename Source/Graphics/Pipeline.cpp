#include "Graphics/Pipeline.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/GraphicsContext.h"

#include <utility>

namespace
{
   vk::Pipeline createPipeline(const GraphicsContext& context, const PipelineInfo& info, const PipelineData& data)
   {
      vk::Viewport viewport = vk::Viewport()
         .setX(0.0f)
         .setY(0.0f)
         .setWidth(1.0f)
         .setHeight(1.0f)
         .setMinDepth(0.0f)
         .setMaxDepth(1.0f);

      vk::Rect2D scissor = vk::Rect2D()
         .setExtent(vk::Extent2D(1, 1));

      vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
      if (info.passType == PipelinePassType::Mesh)
      {
         vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
            .setVertexBindingDescriptions(Vertex::getBindingDescriptions(info.positionOnly))
            .setVertexAttributeDescriptions(Vertex::getAttributeDescriptions(info.positionOnly));
      }

      vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
         .setTopology(vk::PrimitiveTopology::eTriangleList);

      vk::PipelineViewportStateCreateInfo viewportStateCreateInfo = vk::PipelineViewportStateCreateInfo()
         .setViewports(viewport)
         .setScissors(scissor);

      vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo()
         .setPolygonMode(vk::PolygonMode::eFill)
         .setLineWidth(1.0f)
         .setCullMode(info.twoSided ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack)
         .setFrontFace(info.swapFrontFace ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise)
         .setDepthBiasEnable(info.enableDepthBias);

      vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
         .setRasterizationSamples(data.sampleCount)
         .setSampleShadingEnable(true)
         .setMinSampleShading(0.2f);

      vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
         .setDepthTestEnable(info.enableDepthTest)
         .setDepthWriteEnable(info.writeDepth)
         .setDepthCompareOp(info.writeDepth ? vk::CompareOp::eLess : vk::CompareOp::eLessOrEqual)
         .setDepthBoundsTestEnable(false)
         .setStencilTestEnable(false);

      vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
         .setLogicOpEnable(false)
         .setAttachments(data.colorBlendStates);

      std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eScissor, vk::DynamicState::eViewport };
      if (info.enableDepthBias)
      {
         dynamicStates.push_back(vk::DynamicState::eDepthBias);
      }
      vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo()
         .setDynamicStates(dynamicStates);

      vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
         .setStages(data.shaderStages)
         .setPVertexInputState(&vertexInputStateCreateInfo)
         .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
         .setPViewportState(&viewportStateCreateInfo)
         .setPRasterizationState(&rasterizationStateCreateInfo)
         .setPMultisampleState(&multisampleStateCreateInfo)
         .setPDepthStencilState(&depthStencilStateCreateInfo)
         .setPColorBlendState(&colorBlendStateCreateInfo)
         .setPDynamicState(&dynamicStateCreateInfo)
         .setLayout(data.layout)
         .setRenderPass(data.renderPass)
         .setSubpass(0)
         .setBasePipelineHandle(nullptr)
         .setBasePipelineIndex(-1);

      return context.getDevice().createGraphicsPipeline(context.getPipelineCache(), graphicsPipelineCreateInfo).value;
   }
}

Pipeline::Pipeline(const GraphicsContext& graphicsContext, const PipelineInfo& pipelineInfo, const PipelineData& pipelineData)
   : GraphicsResource(graphicsContext)
   , info(pipelineInfo)
   , pipeline(createPipeline(graphicsContext, pipelineInfo, pipelineData))
   , layout(pipelineData.layout)
{
   NAME_CHILD(pipeline, "Pipeline");
}

Pipeline::Pipeline(Pipeline&& other)
   : GraphicsResource(std::move(other))
   , info(other.info)
   , pipeline(other.pipeline)
   , layout(other.layout)
{
   other.pipeline = nullptr;
   other.layout = nullptr;
}

Pipeline::~Pipeline()
{
   context.delayedDestroy(std::move(pipeline));
}
