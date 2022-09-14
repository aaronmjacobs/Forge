#include "Graphics/Mesh.h"

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/DebugUtils.h"
#include "Graphics/Memory.h"

#include <array>
#include <limits>
#include <utility>

// static
const std::vector<vk::VertexInputBindingDescription>& Vertex::getBindingDescriptions(bool positionOnly)
{
   if (positionOnly)
   {
      static std::vector<vk::VertexInputBindingDescription> positionOnlyBindingDescriptions =
      {
         vk::VertexInputBindingDescription()
         .setBinding(0)
         .setStride(sizeof(glm::vec3))
         .setInputRate(vk::VertexInputRate::eVertex)
      };

      return positionOnlyBindingDescriptions;
   }
   else
   {
      static std::vector<vk::VertexInputBindingDescription> defaultBindingDescriptions =
      {
         vk::VertexInputBindingDescription()
         .setBinding(0)
         .setStride(sizeof(Vertex))
         .setInputRate(vk::VertexInputRate::eVertex)
      };

      return defaultBindingDescriptions;
   }
}

// static
const std::vector<vk::VertexInputAttributeDescription>& Vertex::getAttributeDescriptions(bool positionOnly)
{
   if (positionOnly)
   {
      static std::vector<vk::VertexInputAttributeDescription> positionOnlyAttributeDescriptions =
      {
         vk::VertexInputAttributeDescription()
            .setLocation(0)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(0)
      };

      return positionOnlyAttributeDescriptions;
   }
   else
   {
      static std::vector<vk::VertexInputAttributeDescription> defaultAttributeDescriptions =
      {
         vk::VertexInputAttributeDescription()
            .setLocation(0)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(offsetof(Vertex, position)),
         vk::VertexInputAttributeDescription()
            .setLocation(1)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(offsetof(Vertex, normal)),
         vk::VertexInputAttributeDescription()
            .setLocation(2)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(offsetof(Vertex, tangent)),
         vk::VertexInputAttributeDescription()
            .setLocation(3)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(offsetof(Vertex, bitangent)),
         vk::VertexInputAttributeDescription()
            .setLocation(4)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32A32Sfloat)
            .setOffset(offsetof(Vertex, color)),
         vk::VertexInputAttributeDescription()
            .setLocation(5)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32Sfloat)
            .setOffset(offsetof(Vertex, texCoord))
      };

      return defaultAttributeDescriptions;
   }
}

