#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class Image;

namespace STBImageLoader
{
   std::unique_ptr<Image> loadImage(std::vector<uint8_t> fileData, bool sRGB);
}
