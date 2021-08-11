#include "Renderer/ViewInfo.h"

#include "Core/Assert.h"

#include <glm/gtc/matrix_transform.hpp>

namespace
{
   glm::mat4 computeViewToClip(const ViewInfo& viewInfo, bool flipY)
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

      if (flipY)
      {
         viewToClip[1][1] *= -1.0f; // In Vulkan, the clip space Y coordinates are inverted compared to OpenGL (which GLM was written for), so we flip the sign here
      }

      return viewToClip;
   }

   glm::vec3 getCubeFaceForward(CubeFace cubeFace)
   {
      switch (cubeFace)
      {
      case CubeFace::PositiveX:
         return glm::vec3(1.0f, 0.0f, 0.0f);
      case CubeFace::NegativeX:
         return glm::vec3(-1.0f, 0.0f, 0.0f);
      case CubeFace::PositiveY:
         return glm::vec3(0.0f, 1.0f, 0.0f);
      case CubeFace::NegativeY:
         return glm::vec3(0.0f, -1.0f, 0.0f);
      case CubeFace::PositiveZ:
         return glm::vec3(0.0f, 0.0f, 1.0f);
      case CubeFace::NegativeZ:
         return glm::vec3(0.0f, 0.0f, -1.0f);
      default:
         ASSERT(false);
         return glm::vec3(0.0f);
      }
   }

   glm::vec3 getCubeFaceUp(CubeFace cubeFace)
   {
      switch (cubeFace)
      {
      case CubeFace::PositiveX:
         return glm::vec3(0.0f, -1.0f, 0.0f);
      case CubeFace::NegativeX:
         return glm::vec3(0.0f, -1.0f, 0.0f);
      case CubeFace::PositiveY:
         return glm::vec3(0.0f, 0.0f, 1.0f);
      case CubeFace::NegativeY:
         return glm::vec3(0.0f, 0.0f, -1.0f);
      case CubeFace::PositiveZ:
         return glm::vec3(0.0f, -1.0f, 0.0f);
      case CubeFace::NegativeZ:
         return glm::vec3(0.0f, -1.0f, 0.0f);
      default:
         ASSERT(false);
         return glm::vec3(0.0f);
      }
   }
}

ViewMatrices::ViewMatrices(const ViewInfo& viewInfo)
{
   glm::vec3 forward;
   glm::vec3 up;
   bool flipY = true;
   if (viewInfo.cubeFace.has_value())
   {
      forward = getCubeFaceForward(viewInfo.cubeFace.value());
      up = getCubeFaceUp(viewInfo.cubeFace.value());
      flipY = false;
   }
   else
   {
      forward = viewInfo.transform.getForwardVector();
      up = viewInfo.transform.getUpVector();
      flipY = true;
   }

   viewPosition = viewInfo.transform.position;
   viewDirection = forward;

   worldToView = glm::lookAt(viewPosition, viewPosition + viewDirection, up);
   viewToClip = computeViewToClip(viewInfo, flipY);
   worldToClip = viewToClip * worldToView;

   if (viewInfo.projectionMode == ProjectionMode::Orthographic)
   {
      nearFar = glm::vec2(-viewInfo.orthographicInfo.depth, viewInfo.orthographicInfo.depth);
   }
   else
   {
      nearFar = glm::vec2(viewInfo.perspectiveInfo.nearPlane, viewInfo.perspectiveInfo.farPlane);
   }
}
