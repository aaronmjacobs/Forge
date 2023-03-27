#pragma once

#include "Core/Delegate.h"

#include "Scene/Entity.h"
#include "Scene/System.h"

#include <glm/glm.hpp>

#include <vector>

class InputManager;

namespace CameraSystemInputActions
{
   static constexpr const char* const kMoveForward = "MoveForward";
   static constexpr const char* const kMoveRight = "MoveRight";
   static constexpr const char* const kMoveUp = "MoveUp";

   static constexpr const char* const kLookRight = "LookRight";
   static constexpr const char* const kLookUp = "LookUp";
}

class CameraSystem : public System
{
public:
   CameraSystem(Scene& owningScene, InputManager& inputMan);
   ~CameraSystem();

   int getPriority() const override
   {
      return -10;
   }

   void tick(float dt) override;

   Entity getActiveCamera() const
   {
      return activeCameraEntity;
   }

   void setActiveCamera(Entity activeCamera);

private:
   Entity activeCameraEntity;
   InputManager& inputManager;

   std::vector<DelegateHandle> axisMappingHandles;

   float moveSpeed = 3.0f;
   float lookSpeed = 180.0f;

   glm::vec3 moveInput = glm::vec3(0.0f);
   glm::vec2 lookInput = glm::vec2(0.0f);
};
