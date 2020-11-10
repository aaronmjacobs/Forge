#include "Graphics/Texture.h"

#include <memory>

struct ImageDataDeleter
{
   void operator()(uint8_t* data) const;
};

struct LoadedImage
{
   std::unique_ptr<uint8_t, ImageDataDeleter> data;
   vk::DeviceSize size = 0;

   ImageProperties properties;
};
