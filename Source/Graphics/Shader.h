#pragma once

#include "Graphics/GraphicsResource.h"

#include <span>
#include <vector>

class ResourceManager;

template<typename T>
class SpecializationInfo
{
public:
   SpecializationInfo(const std::vector<vk::SpecializationMapEntry>& mapEntries_, const std::vector<T>& permutations_)
      : mapEntries(mapEntries_)
      , permutations(permutations_)
   {
      info.reserve(permutations.size());
      for (const T& permutation : permutations)
      {
         info.push_back(vk::SpecializationInfo().setMapEntries(mapEntries).template setData<T>(permutation));
      }
   }

   SpecializationInfo(const SpecializationInfo& other) = delete;
   SpecializationInfo(SpecializationInfo&& other) = delete;

   SpecializationInfo& operator=(const SpecializationInfo& other) = delete;
   SpecializationInfo& operator=(SpecializationInfo&& other) = delete;

   std::span<const vk::SpecializationInfo> getInfo() const
   {
      return info;
   }

private:
   std::vector<vk::SpecializationMapEntry> mapEntries;
   std::vector<T> permutations;
   std::vector<vk::SpecializationInfo> info;
};

template<typename T>
class SpecializationInfoBuilder
{
public:
   template<typename MemberType>
   void registerMember(MemberType T::* member)
   {
      T dummyValue;
      const uint8_t* structAddress = reinterpret_cast<const uint8_t*>(&dummyValue);
      const uint8_t* memberAddress = reinterpret_cast<const uint8_t*>(&(dummyValue.*member));
      uint32_t offset = static_cast<uint32_t>(memberAddress - structAddress);

      mapEntries.push_back(vk::SpecializationMapEntry().setConstantID(constantCount).setOffset(offset).setSize(sizeof(MemberType)));
      ++constantCount;
   }

   void addPermutation(const T& value)
   {
      uint32_t index = value.getIndex();
      if (index >= permutations.size())
      {
         permutations.resize(index + 1);
      }
      permutations[index] = value;
   }

   SpecializationInfo<T> build() const
   {
      return SpecializationInfo(mapEntries, permutations);
   }

private:
   std::vector<vk::SpecializationMapEntry> mapEntries;
   std::vector<T> permutations;
   uint32_t constantCount = 0;
};

class Shader : public GraphicsResource
{
public:
   struct InitializationInfo
   {
      const char* vertShaderModulePath = nullptr;
      const char* fragShaderModulePath = nullptr;

      const char* vertShaderModuleEntryPoint = "main";
      const char* fragShaderModuleEntryPoint = "main";

      std::span<const vk::SpecializationInfo> specializationInfo;
   };

   Shader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const InitializationInfo& info);

protected:
   std::vector<vk::PipelineShaderStageCreateInfo> getStagesForPermutation(uint32_t permutationIndex) const;

private:
   std::vector<vk::PipelineShaderStageCreateInfo> vertStageCreateInfo;
   std::vector<vk::PipelineShaderStageCreateInfo> fragStageCreateInfo;
};
