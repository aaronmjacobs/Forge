#pragma once

#include "Core/Macros.h"

#if FORGE_DEBUG

#  include "Graphics/GraphicsResource.h"

#  if defined(__cpp_lib_bit_cast)
#     include <bit>
#  else
#     include <cstring>
#  endif
#  include <string>
#  include <type_traits>

namespace DebugUtils
{
   vk::DispatchLoaderDynamic& GetDynamicLoader();

   bool AreLabelsEnabled();
   void SetLabelsEnabled(bool enabled);

   template<typename ObjectType, std::enable_if_t<std::is_base_of<GraphicsResource, ObjectType>::value, int*> = nullptr>
   void setObjectName(vk::Device device, ObjectType& object, const char* name)
   {
      object.setName(name);
   }

   template<typename ObjectType, std::enable_if_t<!std::is_base_of<GraphicsResource, ObjectType>::value, int*> = nullptr>
   void setObjectName(vk::Device device, ObjectType& object, const char* name)
   {
      uint64_t handle = 0;
#if defined(__cpp_lib_bit_cast)
      handle = std::bit_cast<uint64_t>(object);
#else
      static_assert(sizeof(handle) == sizeof(object), "Trying to set name for object of invalid type");
      std::memcpy(&handle, &object, sizeof(handle));
#endif

      vk::DebugUtilsObjectNameInfoEXT nameInfo(ObjectType::objectType, handle, name);
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
#  define NAME_POINTER(object_pointer, object_name) do { if (object_pointer) { NAME_OBJECT(*object_pointer, object_name); } } while(0)

#else

#  define SCOPED_LABEL(label_name) FORGE_NO_OP
#  define SCOPED_COLORED_LABEL(label_name, color_value) FORGE_NO_OP

#  define INLINE_LABEL(label_name) FORGE_NO_OP
#  define INLINE_COLORED_LABEL(label_name, color_value) FORGE_NO_OP

#  define NAME_OBJECT(object_variable, object_name) FORGE_NO_OP
#  define NAME_POINTER(object_pointer, object_name) FORGE_NO_OP

#endif
