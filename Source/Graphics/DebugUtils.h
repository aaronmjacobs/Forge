#pragma once

#include "Core/Macros.h"

#if FORGE_DEBUG

#  include "Graphics/GraphicsResource.h"

#  include <bit>
#  include <string>
#  include <type_traits>

namespace DebugUtils
{
   vk::DispatchLoaderDynamic& GetDynamicLoader();

   bool AreLabelsEnabled();
   void SetLabelsEnabled(bool enabled);

   template<typename ObjectType>
   void setObjectName(vk::Device device, ObjectType& object, const char* name) requires std::is_base_of<GraphicsResource, ObjectType>::value
   {
      object.setName(name);
   }

   template<typename ObjectType>
   void setObjectName(vk::Device device, ObjectType& object, const char* name) requires !std::is_base_of<GraphicsResource, ObjectType>::value
   {
      vk::DebugUtilsObjectNameInfoEXT nameInfo(ObjectType::objectType, std::bit_cast<uint64_t>(object), name);
      device.setDebugUtilsObjectNameEXT(nameInfo, GetDynamicLoader());
   }

   template<typename ObjectType>
   void setObjectName(vk::Device device, ObjectType& object, const std::string& name)
   {
      setObjectName(device, object, name.c_str());
   }

   void InsertInlineLabel(vk::CommandBuffer commandBuffer, const char* labelName, const std::array<float, 4>& color = {});

   struct ScopedCommandBufferLabel
   {
      ScopedCommandBufferLabel(vk::CommandBuffer commandBuffer_, const char* labelName, const std::array<float, 4>& color = {});

      ScopedCommandBufferLabel(vk::CommandBuffer commandBuffer_, const std::string& labelName, const std::array<float, 4>& color = {})
         : ScopedCommandBufferLabel(commandBuffer_, labelName.c_str(), color)
      {
      }

      ~ScopedCommandBufferLabel();

      vk::CommandBuffer commandBuffer;
   };
}

#  define SCOPED_LABEL(label_name) DebugUtils::ScopedCommandBufferLabel scopedCommandBufferLabel##__LINE__(commandBuffer, label_name)
#  define SCOPED_COLORED_LABEL(label_name, color_value) DebugUtils::ScopedCommandBufferLabel scopedCommandBufferLabel##__LINE__(commandBuffer, label_name, color_value)

#  define INLINE_LABEL(label_name) DebugUtils::InsertInlineLabel(commandBuffer, label_name)
#  define INLINE_COLORED_LABEL(label_name, color_value) DebugUtils::InsertInlineLabel(commandBuffer, label_name, color_value)

#  define NAME_OBJECT(object_variable, object_name) DebugUtils::setObjectName(device, object_variable, object_name)

#else

#  define SCOPED_LABEL(label_name) FORGE_NO_OP
#  define SCOPED_COLORED_LABEL(label_name, color_value) FORGE_NO_OP

#  define INLINE_LABEL(label_name) FORGE_NO_OP
#  define INLINE_COLORED_LABEL(label_name, color_value) FORGE_NO_OP

#  define NAME_OBJECT(object_variable, object_name) FORGE_NO_OP

#endif
