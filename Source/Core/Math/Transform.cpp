#include "Core/Math/Transform.h"

Transform Transform::inverse() const
{
   glm::quat inverseOrientation = glm::inverse(orientation);
   glm::vec3 inverseScale = MathUtils::safeReciprocal(scale);
   glm::vec3 inversePosition = inverseOrientation * (inverseScale * -position);

   return Transform(inverseOrientation, inversePosition, inverseScale);
}

Transform Transform::relativeTo(const Transform& other) const
{
   glm::quat otherInverseOrientation = glm::inverse(other.orientation);
   glm::vec3 otherInverseScale = MathUtils::safeReciprocal(other.scale);

   glm::quat relativeOrientation = otherInverseOrientation * orientation;
   glm::vec3 relativeScale = otherInverseScale * scale;
   glm::vec3 relativePosition = otherInverseOrientation * (position - other.position) * otherInverseScale;

   return Transform(relativeOrientation, relativePosition, relativeScale);
}

void Transform::operator*=(const Transform& other)
{
   position = transformPosition(other.position);
   orientation *= other.orientation;
   scale *= other.scale;
}

Transform Transform::operator*(const Transform& other) const
{
   Transform result = *this;
   result *= other;
   return result;
}
