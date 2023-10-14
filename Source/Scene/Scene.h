#pragma once

#include "Core/Delegate.h"

#include "Scene/System.h"

#include <entt/entity/registry.hpp>

#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

class Entity;

class Scene
{
public:
   using TickDelegate = MulticastDelegate<void, float /* dt */>;

   ~Scene();

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
      return registry.view<ComponentTypes...>().each(std::forward<Function>(function));
   }

   template<typename... ComponentTypes, typename Function>
   void forEach(Function&& function) const
   {
      return registry.view<const ComponentTypes...>().each(std::forward<Function>(function));
   }

   template<typename T, typename... Params>
   T* createSystem(Params&&... params)
   {
      storeSystem(std::make_unique<T>(*this, std::forward<Params>(params)...), typeid(T));
      return getSystem<T>();
   }

   template<typename T>
   T* getSystem()
   {
      return static_cast<T*>(getSystem(typeid(T)));
   }

   template<typename T>
   const T* getSystem() const
   {
      return static_cast<const T*>(getSystem(typeid(T)));
   }

private:
   friend class Entity;

   void storeSystem(std::unique_ptr<System> system, std::type_index typeIndex);
   System* getSystem(std::type_index typeIndex);
   const System* getSystem(std::type_index typeIndex) const;

   Entity getEntity(std::size_t index);

   entt::registry registry;

   std::vector<std::unique_ptr<System>> systems;
   std::unordered_map<std::type_index, System*> systemsByType;

   TickDelegate tickDelegate;

   float timeScale = 1.0f;

   float time = 0.0f;
   float deltaTime = 0.0f;

   float rawTime = 0.0f;
   float rawDeltaTime = 0.0f;
};
