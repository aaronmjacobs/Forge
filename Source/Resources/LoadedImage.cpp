#include "Resources/LoadedImage.h"

#include <stb_image.h>

void ImageDataDeleter::operator()(uint8_t* data) const
{
   stbi_image_free(data);
}
