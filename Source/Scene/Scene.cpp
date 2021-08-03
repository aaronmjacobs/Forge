#include "Scene/Scene.h"

#if FORGE_WITH_MIDI
#include "Platform/Midi.h"
#endif // FORGE_WITH_MIDI

#include "Scene/Entity.h"

#include <utility>

Scene::Scene()
{
   // Necessary to avoid circular inclusion issues
   activeCamera = std::make_unique<Entity>();
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
   float timeScale = 1.0f;
#if FORGE_WITH_MIDI
   timeScale = Midi::getState().groups[7].slider;
#endif // FORGE_WITH_MIDI
   float scaledDt = dt * timeScale;

   time += scaledDt;
   deltaTime = scaledDt;

   rawTime += dt;
   rawDeltaTime = dt;

   tickDelegate.broadcast(scaledDt);
}

Entity Scene::createEntity()
{
   return Entity(this, registry.create());
}

void Scene::destroyEntity(Entity entity)
{
   registry.destroy(entity.id);
}

Entity Scene::getActiveCamera() const
{
   return *activeCamera;
}

void Scene::setActiveCamera(Entity newActiveCamera)
{
   ASSERT(newActiveCamera.scene == nullptr || newActiveCamera.scene == this);
   *activeCamera = newActiveCamera;
}
