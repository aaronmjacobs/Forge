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
   buffer = device.createBuffer(bufferCreateInfo);
   NAME_CHILD(buffer, "Mesh Buffer");

   vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(memoryRequirements.size)
      .setMemoryTypeIndex(Memory::findType(context.getPhysicalDevice(), memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

   deviceMemory = device.allocateMemory(allocateInfo);
   device.bindBufferMemory(buffer, deviceMemory, 0);
   NAME_CHILD(deviceMemory, "Mesh Buffer Memory");

   // Create staging buffer, and use it to copy over data

   vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo()
      .setSize(bufferCreateInfo.size)
      .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
      .setSharingMode(vk::SharingMode::eExclusive);
   vk::Buffer stagingBuffer = device.createBuffer(stagingBufferCreateInfo);

   vk::MemoryRequirements stagingMemoryRequirements = device.getBufferMemoryRequirements(stagingBuffer);
   vk::MemoryAllocateInfo stagingAllocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(stagingMemoryRequirements.size)
      .setMemoryTypeIndex(Memory::findType(context.getPhysicalDevice(), stagingMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

   vk::DeviceMemory stagingDeviceMemory = device.allocateMemory(stagingAllocateInfo);
   device.bindBufferMemory(stagingBuffer, stagingDeviceMemory, 0);

   void* mappedData = device.mapMemory(stagingDeviceMemory, 0, stagingAllocateInfo.allocationSize, vk::MemoryMapFlags());
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
      std::memcpy(static_cast<uint8_t*>(mappedData) + mappedDataOffset, sectionData.vertices.data(), vertexDataSize);
      mappedDataOffset += vertexDataSize;

      meshSection.positionOnlyVertexOffset = mappedDataOffset;
      for (std::size_t i = 0; i < sectionData.vertices.size(); ++i)
      {
         std::memcpy(static_cast<uint8_t*>(mappedData) + mappedDataOffset + i * sizeof(glm::vec3), &sectionData.vertices[i], sizeof(glm::vec3));
      }
      mappedDataOffset += positionOnlyVertexDataSize;

      meshSection.indexOffset = mappedDataOffset;
      std::memcpy(static_cast<uint8_t*>(mappedData) + mappedDataOffset, sectionData.indices.data(), indexDataSize);
      mappedDataOffset += indexDataSize;

      meshSection.bounds = sectionData.bounds;
      meshSection.materialHandle = sectionData.materialHandle;

      sections.push_back(meshSection);
   }
   device.unmapMemory(stagingDeviceMemory);
   mappedData = nullptr;

   std::array<Buffer::CopyInfo, 1> copyInfo;
   copyInfo[0].srcBuffer = stagingBuffer;
   copyInfo[0].dstBuffer = buffer;
   copyInfo[0].size = bufferSize;

   Buffer::copy(context, copyInfo);

   device.destroyBuffer(stagingBuffer);
   stagingBuffer = nullptr;

   device.freeMemory(stagingDeviceMemory);
   stagingDeviceMemory = nullptr;
}

Mesh::~Mesh()
{
   ASSERT(buffer);
   context.delayedDestroy(std::move(buffer));

   ASSERT(deviceMemory);
   context.delayedFree(std::move(deviceMemory));
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
