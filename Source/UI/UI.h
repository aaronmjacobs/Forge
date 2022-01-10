#pragma once

#include "Scene/Entity.h"

#include <array>

class ResourceManager;
class Scene;

class UI
{
public:
   static void initialize(struct GLFWwindow* window, bool installCallbacks);
   static void terminate();

   static bool isVisible();
   static void setVisible(bool newVisible);

   static bool wantsMouseInput();
   static bool wantsKeyboardInput();
   static void setIgnoreMouse(bool ignore);

   void render(Scene& scene, const ResourceManager& resourceManager);

private:
   void renderSceneWindow(Scene& scene, const ResourceManager& resourceManager);
   void renderTime(Scene& scene);
   void renderEntityList(Scene& scene);
   void renderSelectedEntity(const ResourceManager& resourceManager);

   static bool visible;

   float timeUntilFrameRateUpdate = 0.0f;
   float maxFrameRate = 0.0f;
   uint32_t frameIndex = 0;
   std::array<float, 100> frameRates{};

   Entity selectedEntity;
};
