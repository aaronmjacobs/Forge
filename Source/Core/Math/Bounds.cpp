#include "Core/Math/Bounds.h"

#include "Core/Assert.h"

// static
Bounds Bounds::fromPoints(std::span<const glm::vec3> points)
{
   ASSERT(points.size() > 0);

   glm::vec3 min = points[0];
   glm::vec3 max = points[0];

   for (const glm::vec3& point : points)
   {
      min = glm::min(min, point);
      max = glm::max(max, point);
   }

   Bounds bounds;
   bounds.center = (min + max) * 0.5f;
   bounds.extent = glm::abs(max - min) * 0.5f;
   bounds.radius = glm::length(bounds.extent);

   return bounds;
}
