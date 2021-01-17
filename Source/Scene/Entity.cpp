#include "Scene/Entity.h"

#include "Core/Hash.h"

namespace std
{
   size_t hash<Entity>::operator()(const Entity& entity) const
   {
      size_t seed = std::hash<Scene*>{}(entity.scene);
      Hash::combine(seed, entity.id);

      return seed;
   }
}
