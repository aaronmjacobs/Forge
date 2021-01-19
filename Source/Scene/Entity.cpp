#include "Scene/Entity.h"

#include "Core/Hash.h"

namespace std
{
   size_t hash<Entity>::operator()(const Entity& entity) const
   {
      size_t hash = 0;

      Hash::combine(hash, entity.scene);
      Hash::combine(hash, entity.id);

      return hash;
   }
}
