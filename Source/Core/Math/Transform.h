#pragma once

#include "Core/Math/MathUtils.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

struct Transform
{
   glm::quat orientation = glm::identity<glm::quat>();
   glm::vec3 position = glm::vec3(0.0f);
   glm::vec3 scale = glm::vec3(1.0f);

   Transform() = default;

   Transform(const glm::quat& initialOrientation, const glm::vec3& initialPosition, const glm::vec3& initialScale)
      : orientation(initialOrientation)
      , position(initialPosition)
      , scale(initialScale)
   {
   }

   glm::mat4 toMatrix() const
   {
      return glm::translate(position) * glm::toMat4(orientation) * glm::scale(scale);
   }

   glm::vec3 transformPosition(const glm::vec3& pos) const
   {
      return orientation * (scale * pos) + position;
   }

   glm::vec3 transformVector(const glm::vec3& vector) const
   {
      return orientation * (scale * vector);
   }

   glm::vec3 rotateVector(const glm::vec3& vector) const
   {
      return orientation * vector;
   }

   Transform inverse() const;
   Transform relativeTo(const Transform& other) const;

   void operator*=(const Transform& other);
   Transform operator*(const Transform& other) const;
};
