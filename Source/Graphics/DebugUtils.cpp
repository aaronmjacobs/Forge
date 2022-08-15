#if FORGE_WITH_DEBUG_UTILS

#include "Graphics/DebugUtils.h"

#include "Core/Assert.h"

#include "Graphics/GraphicsResource.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace DebugUtils
{
   namespace
   {
      struct NameInfo
      {
         std::string baseName;
         std::string compositeName;

         NameInfo* parent = nullptr;
         std::unordered_set<NameInfo*> children;

         GraphicsResource* resource = nullptr;

         uint64_t objectHandle = 0;
         vk::ObjectType objectType = vk::ObjectType::eUnknown;
      };

      std::unordered_map<const GraphicsResource*, std::unique_ptr<NameInfo>> namedResources;
      std::unordered_map<uint64_t, std::unique_ptr<NameInfo>> namedObjects;

      bool labelsEnabled = true;

      NameInfo* getNameInfo(const GraphicsResource* resource)
      {
         auto location = namedResources.find(resource);
         return location == namedResources.end() ? nullptr : location->second.get();
      }

      NameInfo* getNameInfo(uint64_t object)
      {
         auto location = namedObjects.find(object);
         return location == namedObjects.end() ? nullptr : location->second.get();
      }

      NameInfo* addNameInfo(GraphicsResource* resource)
      {
         ASSERT(resource);
         ASSERT(!namedResources.contains(resource));

         auto [location, _] = namedResources.emplace(resource, std::make_unique<NameInfo>());
         NameInfo* nameInfo = location->second.get();
         nameInfo->resource = resource;

         return nameInfo;
      }

      NameInfo* addNameInfo(uint64_t objectHandle, vk::ObjectType objectType)
      {
         ASSERT(objectHandle);
         ASSERT(!namedObjects.contains(objectHandle));

         auto [location, _] = namedObjects.emplace(objectHandle, std::make_unique<NameInfo>());
         NameInfo* nameInfo = location->second.get();
         nameInfo->objectHandle = objectHandle;
         nameInfo->objectType = objectType;

         return nameInfo;
      }

      NameInfo* getOrAddNameInfo(GraphicsResource* resource)
      {
         NameInfo* nameInfo = getNameInfo(resource);
         if (!nameInfo)
         {
            nameInfo = addNameInfo(resource);
         }

         return nameInfo;
      }

      NameInfo* getOrAddNameInfo(uint64_t objectHandle, vk::ObjectType objectType)
      {
         NameInfo* nameInfo = getNameInfo(objectHandle);
         if (!nameInfo)
         {
            nameInfo = addNameInfo(objectHandle, objectType);
         }

         return nameInfo;
      }

      void removeNameInfo(NameInfo* nameInfo)
      {
         ASSERT(nameInfo);

         if (nameInfo->parent)
         {
            nameInfo->parent->children.erase(nameInfo);
         }

         std::unordered_set<NameInfo*> children = nameInfo->children;
         for (NameInfo* child : children)
         {
            removeNameInfo(child);
         }
         ASSERT(nameInfo->children.empty());

         if (nameInfo->resource)
         {
            ASSERT(nameInfo->objectHandle == 0);
            ASSERT(nameInfo->objectType == vk::ObjectType::eUnknown);

            namedResources.erase(nameInfo->resource);
         }
         else
         {
            ASSERT(nameInfo->resource == nullptr);
            ASSERT(nameInfo->objectHandle != 0);

            namedObjects.erase(nameInfo->objectHandle);
         }
      }

      void setNameDirect(vk::Device device, GraphicsResource* resource, uint64_t objectHandle, vk::ObjectType objectType, const char* name)
      {
         if (resource)
         {
            ASSERT(objectHandle == 0);
            ASSERT(objectType == vk::ObjectType::eUnknown);

            resource->updateCachedCompositeName(name);
         }
         else
         {
            ASSERT(resource == nullptr);
            ASSERT(objectHandle != 0);

            vk::DebugUtilsObjectNameInfoEXT objectNameInfo(objectType, objectHandle, name);
            device.setDebugUtilsObjectNameEXT(objectNameInfo, GraphicsContext::GetDynamicLoader());
         }
      }

      void updateCompositeName(vk::Device device, NameInfo* nameInfo)
      {
         ASSERT(device);
         ASSERT(nameInfo);

         std::string newCompositeName;
         if (nameInfo->parent && !nameInfo->parent->compositeName.empty())
         {
            const char* spacer = nameInfo->baseName.empty() ? "" : " :: ";
            newCompositeName = nameInfo->parent->compositeName + spacer + nameInfo->baseName;
         }
         else
         {
            newCompositeName = nameInfo->baseName;
         }

         bool compositeNameChanged = newCompositeName != nameInfo->compositeName;
         nameInfo->compositeName = std::move(newCompositeName);

         if (compositeNameChanged)
         {
            setNameDirect(device, nameInfo->resource, nameInfo->objectHandle, nameInfo->objectType, nameInfo->compositeName.c_str());
         }

         for (NameInfo* child : nameInfo->children)
         {
            updateCompositeName(device, child);
         }
      }

      void setBaseName(vk::Device device, NameInfo* nameInfo, const char* baseName)
      {
         ASSERT(device);
         ASSERT(nameInfo);

         nameInfo->baseName = baseName;
         updateCompositeName(device, nameInfo);
      }

      void setNameInfoParent(vk::Device device, NameInfo* nameInfo, NameInfo* parent)
      {
         ASSERT(nameInfo);

         if (nameInfo->parent)
         {
            nameInfo->parent->children.erase(nameInfo);
         }

         if (parent)
         {
            NameInfo* parentIterator = parent;
            while (parentIterator)
            {
               if (parentIterator == nameInfo)
               {
                  ASSERT(false, "Trying to add name info to a hierarchy that it already exists in, which would cause infinite recursion!");
                  parent = nullptr;
                  break;
               }

               parentIterator = parentIterator->parent;
            }
         }

         if (parent)
         {
            parent->children.insert(nameInfo);
         }

         nameInfo->parent = parent;

         updateCompositeName(device, nameInfo);
      }
   }

   const char* getResourceName(const GraphicsResource* resource)
   {
      if (resource)
      {
         if (const NameInfo* nameInfo = getNameInfo(resource))
         {
            return nameInfo->compositeName.c_str();
         }
      }

      return nullptr;
   }

   const char* getObjectName(uint64_t objectHandle)
   {
      if (objectHandle)
      {
         if (const NameInfo* nameInfo = getNameInfo(objectHandle))
         {
            return nameInfo->compositeName.c_str();
         }
      }

      return nullptr;
   }

   void setResourceName(vk::Device device, GraphicsResource* resource, const char* name)
   {
      if (device && resource)
      {
         NameInfo* nameInfo = getOrAddNameInfo(resource);
         ASSERT(nameInfo);

         setBaseName(device, nameInfo, name);
      }
   }

   void setObjectName(vk::Device device, uint64_t objectHandle, vk::ObjectType objectType, const char* name)
   {
      if (device && objectHandle)
      {
         // To avoid keeping names around for objects that may no longer exist, we only track children of resources (which will be cleaned up when the owning resource is destroyed)
         if (NameInfo* nameInfo = getNameInfo(objectHandle))
         {
            setBaseName(device, nameInfo, name);
         }
         else
         {
            setNameDirect(device, nullptr, objectHandle, objectType, name);
         }
      }
   }

   void setResourceParent(vk::Device device, GraphicsResource* resource, GraphicsResource* parent)
   {
      if (device && resource)
      {
         NameInfo* nameInfo = getOrAddNameInfo(resource);
         ASSERT(nameInfo);

         NameInfo* parentNameInfo = parent ? getOrAddNameInfo(parent) : nullptr;

         setNameInfoParent(device, nameInfo, parentNameInfo);
      }
   }

   void setObjectParent(vk::Device device, uint64_t objectHandle, vk::ObjectType objectType, GraphicsResource* parent)
   {
      if (device && objectHandle)
      {
         NameInfo* nameInfo = getOrAddNameInfo(objectHandle, objectType);
         ASSERT(nameInfo);

         NameInfo* parentNameInfo = parent ? getOrAddNameInfo(parent) : nullptr;

         setNameInfoParent(device, nameInfo, parentNameInfo);
      }
   }

   void onResourceMoved(GraphicsResource* oldResource, GraphicsResource* newResource)
   {
      if (oldResource && newResource)
      {
         auto location = namedResources.find(oldResource);
         if (location != namedResources.end())
         {
            std::unique_ptr<NameInfo> nameInfo = std::move(location->second);
            namedResources.erase(location);

            ASSERT(nameInfo->resource == oldResource);
            nameInfo->resource = newResource;

            namedResources.emplace(newResource, std::move(nameInfo));
         }
      }
   }

   void onResourceDestroyed(GraphicsResource* resource)
   {
      if (resource)
      {
         if (NameInfo* nameInfo = getNameInfo(resource))
         {
            removeNameInfo(nameInfo);
         }
      }
   }

   const std::string& toString(uint64_t n)
   {
      static std::array<std::string, 150> lookupTable;
      static std::array<std::string, 10> dynamic;
      static std::size_t dynamicIndex = 0;
      static bool initialized = false;

      if (!initialized)
      {
         for (uint64_t i = 0; i < lookupTable.size(); ++i)
         {
            lookupTable[i] = std::to_string(i);
         }
         initialized = true;
      }

      if (n < lookupTable.size())
      {
         return lookupTable[n];
      }

      std::size_t index = dynamicIndex;
      dynamicIndex = (dynamicIndex + 1) % dynamic.size();

      dynamic[index] = std::to_string(n);
      return dynamic[index];
   }

   bool areLabelsEnabled()
   {
      return labelsEnabled;
   }

   void setLabelsEnabled(bool enabled)
   {
      labelsEnabled = enabled;
   }

   void insertInlineLabel(vk::CommandBuffer commandBuffer, const char* labelName, const std::array<float, 4>& color /*= {}*/)
   {
      if (areLabelsEnabled())
      {
         vk::DebugUtilsLabelEXT label(labelName);
         commandBuffer.insertDebugUtilsLabelEXT(label, GraphicsContext::GetDynamicLoader());
      }
   }

   ScopedCommandBufferLabel::ScopedCommandBufferLabel(vk::CommandBuffer commandBuffer_, const char* labelName, const std::array<float, 4>& color /*= {}*/)
      : commandBuffer(commandBuffer_)
   {
      if (areLabelsEnabled())
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
#endif // FORGE_WITH_DEBUG_UTILS
