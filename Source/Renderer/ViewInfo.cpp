#include "Renderer/ViewInfo.h"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
   glm::vec3 computeViewDirection(const ViewInfo& viewInfo)
   {
      if (viewInfo.projectionMode == ProjectionMode::Perspective)
      {
         switch (viewInfo.perspectiveInfo.direction)
         {
         case PerspectiveDirection::Forward:
            return MathUtils::kForwardVector;
         case PerspectiveDirection::Backward:
            return -MathUtils::kForwardVector;
         case PerspectiveDirection::Right:
            return MathUtils::kRightVector;
         case PerspectiveDirection::Left:
            return -MathUtils::kRightVector;
         case PerspectiveDirection::Up:
            return MathUtils::kUpVector;
         case PerspectiveDirection::Down:
            return -MathUtils::kUpVector;
         default:
            break;
         }
      }

      return viewInfo.transform.getForwardVector();
   }

   glm::mat4 computeViewToClip(const ViewInfo& viewInfo)
   {
      glm::mat4 viewToClip;

      if (viewInfo.projectionMode == ProjectionMode::Orthographic)
      {
         viewToClip = glm::ortho(-viewInfo.orthographicInfo.width, viewInfo.orthographicInfo.width, -viewInfo.orthographicInfo.height, viewInfo.orthographicInfo.height, -viewInfo.orthographicInfo.depth, viewInfo.orthographicInfo.depth);
      }
      else
      {
         viewToClip = glm::perspective(glm::radians(viewInfo.perspectiveInfo.fieldOfView), viewInfo.perspectiveInfo.aspectRatio, viewInfo.perspectiveInfo.nearPlane, viewInfo.perspectiveInfo.farPlane);
      }

      viewToClip[1][1] *= -1.0f; // In Vulkan, the clip space Y coordinates are inverted compared to OpenGL (which GLM was written for), so we flip the sign here

      return viewToClip;
   }
}

ViewMatrices::ViewMatrices(const ViewInfo& viewInfo)
{
   viewPosition = viewInfo.transform.position;
   viewDirection = computeViewDirection(viewInfo);

   glm::vec3 upVector = glm::all(glm::epsilonEqual(viewDirection, MathUtils::kUpVector, MathUtils::kKindaSmallNumber)) ? -MathUtils::kForwardVector : MathUtils::kUpVector;
   worldToView = glm::lookAt(viewPosition, viewPosition + viewDirection, upVector);
   viewToClip = computeViewToClip(viewInfo);
   worldToClip = viewToClip * worldToView;
}
