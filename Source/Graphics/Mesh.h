#pragma once

#include "Graphics/GraphicsResource.h"

#include <glm/glm.hpp>

#include <vector>

struct Vertex
{
   glm::vec3 position;
   glm::vec3 color;
   glm::vec2 texCoord;

   Vertex(const glm::vec3& initialPosition = glm::vec3(0.0f), const glm::vec3& initialColor = glm::vec3(0.0f), const glm::vec2& initialTexCoord = glm::vec2(0.0f))
      : position(initialPosition)
      , color(initialColor)
      , texCoord(initialTexCoord)
   {
   }

   static std::array<vk::VertexInputBindingDescription, 1> getBindingDescriptions();
   static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct MeshSectionSourceData
{
   std::vector<Vertex> vertices;
   std::vector<uint32_t> indices;
};

struct MeshSection
{
   vk::DeviceSize indexOffset = 0;
   uint32_t numIndices = 0;
};

class Mesh : public GraphicsResource
{
public:
   Mesh(const GraphicsContext& graphicsContext, const std::vector<MeshSectionSourceData>& sourceData);

   ~Mesh();

   uint32_t getNumSections() const
   {
      return static_cast<uint32_t>(sections.size());
   }

   void bindBuffers(vk::CommandBuffer commandBuffer, uint32_t section) const;
   void draw(vk::CommandBuffer commandBuffer, uint32_t section) const;

private:
   vk::Buffer buffer;
   vk::DeviceMemory deviceMemory;

   std::vector<MeshSection> sections;
};
