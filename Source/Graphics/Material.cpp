#include "Graphics/Material.h"

#include "Graphics/DebugUtils.h"

#include "Resources/MaterialResourceManager.h"

Material::Material(const GraphicsContext& graphicsContext, MaterialResourceManager& owningResourceManager, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , materialResourceManager(owningResourceManager)
   , descriptorSet(graphicsContext, owningResourceManager.getDynamicDescriptorPool(), createInfo)
{
   NAME_CHILD(descriptorSet, "Descriptor Set");
}
