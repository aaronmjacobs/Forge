#include "Graphics/Material.h"

class Texture;

class PhongMaterial : public Material
{
public:
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);
   static const std::string kDiffuseTextureParameterName;
   static const std::string kNormalTextureParameterName;

   PhongMaterial(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Sampler sampler, const Texture& diffuseTexture, const Texture& normalTexture);
};
