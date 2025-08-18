#pragma once

#include <glm/glm.hpp>

#include <cmath>

namespace MathUtils
{
   constexpr float kSmallNumber = 1.e-8f;
   constexpr float kKindaSmallNumber = 1.e-4f;

   constexpr glm::vec3 kForwardVector = glm::vec3(0.0f, 1.0f, 0.0f);
   constexpr glm::vec3 kRightVector = glm::vec3(1.0f, 0.0f, 0.0f);
   constexpr glm::vec3 kUpVector = glm::vec3(0.0f, 0.0f, 1.0f);

   inline float safeReciprocal(float value, float tolerance = kSmallNumber)
   {
      if (std::fabsf(value) <= tolerance)
      {
         return 0.0f;
      }

      return 1.0f / value;
   }

   inline glm::vec3 safeReciprocal(const glm::vec3& value, float tolerance = kSmallNumber)
   {
      return glm::vec3(safeReciprocal(value.x, tolerance), safeReciprocal(value.y, tolerance), safeReciprocal(value.z, tolerance));
   }

   template<glm::length_t L, typename T, glm::qualifier Q>
	glm::vec<L, bool, Q> isFinite(glm::vec<L, T, Q> const& v)
	{
		glm::vec<L, bool, Q> Result(true);
		for(glm::length_t i = 0; i < L; ++i)
			Result[i] = std::isfinite(v[i]);
		return Result;
	}

   template<typename T>
   T safeNormal(const T& value)
   {
      T result = glm::normalize(value);
      if (!glm::all(isFinite(result)))
      {
         return T(0);
      }

      return result;
   }

   template<typename T>
   T saturate(const T& value)
   {
      return glm::clamp(value, T(0.0), T(1.0));
   }

   template<typename T>
   T square(const T& value)
   {
      return value * value;
   }
}
