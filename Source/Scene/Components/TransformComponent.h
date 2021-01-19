#pragma once

#include "Math/Transform.h"

#include "Scene/Entity.h"

struct TransformComponent
{
   Transform transform;
   Entity parent;

   Transform getAbsoluteTransform() const;
   void setAbsoluteTransform(const Transform& absoluteTransform);

   TransformComponent* getParentComponent();
   const TransformComponent* getParentComponent() const;
};
