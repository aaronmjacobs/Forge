#include "Graphics/Mesh.h"

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/Memory.h"

#include <limits>

// static
vk::VertexInputBindingDescription Vertex::getBindingDescription()
{
   return vk::VertexInputBindingDescription()
      .setBinding(0)
      .setStride(sizeof(Vertex))
      .setInputRate(vk::VertexInputRate::eVertex);
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

Mesh::Mesh(const VulkanContext& context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
   : device(context.device)
{
   vk::DeviceSize vertexDataSize = vertices.size() * sizeof(Vertex);
   vk::DeviceSize indexDataSize = indices.size() * sizeof(uint32_t);

   // Create the buffer

   vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
      .setSize(vertexDataSize + indexDataSize)
      .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer)
      .setSharingMode(vk::SharingMode::eExclusive);
   buffer = context.device.createBuffer(bufferCreateInfo);

   vk::MemoryRequirements memoryRequirements = context.device.getBufferMemoryRequirements(buffer);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(memoryRequirements.size)
      .setMemoryTypeIndex(Memory::findType(context.physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

   deviceMemory = context.device.allocateMemory(allocateInfo);
   context.device.bindBufferMemory(buffer, deviceMemory, 0);

   indexOffset = vertexDataSize;
   ASSERT(indices.size() <= std::numeric_limits<uint32_t>::max());
   numIndices = static_cast<uint32_t>(indices.size());

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
   std::memcpy(mappedData, vertices.data(), static_cast<std::size_t>(vertexDataSize));
   std::memcpy(static_cast<uint8_t*>(mappedData) + vertexDataSize, indices.data(), static_cast<std::size_t>(indexDataSize));
   context.device.unmapMemory(stagingDeviceMemory);
   mappedData = nullptr;

   Buffer::CopyInfo copyInfo;
   copyInfo.srcBuffer = stagingBuffer;
   copyInfo.dstBuffer = buffer;
   copyInfo.size = vertexDataSize + indexDataSize;

   Buffer::copy(context, { copyInfo });

   context.device.destroyBuffer(stagingBuffer);
   stagingBuffer = nullptr;

   context.device.freeMemory(stagingDeviceMemory);
   stagingDeviceMemory = nullptr;
}

Mesh::Mesh(Mesh&& other)
{
   move(std::move(other));
}

Mesh::~Mesh()
{
   release();
}

void Mesh::bindBuffers(vk::CommandBuffer commandBuffer)
{
   commandBuffer.bindVertexBuffers(0, { buffer }, { 0 });
   commandBuffer.bindIndexBuffer(buffer, indexOffset, vk::IndexType::eUint32);
}

void Mesh::draw(vk::CommandBuffer commandBuffer)
{
   commandBuffer.drawIndexed(numIndices, 1, 0, 0, 0);
}

Mesh& Mesh::operator=(Mesh&& other)
{
   move(std::move(other));
   release();

   return *this;
}

void Mesh::move(Mesh&& other)
{
   ASSERT(!device);
   device = other.device;
   other.device = nullptr;

   ASSERT(!buffer);
   buffer = other.buffer;
   other.buffer = nullptr;

   ASSERT(!deviceMemory);
   deviceMemory = other.deviceMemory;
   other.deviceMemory = nullptr;

   indexOffset = other.indexOffset;
   other.indexOffset = 0;

   numIndices = other.numIndices;
   other.numIndices = 0;
}

void Mesh::release()
{
   if (device)
   {
      ASSERT(buffer);
      device.destroyBuffer(buffer);
      buffer = nullptr;

      ASSERT(deviceMemory);
      device.freeMemory(deviceMemory);
      deviceMemory = nullptr;
   }

   indexOffset = 0;
   numIndices = 0;
}
