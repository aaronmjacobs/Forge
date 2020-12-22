#include "Graphics/Mesh.h"

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/Memory.h"

#include <limits>

// static
std::array<vk::VertexInputBindingDescription, 1> Vertex::getBindingDescriptions()
{
   return
   {
      vk::VertexInputBindingDescription()
      .setBinding(0)
      .setStride(sizeof(Vertex))
      .setInputRate(vk::VertexInputRate::eVertex)
   };
}

// static
std::array<vk::VertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
   return
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
         .setOffset(offsetof(Vertex, color)),
      vk::VertexInputAttributeDescription()
         .setLocation(2)
         .setBinding(0)
         .setFormat(vk::Format::eR32G32Sfloat)
         .setOffset(offsetof(Vertex, texCoord))
   };
}

Mesh::Mesh(const VulkanContext& context, const std::vector<MeshSectionSourceData>& sourceData)
   : GraphicsResource(context)
{
   vk::DeviceSize bufferSize = 0;
   for (const MeshSectionSourceData& sectionData : sourceData)
   {
      bufferSize += sectionData.vertices.size() * sizeof(Vertex);
      bufferSize += sectionData.indices.size() * sizeof(uint32_t);
   }

   // Create the buffer

   vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
      .setSize(bufferSize)
      .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer)
      .setSharingMode(vk::SharingMode::eExclusive);
   buffer = context.device.createBuffer(bufferCreateInfo);

   vk::MemoryRequirements memoryRequirements = context.device.getBufferMemoryRequirements(buffer);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(memoryRequirements.size)
      .setMemoryTypeIndex(Memory::findType(context.physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

   deviceMemory = context.device.allocateMemory(allocateInfo);
   context.device.bindBufferMemory(buffer, deviceMemory, 0);

   // Create staging buffer, and use it to copy over data

   vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo()
      .setSize(bufferCreateInfo.size)
      .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
      .setSharingMode(vk::SharingMode::eExclusive);
   vk::Buffer stagingBuffer = context.device.createBuffer(stagingBufferCreateInfo);

   vk::MemoryRequirements stagingMemoryRequirements = context.device.getBufferMemoryRequirements(stagingBuffer);
   vk::MemoryAllocateInfo stagingAllocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(stagingMemoryRequirements.size)
      .setMemoryTypeIndex(Memory::findType(context.physicalDevice, stagingMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

   vk::DeviceMemory stagingDeviceMemory = context.device.allocateMemory(stagingAllocateInfo);
   context.device.bindBufferMemory(stagingBuffer, stagingDeviceMemory, 0);

   void* mappedData = context.device.mapMemory(stagingDeviceMemory, 0, stagingAllocateInfo.allocationSize, vk::MemoryMapFlags());
   std::size_t mappedDataOffset = 0;
   sections.reserve(sourceData.size());
   for (const MeshSectionSourceData& sectionData : sourceData)
   {
      MeshSection meshSection;

      ASSERT(sectionData.indices.size() <= std::numeric_limits<uint32_t>::max());
      meshSection.numIndices = static_cast<uint32_t>(sectionData.indices.size());

      std::size_t vertexDataSize = sectionData.vertices.size() * sizeof(Vertex);
      std::size_t indexDataSize = sectionData.indices.size() * sizeof(uint32_t);

      std::memcpy(static_cast<uint8_t*>(mappedData) + mappedDataOffset, sectionData.vertices.data(), vertexDataSize);
      mappedDataOffset += vertexDataSize;

      meshSection.indexOffset = mappedDataOffset;
      std::memcpy(static_cast<uint8_t*>(mappedData) + mappedDataOffset, sectionData.indices.data(), indexDataSize);
      mappedDataOffset += indexDataSize;

      sections.push_back(meshSection);
   }
   context.device.unmapMemory(stagingDeviceMemory);
   mappedData = nullptr;

   Buffer::CopyInfo copyInfo;
   copyInfo.srcBuffer = stagingBuffer;
   copyInfo.dstBuffer = buffer;
   copyInfo.size = bufferSize;

   Buffer::copy(context, { copyInfo });

   context.device.destroyBuffer(stagingBuffer);
   stagingBuffer = nullptr;

   context.device.freeMemory(stagingDeviceMemory);
   stagingDeviceMemory = nullptr;
}

Mesh::~Mesh()
{
   ASSERT(buffer);
   device.destroyBuffer(buffer);

   ASSERT(deviceMemory);
   device.freeMemory(deviceMemory);
}

void Mesh::bindBuffers(vk::CommandBuffer commandBuffer, uint32_t section) const
{
   commandBuffer.bindVertexBuffers(0, { buffer }, { 0 });
   commandBuffer.bindIndexBuffer(buffer, sections[section].indexOffset, vk::IndexType::eUint32);
}

void Mesh::draw(vk::CommandBuffer commandBuffer, uint32_t section) const
{
   commandBuffer.drawIndexed(sections[section].numIndices, 1, 0, 0, 0);
}
