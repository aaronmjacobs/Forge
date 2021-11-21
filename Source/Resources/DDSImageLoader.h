#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class Image;

namespace DDSImageLoader
{
   std::unique_ptr<Image> loadImage(std::vector<uint8_t> fileData);
}
