#pragma once

#include "Core/Assert.h"

#include "Scene/Scene.h"

class Entity
{
public:
   Entity() = default;

   void destroy()
   {
      ASSERT(scene);
      scene->destroyEntity(*this);
   }

   template<typename T, typename... Args>
   T& createComponent(Args&&... args)
   {
      ASSERT(!hasComponent<T>());
      return scene->registry.emplace<T>(id, std::forward<Args>(args)...);
   }

   template<typename T>
   void destroyComponent()
   {
      ASSERT(hasComponent<T>());
      scene->registry.remove<T>(id);
   }

   template<typename T>
   T& getComponent()
   {
      ASSERT(hasComponent<T>());
      return scene->registry.get<T>(id);
   }

   template<typename T>
   const T& getComponent() const
   {
      ASSERT(hasComponent<T>());
      return scene->registry.get<T>(id);
   }

   template<typename T>
   T* tryGetComponent()
   {
      ASSERT(scene);
      return scene->registry.try_get<T>(id);
   }

   template<typename T>
   const T* tryGetComponent() const
   {
      ASSERT(scene);
      return scene->registry.try_get<T>(id);
   }

   template<typename T>
   bool hasComponent() const
   {
      ASSERT(scene);
      return scene->registry.all_of<T>(id);
   }

   explicit operator bool() const
   {
      return scene && scene->registry.valid(id);
   }

   bool operator==(const Entity& other) const
   {
      return scene == other.scene && id == other.id;
   }

private:
   friend class Scene;
   friend struct std::hash<Entity>;

   Entity(Scene* owningScene, entt::entity entityId)
      : scene(owningScene)
      , id(entityId)
   {
   }

   Scene* scene = nullptr;
   entt::entity id = entt::null;
};

namespace std
{
   template<>
   struct hash<Entity>
   {
      size_t operator()(const Entity& entity) const;
   };
}
