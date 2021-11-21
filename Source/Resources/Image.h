#pragma once

#include "Graphics/TextureInfo.h"

#include <cstdint>
#include <span>

class Image
{
public:
   Image() = default;
   Image(const ImageProperties& imageProperties)
      : properties(imageProperties)
   {
   }

   virtual ~Image() = default;

   virtual TextureData getTextureData() const = 0;

   const ImageProperties& getProperties() const
   {
      return properties;
   }

protected:
   ImageProperties properties;
};
