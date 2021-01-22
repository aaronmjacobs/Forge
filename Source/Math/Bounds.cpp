#include "Math/Bounds.h"

#include "Core/Assert.h"

Bounds::Bounds(const glm::vec3& centerPosition, const glm::vec3& extentVector)
   : center(centerPosition)
   , extent(extentVector)
   , radius(glm::length(extentVector))
{
}

Bounds::Bounds(std::span<const glm::vec3> points)
{
   ASSERT(points.size() > 0);

   glm::vec3 min = points[0];
   glm::vec3 max = points[0];

   for (const glm::vec3& point : points)
   {
      min = glm::min(min, point);
      max = glm::max(max, point);
   }

   center = (min + max) * 0.5f;
   extent = glm::abs(max - min) * 0.5f;
   radius = glm::length(extent);
}

void Bounds::setExtent(const glm::vec3& extentVector)
{
   extent = extentVector;
   radius = glm::length(extent);
}
