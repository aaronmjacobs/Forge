#pragma once

#include "Core/Delegate.h"

#include <entt/entity/registry.hpp>

#include <future>
#include <memory>
#include <thread>
#include <vector>

class Entity;

class Scene
{
public:
   using TickDelegate = MulticastDelegate<void, float /* dt */>;

   Scene();

   DelegateHandle addTickDelegate(TickDelegate::FuncType&& function);
   void removeTickDelegate(DelegateHandle handle);

   void tick(float dt);

   float getTimeScale() const
   {
      return timeScale;
   }

   void setTimeScale(float newTimeScale)
   {
      timeScale = newTimeScale;
   }

   float getTime() const
   {
      return time;
   }

   float getDeltaTime() const
   {
      return deltaTime;
   }

   float getRawTime() const
   {
      return rawTime;
   }

   float getRawDeltaTime() const
   {
      return rawDeltaTime;
   }

   Entity createEntity();
   void destroyEntity(Entity entity);

   template<typename... ComponentTypes, typename Function>
   void forEach(Function&& function)
   {
      registry.view<ComponentTypes...>().each(std::forward<Function>(function));
   }

   template<typename... ComponentTypes, typename Function>
   void forEach(Function&& function) const
   {
      registry.view<const ComponentTypes...>().each(std::forward<Function>(function));
   }

   template<typename ResultType, typename... ComponentTypes, typename Function>
   std::vector<ResultType> collectParallel(Function&& function) const
   {
      std::vector<std::tuple<const ComponentTypes*...>> components;
      registry.view<const ComponentTypes...>().each([&components](const ComponentTypes&... comps) { components.push_back(std::make_tuple(&comps...)); });

      unsigned int numThreads = std::thread::hardware_concurrency();
      if (numThreads == 0)
      {
         numThreads = 1;
      }
      std::size_t groupsPerThread = components.size() / numThreads;
      std::size_t leftovers = components.size() % numThreads;

      std::size_t startIndex = 0;
      std::size_t count = 0;

      std::vector<std::future<std::vector<std::optional<ResultType>>>> futures;
      futures.reserve(components.size());
      for (std::size_t threadIndex = 0; threadIndex < numThreads; ++threadIndex)
      {
         std::size_t count = groupsPerThread + (leftovers > 0 ? 1 : 0);
         if (leftovers > 0)
         {
            --leftovers;
         }

         if (count == 0)
         {
            break;
         }

         futures.push_back(std::async([&function, &components, startIndex, count]()
         {
            std::vector<std::optional<ResultType>> localResults;
            localResults.reserve(count);

            for (std::size_t i = startIndex; i < startIndex + count; ++i)
            {
               localResults.push_back(std::apply(std::forward<Function>(function), components[i]));
            }

            return localResults;
         }));

         startIndex += count;
      }

      std::vector<ResultType> results;
      for (auto& future : futures)
      {
         std::vector<std::optional<ResultType>> localResults = future.get();
         for (const std::optional<ResultType>& result : localResults)
         {
            if (result.has_value())
            {
               results.push_back(std::move(result.value()));
            }
         }
      }

      return results;
   }

   Entity getActiveCamera() const;
   void setActiveCamera(Entity newActiveCamera);

private:
   friend class Entity;

   Entity getEntity(std::size_t index);

   entt::registry registry;

   TickDelegate tickDelegate;

   float timeScale = 1.0f;

   float time = 0.0f;
   float deltaTime = 0.0f;

   float rawTime = 0.0f;
   float rawDeltaTime = 0.0f;

   std::unique_ptr<Entity> activeCamera;
};
