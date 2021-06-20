#pragma once

#include <glm/glm.hpp>

class LightComponent
{
public:
   const glm::vec3& getColor() const
   {
      return color;
   }

   void setColor(const glm::vec3& newColor)
   {
      color = glm::max(newColor, glm::vec3(0.0f));
   }

private:
   glm::vec3 color = glm::vec3(1.0f);
};

class DirectionalLightComponent : public LightComponent
{
};

class PointLightComponent : public LightComponent
{
public:
   float getRadius() const
   {
      return radius;
   }

   void setRadius(float newRadius)
   {
      radius = glm::max(newRadius, 0.0f);
   }

private:
   float radius = 10.0f;
};

class SpotLightComponent : public LightComponent
{
public:
   float getRadius() const
   {
      return radius;
   }

   void setRadius(float newRadius)
   {
      radius = glm::max(newRadius, 0.0f);
   }

   float getBeamAngle() const
   {
      return beamAngle;
   }

   void setBeamAngle(float newBeamAngle)
   {
      beamAngle = glm::clamp(newBeamAngle, 0.0f, cutoffAngle - 0.01f);
   }

   float getCutoffAngle() const
   {
      return cutoffAngle;
   }

   void setCutoffAngle(float newCutoffAngle)
   {
      cutoffAngle = glm::clamp(newCutoffAngle, beamAngle + 0.01f, 179.0f);
   }

private:
   float radius = 10.0f;
   float beamAngle = 30.0f;
   float cutoffAngle = 45.0f;
};
