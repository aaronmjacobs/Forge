#include "Graphics/Material.h"

#include "Graphics/DebugUtils.h"

Material::Material(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool, createInfo)
{
   NAME_CHILD(descriptorSet, "Descriptor Set");
}
