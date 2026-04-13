#pragma once

#include "Core/Log.h"

#include "Graphics/SpecializationInfo.h"

#include <cstring>
#include <type_traits>
#include <utility>

template<typename T>
class ShaderPermutationManager
{
public:
   void registerMember(const VkBool32 T::* member)
   {
      registerMemberInternal(member, static_cast<VkBool32>(0), static_cast<VkBool32>(1));
   }

   template<typename MemberType>
   void registerMember(const MemberType T::* member, MemberType min, MemberType max) requires (std::is_integral_v<MemberType> || std::is_enum_v<MemberType>)
   {
      registerMemberInternal(member, min, max);
   }

   SpecializationInfo<T> buildSpecializationInfo() const
   {
      int32_t numPermutations = 1;
      for (const ValueRange& valueRange : valueRanges)
      {
         int32_t numValues = (valueRange.max - valueRange.min) + 1;
         numPermutations *= numValues;
      }

      std::vector<T> permutations(numPermutations);
      buildPermutations(permutations, 0);

#if FORGE_WITH_DEBUG_UTILS
      for (uint32_t i = 0; i < permutations.size(); ++i)
      {
         ASSERT(i == getIndex(permutations[i]), "Permutation index calculation is incorrect - expected %u, got %u", i, getIndex(permutations[i]));
      }
#endif // FORGE_WITH_DEBUG_UTILS

      LOG_INFO("Created " << permutations.size() << " permutations of " << typeid(T).name());

      return SpecializationInfo(mapEntries, std::move(permutations));
   }

   uint32_t getIndex(const T& permutation) const
   {
      int32_t index = 0;

      int32_t multiplier = 1;
      for (std::size_t i = mapEntries.size(); i > 0; --i)
      {
         std::size_t memberIndex = i - 1;

         uint32_t memberOffset = mapEntries[memberIndex].offset;
         std::size_t memberSize = mapEntries[memberIndex].size;

         int32_t value = 0;
         std::memcpy(&value, reinterpret_cast<const char*>(&permutation) + memberOffset, memberSize);

         index += value * multiplier;

         const ValueRange& memberValueRange = valueRanges[memberIndex];
         int32_t numValues = (memberValueRange.max - memberValueRange.min) + 1;
         multiplier *= numValues;
      }

      ASSERT(index >= 0);
      return static_cast<uint32_t>(index);
   }

private:
   template<typename MemberType>
   void registerMemberInternal(const MemberType T::* member, MemberType min, MemberType max)
   {
      static_assert(sizeof(MemberType) <= sizeof(int32_t), "Specialization constants must not be larger than int32_t");

      ASSERT(min < max);

      const T dummyValue;
      const uint8_t* structAddress = reinterpret_cast<const uint8_t*>(&dummyValue);
      const uint8_t* memberAddress = reinterpret_cast<const uint8_t*>(&(dummyValue.*member));
      uint32_t offset = static_cast<uint32_t>(memberAddress - structAddress);

      mapEntries.push_back(vk::SpecializationMapEntry().setConstantID(static_cast<uint32_t>(mapEntries.size())).setOffset(offset).setSize(sizeof(MemberType)));
      valueRanges.push_back(ValueRange(min, max));
   }

   void buildPermutations(std::span<T> permutations, int32_t memberIndex) const
   {
      ASSERT(mapEntries.size() == valueRanges.size());

      uint32_t memberOffset = mapEntries[memberIndex].offset;
      std::size_t memberSize = mapEntries[memberIndex].size;
      const ValueRange& memberValueRange = valueRanges[memberIndex];

      int32_t numValues = (memberValueRange.max - memberValueRange.min) + 1;
      std::size_t permutationsPerValue = permutations.size() / numValues;
      ASSERT(permutations.size() % numValues == 0);

      for (int32_t valueIndex = 0; valueIndex < numValues; ++valueIndex)
      {
         int32_t value = memberValueRange.min + valueIndex;

         for (std::size_t permutationIndex = permutationsPerValue * valueIndex; permutationIndex < permutationsPerValue * (valueIndex + 1); ++permutationIndex)
         {
            T& permutation = permutations[permutationIndex];
            std::memcpy(reinterpret_cast<char*>(&permutation) + memberOffset, &value, memberSize);
         }
      }

      int32_t nextMember = memberIndex + 1;
      if (nextMember < mapEntries.size())
      {
         for (int32_t valueIndex = 0; valueIndex < numValues; ++valueIndex)
         {
            buildPermutations(permutations.subspan(permutationsPerValue * valueIndex, permutationsPerValue), nextMember);
         }
      }
   }

   struct ValueRange
   {
      int32_t min = 0;
      int32_t max = 0;

      ValueRange(int32_t minValue, int32_t maxValue)
         : min(minValue)
         , max(maxValue)
      {}

      template<typename U>
      ValueRange(U minValue, U maxValue) requires std::is_enum_v<U>
         : ValueRange(static_cast<int32_t>(minValue), static_cast<int32_t>(maxValue))
      {}
   };

   std::vector<vk::SpecializationMapEntry> mapEntries;
   std::vector<ValueRange> valueRanges;
};

template<>
class ShaderPermutationManager<void>
{
};
