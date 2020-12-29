#pragma once

#include <glm/glm.hpp>

struct ViewUniformData
{
   alignas(16) glm::mat4 worldToClip;
};

struct MeshUniformData
{
   alignas(16) glm::mat4 localToWorld;
};
