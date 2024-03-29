#pragma once

#include "Graphics/GraphicsResource.h"

#include "Math/Bounds.h"

#include "Resources/ResourceTypes.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <span>
#include <vector>

struct Vertex
{
   glm::vec3 position;
   glm::vec3 normal;
   glm::vec3 tangent;
   glm::vec3 bitangent;
   glm::vec4 color;
   glm::vec2 texCoord;

   static const std::vector<vk::VertexInputBindingDescription>& getBindingDescriptions(bool positionOnly);
   static const std::vector<vk::VertexInputAttributeDescription>& getAttributeDescriptions(bool positionOnly);
};

struct MeshSectionSourceData
{
   std::vector<Vertex> vertices;
   std::vector<uint32_t> indices;
   bool hasValidTexCoords = false;
   Bounds bounds;
   StrongMaterialHandle materialHandle;
};

struct MeshSection
{
   vk::DeviceSize vertexOffset = 0;
   vk::DeviceSize positionOnlyVertexOffset = 0;
   vk::DeviceSize indexOffset = 0;
   uint32_t numIndices = 0;
   bool hasValidTexCoords = false;
   Bounds bounds;
   StrongMaterialHandle materialHandle;
};

class Mesh : public GraphicsResource
{
public:
   Mesh(const GraphicsContext& graphicsContext, std::span<const MeshSectionSourceData> sourceData);
   ~Mesh();

   uint32_t getNumSections() const
   {
      return static_cast<uint32_t>(sections.size());
   }

   MeshSection& getSection(uint32_t index)
   {
      return sections[index];
   }

   const MeshSection& getSection(uint32_t index) const
   {
      return sections[index];
   }

   uint32_t getMaterialTypeMask() const
   {
      return materialTypeMask;
   }

   void bindBuffers(vk::CommandBuffer commandBuffer, uint32_t section, bool positionOnly) const;
   void draw(vk::CommandBuffer commandBuffer, uint32_t section) const;

private:
   vk::Buffer buffer;
   VmaAllocation bufferAllocation = nullptr;

   std::vector<MeshSection> sections;
   uint32_t materialTypeMask = 0;
};
