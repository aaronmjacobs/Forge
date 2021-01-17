#include "Scene/Components/TransformComponent.h"

Transform TransformComponent::getAbsoluteTransform() const
{
   if (const TransformComponent* parentComponent = getParentComponent())
   {
      return parentComponent->getAbsoluteTransform() * transform;
   }

   return transform;
}

void TransformComponent::setAbsoluteTransform(const Transform& absoluteTransform)
{
   if (const TransformComponent* parentComponent = getParentComponent())
   {
      transform = absoluteTransform.relativeTo(parentComponent->getAbsoluteTransform());
   }
   else
   {
      transform = absoluteTransform;
   }
}

TransformComponent* TransformComponent::getParentComponent()
{
   return parent ? parent.tryGetComponent<TransformComponent>() : nullptr;
}

const TransformComponent* TransformComponent::getParentComponent() const
{
   return parent ? parent.tryGetComponent<TransformComponent>() : nullptr;
}
