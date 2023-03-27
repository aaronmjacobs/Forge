#include "Scene/Systems/OscillatingMovementSystem.h"

#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/OscillatingMovementComponent.h"
#include "Scene/Scene.h"

namespace
{
   glm::vec3 evaluateOscillation(const OscillatingMovementFunctions& functions, float time)
   {
      glm::vec3 result = glm::vec3(0.0f);

      result += glm::sin(functions.sin.timeScale * time + functions.sin.timeOffset) * functions.sin.valueScale;
      result += glm::cos(functions.cos.timeScale * time + functions.cos.timeOffset) * functions.cos.valueScale;

      return result;
   }
}

OscillatingMovementSystem::OscillatingMovementSystem(Scene& owningScene)
   : System(owningScene)
{
}

void OscillatingMovementSystem::tick(float dt)
{
   float currentTime = scene.getTime();

   scene.forEach<TransformComponent, OscillatingMovementComponent>([this, currentTime](TransformComponent& transformComponent, const OscillatingMovementComponent& oscillatingMovementComponent)
   {
      glm::quat lastRotation = glm::quat(glm::radians(evaluateOscillation(oscillatingMovementComponent.rotation, lastTime)));
      glm::quat currentRotation = glm::quat(glm::radians(evaluateOscillation(oscillatingMovementComponent.rotation, currentTime)));
      glm::quat rotationDiff = currentRotation * glm::inverse(lastRotation);
      transformComponent.transform.rotateBy(rotationDiff);

      glm::vec3 lastLocation = evaluateOscillation(oscillatingMovementComponent.location, lastTime);
      glm::vec3 currentLocation = evaluateOscillation(oscillatingMovementComponent.location, currentTime);
      glm::vec3 locationDiff = currentLocation - lastLocation;
      transformComponent.transform.translateBy(locationDiff);
   });

   lastTime = currentTime;
}
