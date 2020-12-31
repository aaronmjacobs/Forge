#pragma once

#include <glm/glm.hpp>

struct MeshUniformData
{
   alignas(16) glm::mat4 localToWorld;
};
