#pragma once

#include "Core/Assert.h"
#include "Core/Hash.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

class DelegateHandle
{
public:
   static DelegateHandle create();

   bool isValid() const
   {
      return id != 0;
   }

   void invalidate()
   {
      id = 0;
   }

   friend bool operator==(const DelegateHandle& first, const DelegateHandle& second) = default;

   std::size_t hash() const
   {
      return Hash::of(id);
   }

private:
   static uint64_t counter;

   friend struct std::hash<DelegateHandle>;

   uint64_t id = 0;
};

USE_MEMBER_HASH_FUNCTION(DelegateHandle);

template<typename RetType, typename... Params>
class Delegate
{
public:
   using ReturnType = RetType;
   using FuncType = std::function<ReturnType(Params...)>;

   DelegateHandle bind(FuncType&& func)
   {
      function = std::move(func);
      handle = DelegateHandle::create();

      return handle;
   }

   void unbind()
   {
      function = nullptr;
      handle.invalidate();
   }

   bool isBound() const
   {
      return !!function;
   }

   ReturnType execute(Params... params) const
   {
      ASSERT(isBound());
      return function(std::forward<Params>(params)...);
   }

   void executeIfBound(Params... params) const requires std::is_void_v<RetType>
   {
      if (isBound())
      {
         execute(std::forward<Params>(params)...);
      }
   }

   DelegateHandle getHandle() const
   {
      return handle;
   }

private:
   FuncType function;
   DelegateHandle handle;
};

template<typename RetType, typename... Params>
class MulticastDelegate
{
public:
   using ReturnType = RetType;
   using DelegateType = Delegate<ReturnType, Params...>;
   using FuncType = typename DelegateType::FuncType;

   DelegateHandle add(FuncType&& function)
   {
      DelegateType& newDelegate = delegates.emplace_back();
      return newDelegate.bind(std::move(function));
   }

   void remove(DelegateHandle handle)
   {
      delegates.erase(std::remove_if(delegates.begin(), delegates.end(), [handle](const DelegateType& delegate) { return delegate.getHandle() == handle; }), delegates.end());
   }

   void clear()
   {
      delegates.clear();
   }

   bool isBound() const
   {
      return !delegates.empty();
   }

   void broadcast(Params... params) const
   {
      for (const DelegateType& delegate : delegates)
      {
         delegate.execute(std::forward<Params>(params)...);
      }
   }

   std::vector<ReturnType> broadcastWithReturn(Params... params) const
   {
      std::vector<ReturnType> returnValues(delegates.size());

      for (std::size_t i = 0; i < delegates.size(); ++i)
      {
         returnValues[i] = delegates[i].execute(std::forward<Params>(params)...);
      }

      return returnValues;
   }

private:
   std::vector<DelegateType> delegates;
};
