#pragma once

#include "Scene/Entity.h"

#include <array>

class GraphicsContext;
class ResourceManager;
class Scene;
struct RenderCapabilities;
struct RenderSettings;

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

   void render(const GraphicsContext& graphicsContext, Scene& scene, const RenderCapabilities& renderCapabilities, RenderSettings& settings, const ResourceManager& resourceManager);

private:
   void renderSceneWindow(const GraphicsContext& graphicsContext, Scene& scene, const RenderCapabilities& renderCapabilities, RenderSettings& settings, const ResourceManager& resourceManager);
   void renderTime(Scene& scene);
   void renderSettings(const GraphicsContext& graphicsContext, const RenderCapabilities& renderCapabilities, RenderSettings& settings);
   void renderEntityList(Scene& scene);
   void renderSelectedEntity(const ResourceManager& resourceManager);

   static bool visible;

   float timeUntilFrameRateUpdate = 0.0f;
   float maxFrameRate = 0.0f;
   uint32_t frameIndex = 0;
   std::array<float, 100> frameRates{};

   Entity selectedEntity;
};
