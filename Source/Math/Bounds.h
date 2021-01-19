#pragma once

#include <glm/glm.hpp>

#include <span>

struct Bounds
{
   static Bounds fromPoints(std::span<const glm::vec3> points);

   glm::vec3 getMin() const
   {
      return center - extent;
   }

   glm::vec3 getMax() const
   {
      return center + extent;
   }

   glm::vec3 center = glm::vec3(0.0f);
   glm::vec3 extent = glm::vec3(0.0f);
   float radius = 0.0f;
};
