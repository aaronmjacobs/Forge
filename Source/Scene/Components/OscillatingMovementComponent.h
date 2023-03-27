#pragma once

#include <glm/glm.hpp>

struct OscillatingMovementValues
{
   glm::vec3 timeOffset = glm::vec3(0.0f);
   glm::vec3 timeScale = glm::vec3(0.0f);
   glm::vec3 valueScale = glm::vec3(0.0f);
};

struct OscillatingMovementFunctions
{
   OscillatingMovementValues sin;
   OscillatingMovementValues cos;
};

struct OscillatingMovementComponent
{
   OscillatingMovementFunctions location;
   OscillatingMovementFunctions rotation;
};
