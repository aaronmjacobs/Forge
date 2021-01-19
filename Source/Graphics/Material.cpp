#include "Graphics/Material.h"

Material::Material(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, createInfo)
{
}
