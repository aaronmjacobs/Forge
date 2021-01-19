#include "Scene/Scene.h"

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
   time += dt;
   deltaTime = dt;

   tickDelegate.broadcast(dt);
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
