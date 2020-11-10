#pragma once

#include "Graphics/Context.h"

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

   static vk::VertexInputBindingDescription getBindingDescription();
   static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
};

class Mesh
{
public:
   Mesh(const VulkanContext& context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

   Mesh(const Mesh& other) = delete;
   Mesh(Mesh&& other);

   ~Mesh();

   Mesh& operator=(const Mesh& other) = delete;
   Mesh& operator=(Mesh&& other);

private:
   void move(Mesh&& other);
   void release();

public:
   void bindBuffers(vk::CommandBuffer commandBuffer);
   void draw(vk::CommandBuffer commandBuffer);

private:
   vk::Device device;

   vk::Buffer buffer;
   vk::DeviceMemory deviceMemory;
   vk::DeviceSize indexOffset = 0;
   uint32_t numIndices = 0;
};
