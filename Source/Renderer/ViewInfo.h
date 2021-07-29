#pragma once

#include "Math/Transform.h"

enum class ProjectionMode
{
   Orthographic,
   Perspective
};

enum class PerspectiveDirection
{
   FromTransform,

   Forward,
   Backward,
   Right,
   Left,
   Up,
   Down
};

struct OrthographicInfo
{
   float width = 10.0f;
   float height = 10.0f;
   float depth = 10.0f;
};

struct PerspectiveInfo
{
   PerspectiveDirection direction = PerspectiveDirection::FromTransform;
   float fieldOfView = 70.0f;
   float aspectRatio = 1.0f;
   float nearPlane = 0.1f;
   float farPlane = 100.0f;
};

struct ViewInfo
{
   Transform transform;

   ProjectionMode projectionMode = ProjectionMode::Perspective;
   OrthographicInfo orthographicInfo;
   PerspectiveInfo perspectiveInfo;
};