Mesh::Mesh(const GraphicsContext& graphicsContext, std::span<const MeshSectionSourceData> sourceData)
   : GraphicsResource(graphicsContext)
{
   vk::DeviceSize bufferSize = 0;
   for (const MeshSectionSourceData& sectionData : sourceData)
   {
      bufferSize += sectionData.vertices.size() * sizeof(Vertex);
      bufferSize += sectionData.vertices.size() * sizeof(glm::vec3);
      bufferSize += sectionData.indices.size() * sizeof(uint32_t);
   }

   // Create the buffer

   vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
      .setSize(bufferSize)
      .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer)
      .setSharingMode(vk::SharingMode::eExclusive);

   VmaAllocationCreateInfo bufferAllocationCreateInfo{};
   bufferAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

   VkBuffer vkBuffer = nullptr;
   VkResult bufferCreateResult = vmaCreateBuffer(context.getVmaAllocator(), &static_cast<VkBufferCreateInfo&>(bufferCreateInfo), &bufferAllocationCreateInfo, &vkBuffer, &bufferAllocation, nullptr);
   if (bufferCreateResult != VK_SUCCESS)
   {
      throw new std::runtime_error("Failed to allocate mesh buffer");
   }
   buffer = vkBuffer;
   NAME_CHILD(buffer, "Mesh Buffer");

   // Create staging buffer, and use it to copy over data

   vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo()
      .setSize(bufferCreateInfo.size)
      .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
      .setSharingMode(vk::SharingMode::eExclusive);

   VmaAllocationCreateInfo stagingBufferAllocationCreateInfo{};
   stagingBufferAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
   stagingBufferAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

   VkBuffer stagingBuffer = nullptr;
   VmaAllocation stagingBufferAllocation = nullptr;
   VmaAllocationInfo stagingBufferAllocationInfo{};
   VkResult stagingBufferCreateResult = vmaCreateBuffer(context.getVmaAllocator(), &static_cast<VkBufferCreateInfo&>(stagingBufferCreateInfo), &stagingBufferAllocationCreateInfo, &stagingBuffer, &stagingBufferAllocation, &stagingBufferAllocationInfo);
   if (stagingBufferCreateResult != VK_SUCCESS)
   {
      throw new std::runtime_error("Failed to allocate mesh staging buffer");
   }

   uint8_t* mappedData = static_cast<uint8_t*>(stagingBufferAllocationInfo.pMappedData);
   std::size_t mappedDataOffset = 0;
   sections.reserve(sourceData.size());
   for (const MeshSectionSourceData& sectionData : sourceData)
   {
      MeshSection meshSection;

      ASSERT(sectionData.indices.size() <= std::numeric_limits<uint32_t>::max());
      meshSection.numIndices = static_cast<uint32_t>(sectionData.indices.size());

      meshSection.hasValidTexCoords = sectionData.hasValidTexCoords;

      std::size_t vertexDataSize = sectionData.vertices.size() * sizeof(Vertex);
      std::size_t positionOnlyVertexDataSize = sectionData.vertices.size() * sizeof(glm::vec3);
      std::size_t indexDataSize = sectionData.indices.size() * sizeof(uint32_t);

      meshSection.vertexOffset = mappedDataOffset;
      std::memcpy(mappedData + mappedDataOffset, sectionData.vertices.data(), vertexDataSize);
      mappedDataOffset += vertexDataSize;

      meshSection.positionOnlyVertexOffset = mappedDataOffset;
      for (std::size_t i = 0; i < sectionData.vertices.size(); ++i)
      {
         std::memcpy(mappedData + mappedDataOffset + i * sizeof(glm::vec3), &sectionData.vertices[i], sizeof(glm::vec3));
      }
      mappedDataOffset += positionOnlyVertexDataSize;

      meshSection.indexOffset = mappedDataOffset;
      std::memcpy(mappedData + mappedDataOffset, sectionData.indices.data(), indexDataSize);
      mappedDataOffset += indexDataSize;

      meshSection.bounds = sectionData.bounds;
      meshSection.materialHandle = sectionData.materialHandle;

      sections.push_back(meshSection);
   }
   mappedData = nullptr;

   std::array<Buffer::CopyInfo, 1> copyInfo;
   copyInfo[0].srcBuffer = stagingBuffer;
   copyInfo[0].dstBuffer = buffer;
   copyInfo[0].size = bufferSize;

   Buffer::copy(context, copyInfo);

   vmaDestroyBuffer(context.getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
   stagingBuffer = nullptr;
   stagingBufferAllocation = nullptr;
}

Mesh::~Mesh()
{
   ASSERT(buffer);
   ASSERT(bufferAllocation);
   context.delayedDestroy(std::move(buffer), std::move(bufferAllocation));
}

void Mesh::bindBuffers(vk::CommandBuffer commandBuffer, uint32_t section, bool positionOnly) const
{
   commandBuffer.bindVertexBuffers(0, { buffer }, { positionOnly ? sections[section].positionOnlyVertexOffset : sections[section].vertexOffset });
   commandBuffer.bindIndexBuffer(buffer, sections[section].indexOffset, vk::IndexType::eUint32);
}

void Mesh::draw(vk::CommandBuffer commandBuffer, uint32_t section) const
{
   commandBuffer.drawIndexed(sections[section].numIndices, 1, 0, 0, 0);
}
