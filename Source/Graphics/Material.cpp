#include "Graphics/Material.h"

#include "Graphics/DebugUtils.h"

#include "Resources/MaterialLoader.h"

Material::Material(const GraphicsContext& graphicsContext, MaterialLoader& owningMaterialLoader, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , materialLoader(owningMaterialLoader)
   , descriptorSet(graphicsContext, owningMaterialLoader.getDynamicDescriptorPool(), createInfo)
{
   NAME_CHILD(descriptorSet, "Descriptor Set");
}
