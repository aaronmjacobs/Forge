#include "Graphics/Material.h"

class Texture;

class SimpleMaterial : public Material
{
public:
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   SimpleMaterial(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const Texture& texture, vk::Sampler sampler);
};
