#pragma once

#include "Graphics/DebugUtils.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/RenderPass.h"

#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

#include <unordered_map>
#include <vector>

template<typename PassType>
struct PipelineDescription;

template<typename Derived>
class SceneRenderPass : public RenderPass
{
public:
   SceneRenderPass(const GraphicsContext& graphicsContext)
      : RenderPass(graphicsContext)
   {
   }

   ~SceneRenderPass()
   {
      pipelineMap.clear();
   }

protected:
   void postRenderPassInitialized() override
   {
      pipelineMap.clear();
   }

   template<BlendMode blendMode>
   void renderMeshes(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
   {
      Derived* derivedThis = static_cast<Derived*>(this);

      vk::PipelineLayout pipelineLayout = derivedThis->selectPipelineLayout(blendMode);

      vk::Pipeline lastPipeline;
      for (const MeshRenderInfo& meshRenderInfo : sceneRenderInfo.meshes)
      {
         const std::vector<uint32_t>& sections = blendMode == BlendMode::Translucent ? meshRenderInfo.visibleTranslucentSections : blendMode == BlendMode::Masked ? meshRenderInfo.visibleMaskedSections : meshRenderInfo.visibleOpaqueSections;
         if (!sections.empty())
         {
            ASSERT(meshRenderInfo.mesh);

            SCOPED_LABEL(meshRenderInfo.mesh->getName());

            MeshUniformData meshUniformData;
            meshUniformData.localToWorld = meshRenderInfo.localToWorld;
            commandBuffer.pushConstants<MeshUniformData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, meshUniformData);

            for (uint32_t section : sections)
            {
               SCOPED_LABEL("Section " + DebugUtils::toString(section));

               const Material* material = meshRenderInfo.materials[section];
               ASSERT(material);

               PipelineDescription<Derived> pipelineDescription = derivedThis->getPipelineDescription(sceneRenderInfo.view, meshRenderInfo.mesh->getSection(section), *material);
               const Pipeline& pipeline = getPipeline(pipelineDescription);
               ASSERT(pipeline.getLayout() == pipelineLayout);

               if (pipeline.getVkPipeline() != lastPipeline)
               {
                  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());
                  lastPipeline = pipeline.getVkPipeline();
               }

               derivedThis->renderMesh(commandBuffer, pipeline, sceneRenderInfo.view, *meshRenderInfo.mesh, section, *material);
            }
         }
      }
   }

   void renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
   {
      mesh.bindBuffers(commandBuffer, section, pipeline.getInfo().positionOnly);
      mesh.draw(commandBuffer, section);
   }

   void renderScreenMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline)
   {
      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());
      commandBuffer.draw(3, 1, 0, 0);
   }

   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const
   {
      return nullptr;
   }

   PipelineDescription<Derived> getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const
   {
      return PipelineDescription<Derived>{};
   }

   Pipeline createPipeline(const PipelineDescription<Derived>& description)
   {
      return nullptr;
   }

   const Pipeline& getPipeline(const PipelineDescription<Derived>& description)
   {
      auto location = pipelineMap.find(description);
      if (location == pipelineMap.end())
      {
         location = pipelineMap.emplace(description, static_cast<Derived*>(this)->createPipeline(description)).first;
      }

      return location->second;
   }

private:
   std::unordered_map<PipelineDescription<Derived>, Pipeline> pipelineMap;
};
