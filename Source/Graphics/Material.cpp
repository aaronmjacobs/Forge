#include "Graphics/Material.h"

#include "Graphics/DebugUtils.h"

Material::Material(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, createInfo)
{
   NAME_CHILD(descriptorSet, "Descriptor Set");
}
