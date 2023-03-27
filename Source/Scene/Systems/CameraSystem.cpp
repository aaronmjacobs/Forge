#include "Scene/Systems/CameraSystem.h"

#include "Platform/InputManager.h"

#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"

CameraSystem::CameraSystem(Scene& owningScene, InputManager& inputMan)
   : System(owningScene)
   , inputManager(inputMan)
{
   axisMappingHandles.push_back(inputManager.bindAxisMapping(CameraSystemInputActions::kMoveForward, [this](float value) mutable
   {
      moveInput += MathUtils::kForwardVector * value;
   }));

   axisMappingHandles.push_back(inputManager.bindAxisMapping(CameraSystemInputActions::kMoveRight, [this](float value) mutable
   {
      moveInput += MathUtils::kRightVector * value;
   }));

   axisMappingHandles.push_back(inputManager.bindAxisMapping(CameraSystemInputActions::kMoveUp, [this](float value) mutable
   {
      moveInput += MathUtils::kUpVector * value;
   }));

   axisMappingHandles.push_back(inputManager.bindAxisMapping(CameraSystemInputActions::kLookRight, [this](float value) mutable
   {
      lookInput.x += value;
   }));

   axisMappingHandles.push_back(inputManager.bindAxisMapping(CameraSystemInputActions::kLookUp, [this](float value) mutable
   {
      lookInput.y += value;
   }));
}

CameraSystem::~CameraSystem()
{
   for (DelegateHandle axisMappingHandle : axisMappingHandles)
   {
      inputManager.unbindAxisMapping(axisMappingHandle);
   }
}

void CameraSystem::tick(float dt)
{
   TransformComponent* transformComponent = activeCameraEntity.tryGetComponent<TransformComponent>();
   if (transformComponent && activeCameraEntity.tryGetComponent<CameraComponent>())
   {
      Transform& transform = transformComponent->transform;

      glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.orientation));
      euler.x = glm::clamp(euler.x + lookInput.y * lookSpeed * 0.75f * scene.getRawDeltaTime(), -89.0f, 89.0f);
      euler.z -= lookInput.x * lookSpeed * scene.getRawDeltaTime();
      transform.orientation = glm::quat(glm::radians(euler));

      transform.translateBy(transform.rotateVector(moveInput) * moveSpeed * scene.getRawDeltaTime());
   }

   moveInput = glm::vec3(0.0f);
   lookInput = glm::vec2(0.0f);
}

void CameraSystem::setActiveCamera(Entity activeCamera)
{
   ASSERT(activeCamera.isInScene(scene) || !activeCamera.isValid());
   activeCameraEntity = activeCamera;
}
