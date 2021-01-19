#pragma once

#include "Core/Delegate.h"

#include <entt/entity/registry.hpp>

#include <memory>

class Entity;

class Scene
{
public:
   using TickDelegate = MulticastDelegate<void, float /* dt */>;

   Scene();

   DelegateHandle addTickDelegate(TickDelegate::FuncType&& function);
   void removeTickDelegate(DelegateHandle handle);

   void tick(float dt);

   float getTime() const
   {
      return time;
   }

   float getDeltaTime() const
   {
      return deltaTime;
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

   Entity getActiveCamera() const;
   void setActiveCamera(Entity newActiveCamera);

private:
   friend class Entity;

   entt::registry registry;

   TickDelegate tickDelegate;
   float time = 0.0f;
   float deltaTime = 0.0f;

   std::unique_ptr<Entity> activeCamera;
};
