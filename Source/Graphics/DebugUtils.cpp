#include "Graphics/DebugUtils.h"

#if FORGE_DEBUG
namespace DebugUtils
{
   namespace
   {
      bool labelsEnabled = true;
   }

   bool AreLabelsEnabled()
   {
      return labelsEnabled;
   }

   void SetLabelsEnabled(bool enabled)
   {
      labelsEnabled = enabled;
   }

   void InsertInlineLabel(vk::CommandBuffer commandBuffer, const char* labelName, const std::array<float, 4>& color /*= {}*/)
   {
      if (AreLabelsEnabled())
      {
         vk::DebugUtilsLabelEXT label(labelName);
         commandBuffer.insertDebugUtilsLabelEXT(label, GraphicsContext::GetDynamicLoader());
      }
   }

   ScopedCommandBufferLabel::ScopedCommandBufferLabel(vk::CommandBuffer commandBuffer_, const char* labelName, const std::array<float, 4>& color /*= {}*/)
      : commandBuffer(commandBuffer_)
   {
      if (AreLabelsEnabled())
      {
         vk::DebugUtilsLabelEXT label(labelName, color);
         commandBuffer.beginDebugUtilsLabelEXT(label, GraphicsContext::GetDynamicLoader());
      }
   }

   ScopedCommandBufferLabel::~ScopedCommandBufferLabel()
   {
      if (commandBuffer)
      {
         commandBuffer.endDebugUtilsLabelEXT(GraphicsContext::GetDynamicLoader());
      }
   }
}
#endif
