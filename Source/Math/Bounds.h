#pragma once

#include <glm/glm.hpp>

#include <span>

class Bounds
{
public:
   Bounds() = default;

   Bounds(const glm::vec3& centerPosition, const glm::vec3& extentVector);
   Bounds(std::span<const glm::vec3> points);

   const glm::vec3& getCenter() const
   {
      return center;
   }

   const glm::vec3& getExtent() const
   {
      return extent;
   }

   float getRadius() const
   {
      return radius;
   }

   glm::vec3 getMin() const
   {
      return center - extent;
   }

   glm::vec3 getMax() const
   {
      return center + extent;
   }

   void setCenter(const glm::vec3& centerPosition)
   {
      center = centerPosition;
   }

   void setExtent(const glm::vec3& extentVector);

private:
   glm::vec3 center = glm::vec3(0.0f);
   glm::vec3 extent = glm::vec3(0.0f);
   float radius = 0.0f;
};
