#pragma once

#include "Core/Assert.h"

#include "Graphics/Vulkan.h"

#include <span>
#include <utility>
#include <vector>

template<typename T>
class SpecializationInfo
{
public:
   SpecializationInfo() = default;

   SpecializationInfo(const std::vector<vk::SpecializationMapEntry>& mapEntries_, std::vector<T> permutations_)
      : mapEntries(mapEntries_)
      , permutations(std::move(permutations_))
   {
      info.reserve(permutations.size());
      for (const T& permutation : permutations)
      {
         info.push_back(vk::SpecializationInfo().setMapEntries(mapEntries).template setData<T>(permutation));
      }
   }

   SpecializationInfo(const SpecializationInfo& other) = delete;
   SpecializationInfo(SpecializationInfo&& other)
      : mapEntries(std::move(other.mapEntries))
      , permutations(std::move(other.permutations))
      , info(std::move(other.info))
   {
   }

   SpecializationInfo& operator=(const SpecializationInfo& other) = delete;
   SpecializationInfo& operator=(SpecializationInfo&& other)
   {
      mapEntries = std::move(other.mapEntries);
      permutations = std::move(other.permutations);
      info = std::move(other.info);

      return *this;
   }

   std::span<const vk::SpecializationInfo> getInfo() const
   {
      return info;
   }

private:
   std::vector<vk::SpecializationMapEntry> mapEntries;
   std::vector<T> permutations;
   std::vector<vk::SpecializationInfo> info;
};

template<>
class SpecializationInfo<void>
{
};
