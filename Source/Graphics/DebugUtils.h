#pragma once

#include "Core/Macros.h"

#if FORGE_WITH_DEBUG_UTILS

#  include "Core/Types.h"

#	include "Graphics/Vulkan.h"

#  include <array>
#  include <string>
#  include <type_traits>

class GraphicsResource;

namespace DebugUtils
{
   const char* getResourceName(const GraphicsResource* resource);
   const char* getObjectName(uint64_t objectHandle);

   void setResourceName(vk::Device device, GraphicsResource* resource, const char* name);
   void setObjectName(vk::Device device, uint64_t objectHandle, vk::ObjectType objectType, const char* name);

   void setResourceParent(vk::Device device, GraphicsResource* resource, GraphicsResource* parent);
   void setObjectParent(vk::Device device, uint64_t objectHandle, vk::ObjectType objectType, GraphicsResource* parent);

   void onResourceMoved(GraphicsResource* oldResource, GraphicsResource* newResource);
   void onResourceDestroyed(GraphicsResource* resource);

   const std::string& toString(uint64_t n);

   template<typename T>
   const std::string& toString(T n)
   {
      return toString(static_cast<uint64_t>(n));
   }

   template<typename ItemType>
   const char* getItemName(ItemType& item) requires (std::is_base_of_v<GraphicsResource, ItemType>)
   {
      return getResourceName(item);
   }

   template<typename ItemType>
   const char* getItemName(ItemType& item) requires (!std::is_base_of_v<GraphicsResource, ItemType>)
   {
      return getObjectName(Types::bit_cast<uint64_t>(item));
   }

   template<typename ItemType>
   void setItemName(vk::Device device, ItemType& item, const char* name) requires (std::is_base_of_v<GraphicsResource, ItemType>)
   {
      setResourceName(device, &item, name);
   }

   template<typename ItemType>
   void setItemName(vk::Device device, ItemType& item, const char* name) requires (!std::is_base_of_v<GraphicsResource, ItemType>)
   {
      setObjectName(device, Types::bit_cast<uint64_t>(item), ItemType::objectType, name);
   }

   template<typename ItemType>
   void setItemName(vk::Device device, ItemType& item, const std::string& name)
   {
      setItemName(device, item, name.c_str());
   }

   template<typename ItemType>
   void setChildName(vk::Device device, ItemType& item, GraphicsResource& parent, const char* name) requires (std::is_base_of_v<GraphicsResource, ItemType>)
   {
      setResourceParent(device, &item, &parent);
      setItemName(device, item, name);
   }

   template<typename ItemType>
   void setChildName(vk::Device device, ItemType& item, GraphicsResource& parent, const char* name) requires (!std::is_base_of_v<GraphicsResource, ItemType>)
   {
      setObjectParent(device, Types::bit_cast<uint64_t>(item), ItemType::objectType, &parent);
      setItemName(device, item, name);
   }

   template<typename ItemType>
   void setChildName(vk::Device device, ItemType& item, GraphicsResource& parent, const std::string& name)
   {
      setChildName(device, item, parent, name.c_str());
   }

   bool labelsAreEnabled();
   void setLabelsEnabled(bool enabled);

   void insertInlineLabel(vk::CommandBuffer commandBuffer, const char* labelName, const std::array<float, 4>& color = {});

   inline void insertInlineLabel(vk::CommandBuffer commandBuffer, const std::string& labelName, const std::array<float, 4>& color = {})
   {
      insertInlineLabel(commandBuffer, labelName.c_str(), color);
   }

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

#  define INLINE_LABEL(label_name) DebugUtils::insertInlineLabel(commandBuffer, label_name)
#  define INLINE_COLORED_LABEL(label_name, color_value) DebugUtils::insertInlineLabel(commandBuffer, label_name, color_value)

#  define NAME_ITEM(device, object_variable, object_name) DebugUtils::setItemName(device, object_variable, object_name)
#  define NAME_POINTER(device, object_pointer, object_name) do { if (object_pointer) { NAME_ITEM(device, *object_pointer, object_name); } } while(0)

#  define NAME_CHILD(object_variable, object_name) DebugUtils::setChildName(device, object_variable, *this, object_name)
#  define NAME_CHILD_POINTER(object_pointer, object_name) do { if (object_pointer) { NAME_CHILD(*object_pointer, object_name); } } while(0)

#  define NAME_CHILD_IF_UNNAMED(object_variable, object_name) do { if (DebugUtils::getItemName(object_variable) == nullptr) { NAME_CHILD(object_variable, object_name); } } while (0)

#else

#  define SCOPED_LABEL(label_name) FORGE_NO_OP
#  define SCOPED_COLORED_LABEL(label_name, color_value) FORGE_NO_OP

#  define INLINE_LABEL(label_name) FORGE_NO_OP
#  define INLINE_COLORED_LABEL(label_name, color_value) FORGE_NO_OP

#  define NAME_ITEM(device, object_variable, object_name) FORGE_NO_OP
#  define NAME_POINTER(device, object_pointer, object_name) FORGE_NO_OP

#  define NAME_CHILD(object_variable, object_name) FORGE_NO_OP
#  define NAME_CHILD_POINTER(object_pointer, object_name) FORGE_NO_OP

#endif
