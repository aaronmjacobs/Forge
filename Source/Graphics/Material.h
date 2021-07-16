#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

enum class BlendMode
{
   Opaque,
   Translucent
};

class Material : public GraphicsResource
{
public:
   Material(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo);

   const DescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

   BlendMode getBlendMode() const
   {
      return blendMode;
   }

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

protected:
   DescriptorSet descriptorSet;
   BlendMode blendMode = BlendMode::Opaque;
};
