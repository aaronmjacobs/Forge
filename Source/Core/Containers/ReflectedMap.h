#pragma once

#include "Core/Assert.h"

#include <unordered_map>

template<typename A, typename B>
class ReflectedMap
{
public:
   void add(const A& a, const B& b)
   {
      aToB[a] = b;
      bToA[b] = a;
   }
   
   bool remove(const A& a)
   {
      auto aLocation = aToB.find(a);
      if (aLocation != aToB.end())
      {
         auto bLocation = bToA.find(aLocation->second);
         ASSERT(bLocation != bToA.end());
         
         bToA.erase(bLocation);
         aToB.erase(aLocation);
         
         return true;
      }
      
      return false;
   }
   
   bool remove(const B& b)
   {
      auto bLocation = bToA.find(b);
      if (bLocation != bToA.end())
      {
         auto aLocation = aToB.find(bLocation->second);
         ASSERT(aLocation != aToB.end());
         
         aToB.erase(aLocation);
         bToA.erase(bLocation);
         
         return true;
      }
      
      return false;
   }
   
   void clear()
   {
      aToB.clear();
      bToA.clear();
   }
   
   B* find(const A& a)
   {
      auto location = aToB.find(a);
      return location == aToB.end() ? nullptr : &location->second;
   }
   
   A* find(const B& b)
   {
      auto location = bToA.find(b);
      return location == bToA.end() ? nullptr : &location->second;
   }
   
   const B* find(const A& a) const
   {
      auto location = aToB.find(a);
      return location == aToB.end() ? nullptr : &location->second;
   }
   
   const A* find(const B& b) const
   {
      auto location = bToA.find(b);
      return location == bToA.end() ? nullptr : &location->second;
   }
   
private:
   std::unordered_map<A, B> aToB;
   std::unordered_map<B, A> bToA;
};
