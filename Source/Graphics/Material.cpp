#include "Graphics/Material.h"

#include "Graphics/DebugUtils.h"

Material::Material(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, createInfo)
{
}

#if FORGE_DEBUG
void Material::setName(std::string_view newName)
{
   GraphicsResource::setName(newName);

   NAME_OBJECT(descriptorSet, name);
}
#endif // FORGE_DEBUG
