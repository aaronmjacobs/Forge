#pragma once

#include "Math/Transform.h"

#include <optional>

constexpr uint32_t kNumCubeFaces = 6;

enum class CubeFace
{
   PositiveX = 0,
   NegativeX = 1,
   PositiveY = 2,
   NegativeY = 3,
   PositiveZ = 4,
   NegativeZ = 5,
};

enum class ProjectionMode
{
   Orthographic,
   Perspective
};

struct OrthographicInfo
{
   float width = 10.0f;
   float height = 10.0f;
   float depth = 10.0f;
};

struct PerspectiveInfo
{
   float fieldOfView = 70.0f;
   float aspectRatio = 1.0f;
   float nearPlane = 0.1f;
   float farPlane = 100.0f;
};

struct ViewInfo
{
   Transform transform;
   std::optional<CubeFace> cubeFace;

   ProjectionMode projectionMode = ProjectionMode::Perspective;
   OrthographicInfo orthographicInfo;
   PerspectiveInfo perspectiveInfo;

   float depthBiasConstantFactor = 0.0f;
   float depthBiasSlopeFactor = 0.0f;
   float depthBiasClamp = 0.0f;
};

struct ViewMatrices
{
   glm::mat4 worldToView;
   glm::mat4 viewToClip;
   glm::mat4 worldToClip;

   glm::vec3 viewPosition;
   glm::vec3 viewDirection;
   glm::vec2 nearFar;

   ViewMatrices() = default;
   ViewMatrices(const ViewInfo& viewInfo);
};
