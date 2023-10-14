#include "Scene/Scene.h"

#if FORGE_WITH_MIDI
#include "Platform/Midi.h"
#endif // FORGE_WITH_MIDI

#include "Scene/Entity.h"
#include "Scene/System.h"

#include <algorithm>
#include <utility>

Scene::~Scene()
{
   systemsByType.clear();
   systems.clear();
}

DelegateHandle Scene::addTickDelegate(TickDelegate::FuncType&& function)
{
   return tickDelegate.add(std::move(function));
}

void Scene::removeTickDelegate(DelegateHandle handle)
{
   tickDelegate.remove(handle);
}

void Scene::tick(float dt)
{
   float scaledDt = dt * timeScale;

   time += scaledDt;
   deltaTime = scaledDt;

   rawTime += dt;
   rawDeltaTime = dt;

   for (std::unique_ptr<System>& system : systems)
   {
      system->tick(scaledDt);
   }

   tickDelegate.broadcast(scaledDt);
}

void Scene::storeSystem(std::unique_ptr<System> system, std::type_index typeIndex)
{
   ASSERT(system);

   ASSERT(!systemsByType.contains(typeIndex));
   systemsByType.emplace(typeIndex, system.get());

   auto location = std::upper_bound(systems.begin(), systems.end(), system, [](const std::unique_ptr<System>& first, const std::unique_ptr<System>& second)
   {
      ASSERT(first && second);
      return first->getPriority() >= second->getPriority();
   });
   systems.insert(location, std::move(system));
}

System* Scene::getSystem(std::type_index typeIndex)
{
   auto location = systemsByType.find(typeIndex);
   if (location == systemsByType.end())
   {
      return nullptr;
   }

   ASSERT(location->second);
   System& system = *location->second;

   ASSERT(typeid(system) == typeIndex);
   return &system;
}

const System* Scene::getSystem(std::type_index typeIndex) const
{
   auto location = systemsByType.find(typeIndex);
   if (location == systemsByType.end())
   {
      return nullptr;
   }

   ASSERT(location->second);
   System& system = *location->second;

   ASSERT(typeid(system) == typeIndex);
   return &system;
}

Entity Scene::createEntity()
{
   return Entity(this, registry.create());
}

void Scene::destroyEntity(Entity entity)
{
   registry.destroy(entity.id);
}

Entity Scene::getEntity(std::size_t index)
{
   return Entity(this, registry.storage<entt::entity>().data()[index]);
}
