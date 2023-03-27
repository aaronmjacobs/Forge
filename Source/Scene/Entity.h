#pragma once

#include "Core/Assert.h"

#include "Scene/Scene.h"

class Entity
{
public:
    template<typename Function>
    static void forEach(Scene& scene, Function&& function)
    {
       for (std::size_t i = 0; i < scene.registry.size(); ++i)
       {
          function(scene.getEntity(i));
       }
    }

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
      return isValid() ? scene->registry.try_get<T>(id) : nullptr;
   }

   template<typename T>
   const T* tryGetComponent() const
   {
      return isValid() ? scene->registry.try_get<T>(id) : nullptr;
   }

   template<typename T>
   bool hasComponent() const
   {
      return isValid() ? scene->registry.all_of<T>(id) : false;
   }

   bool isInScene(Scene& queryScene) const
   {
      return scene == &queryScene;
   }

   bool isValid() const
   {
      return scene && scene->registry.valid(id);
   }

   explicit operator bool() const
   {
      return isValid();
   }

   bool operator==(const Entity& other) const = default;

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
