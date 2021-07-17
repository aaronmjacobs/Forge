#pragma once

#include "Graphics/DebugUtils.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/RenderPass.h"

#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

#include <vector>

template<typename Derived>
class SceneRenderPass : public RenderPass
{
public:
   SceneRenderPass(const GraphicsContext& graphicsContext)
      : RenderPass(graphicsContext)
   {
   }

protected:
   template<BlendMode blendMode>
   void renderMeshes(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
   {
      vk::Pipeline lastPipeline;
      for (const MeshRenderInfo& meshRenderInfo : sceneRenderInfo.meshes)
      {
         const std::vector<uint32_t>& sections = blendMode == BlendMode::Translucent ? meshRenderInfo.visibleTranslucentSections : meshRenderInfo.visibleOpaqueSections;
         if (!sections.empty())
         {
            ASSERT(meshRenderInfo.mesh);

            SCOPED_LABEL(meshRenderInfo.mesh->getName());

            MeshUniformData meshUniformData;
            meshUniformData.localToWorld = meshRenderInfo.localToWorld;
            commandBuffer.pushConstants<MeshUniformData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, meshUniformData);

            for (uint32_t section : sections)
            {
               SCOPED_LABEL("Section " + std::to_string(section));

               const Material* material = meshRenderInfo.materials[section];
               ASSERT(material);

               vk::Pipeline desiredPipeline = static_cast<Derived*>(this)->selectPipeline(meshRenderInfo.mesh->getSection(section), *material);
               if (desiredPipeline != lastPipeline)
               {
                  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, desiredPipeline);
                  lastPipeline = desiredPipeline;
               }

               static_cast<Derived*>(this)->renderMesh(commandBuffer, sceneRenderInfo.view, *meshRenderInfo.mesh, section, *material);
            }
         }
      }
   }

   void renderMesh(vk::CommandBuffer commandBuffer, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
   {
      mesh.bindBuffers(commandBuffer, section);
      mesh.draw(commandBuffer, section);
   }

   void renderScreenMesh(vk::CommandBuffer commandBuffer, vk::Pipeline pipeline)
   {
      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
      commandBuffer.draw(3, 1, 0, 0);
   }

   vk::Pipeline selectPipeline(const MeshSection& meshSection, const Material& material) const
   {
      return nullptr;
   }
};
