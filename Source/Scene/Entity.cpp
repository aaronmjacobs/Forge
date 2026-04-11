#include "Scene/Entity.h"

#include "Core/Hash.h"

namespace std
{
   size_t hash<Entity>::operator()(const Entity& entity) const
   {
      return Hash::of(entity.scene, entity.id);
   }
}
