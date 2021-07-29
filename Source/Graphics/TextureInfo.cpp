#include "Graphics/TextureInfo.h"

BasicTextureInfo TextureInfo::asBasic() const
{
	BasicTextureInfo basicInfo;

	basicInfo.format = format;
	basicInfo.sampleCount = sampleCount;
	basicInfo.isSwapchainTexture = isSwapchainTexture;

	return basicInfo;
}

BasicAttachmentInfo AttachmentInfo::asBasic() const
{
   BasicAttachmentInfo basicInfo;

   if (depthInfo.has_value())
   {
      basicInfo.depthInfo = depthInfo->asBasic();
   }

   basicInfo.colorInfo.reserve(colorInfo.size());
   for (const TextureInfo& color : colorInfo)
   {
      basicInfo.colorInfo.push_back(color.asBasic());
   }

   basicInfo.resolveInfo.reserve(resolveInfo.size());
   for (const TextureInfo& resolve : resolveInfo)
   {
      basicInfo.resolveInfo.push_back(resolve.asBasic());
   }

   return basicInfo;
}
