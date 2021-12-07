#include "ForgeApplication.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Swapchain.h"

#if FORGE_WITH_MIDI
#  include "Platform/Midi.h"
#endif // FORGE_WITH_MIDI
#include "Platform/Window.h"

#include "Renderer/Renderer.h"

#include "Resources/ResourceManager.h"

#include "Scene/Entity.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"

#include <GLFW/glfw3.h>

#include <array>
#include <sstream>
#include <stdexcept>

namespace
{
   namespace InputActions
   {
      const char* kToggleFullscreen = "ToggleFullscreen";
      const char* kReleaseCursor = "ReleaseCursor";

      const char* kToggleMSAA = "ToggleMSAA";
      const char* kToggleLabels = "ToggleLabels";

      const char* kMoveForward = "MoveForward";
      const char* kMoveRight = "MoveRight";
      const char* kMoveUp = "MoveUp";

      const char* kLookRight = "LookRight";
      const char* kLookUp = "LookUp";
   }
}

ForgeApplication::ForgeApplication()
{
#if FORGE_WITH_MIDI
   Midi::initialize();
#endif // FORGE_WITH_MIDI

   initializeGlfw();
   initializeVulkan();
   initializeSwapchain();
   initializeRenderer();
   initializeCommandBuffers();
   initializeSyncObjects();

   loadScene();
}

ForgeApplication::~ForgeApplication()
{
   terminateSyncObjects();
   terminateCommandBuffers(false);
   terminateRenderer();
   terminateSwapchain();
   terminateVulkan();
   terminateGlfw();

#if FORGE_WITH_MIDI
   Midi::terminate();
#endif // FORGE_WITH_MIDI
}

void ForgeApplication::run()
{
   static const double kFrameRateReportInterval = 0.25;

   int frameCount = 0;
   double accumulatedTime = 0.0;

   double lastTime = glfwGetTime();
   while (!window->shouldClose())
   {
#if FORGE_WITH_MIDI
      Midi::update();
#endif // FORGE_WITH_MIDI

      window->pollEvents();

      double time = glfwGetTime();
      double dt = time - lastTime;
      lastTime = time;

      scene.tick(static_cast<float>(dt));

      render();

      ++frameCount;
      accumulatedTime += dt;
      if (accumulatedTime > kFrameRateReportInterval)
      {
         double frameRate = frameCount / accumulatedTime;
         frameCount = 0;
         accumulatedTime -= kFrameRateReportInterval;

         std::stringstream ss;
         ss << FORGE_PROJECT_NAME << " | " << std::fixed << std::setprecision(2) << frameRate << " FPS";
         window->setTitle(ss.str().c_str());
      }
   }

   context->getDevice().waitIdle();
}

void ForgeApplication::render()
{
   if (framebufferSizeChanged)
   {
      if (!recreateSwapchain())
      {
         return;
      }
   }

   vk::Device device = context->getDevice();

   vk::Result frameWaitResult = device.waitForFences({ frameFences[frameIndex] }, true, UINT64_MAX);
   if (frameWaitResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to wait for frame fence");
   }

   vk::ResultValue<uint32_t> swapchainIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   if (swapchainIndexResultValue.result == vk::Result::eErrorOutOfDateKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }

      swapchainIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   }
   if (swapchainIndexResultValue.result != vk::Result::eSuccess && swapchainIndexResultValue.result != vk::Result::eSuboptimalKHR)
   {
      throw std::runtime_error("Failed to acquire swapchain image");
   }
   uint32_t swapchainIndex = swapchainIndexResultValue.value;

   if (swapchainFences[swapchainIndex])
   {
      // If a previous frame is still using the swapchain image, wait for it to complete
      vk::Result imageWaitResult = device.waitForFences({ swapchainFences[swapchainIndex] }, true, UINT64_MAX);
      if (imageWaitResult != vk::Result::eSuccess)
      {
         throw std::runtime_error("Failed to wait for image fence");
      }
   }
   swapchainFences[swapchainIndex] = frameFences[frameIndex];

   context->setSwapchainIndex(swapchainIndex);
   context->setFrameIndex(frameIndex);

   vk::CommandBuffer commandBuffer = commandBuffers[swapchainIndex];
   {
      vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
         .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      commandBuffer.begin(commandBufferBeginInfo);

      renderer->render(commandBuffer, scene);

      commandBuffer.end();
   }

   std::array<vk::Semaphore, 1> waitSemaphores = { imageAvailableSemaphores[frameIndex] };
   std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
   static_assert(waitSemaphores.size() == waitStages.size(), "Wait semaphores and wait stages must be parallel arrays");

   std::array<vk::Semaphore, 1> signalSemaphores = { renderFinishedSemaphores[frameIndex] };
   vk::SubmitInfo submitInfo = vk::SubmitInfo()
      .setWaitSemaphores(waitSemaphores)
      .setPWaitDstStageMask(waitStages.data())
      .setCommandBuffers(commandBuffer)
      .setSignalSemaphores(signalSemaphores);

   device.resetFences({ frameFences[frameIndex] });
   context->getGraphicsQueue().submit({ submitInfo }, frameFences[frameIndex]);

   vk::SwapchainKHR swapchainKHR = swapchain->getSwapchainKHR();
   vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
      .setWaitSemaphores(signalSemaphores)
      .setSwapchains(swapchainKHR)
      .setImageIndices(swapchainIndex);

   vk::Result presentResult = context->getPresentQueue().presentKHR(presentInfo);
   if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }
   }
   else if (presentResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to present swapchain image");
   }

   frameIndex = (frameIndex + 1) % GraphicsContext::kMaxFramesInFlight;
}

bool ForgeApplication::recreateSwapchain()
{
   vk::Extent2D windowExtent = window->getExtent();
   while ((windowExtent.width == 0 || windowExtent.height == 0) && !window->shouldClose())
   {
      window->waitEvents();
      windowExtent = window->getExtent();
   }

   if (windowExtent.width == 0 || windowExtent.height == 0)
   {
      return false;
   }

   context->getDevice().waitIdle();

   terminateCommandBuffers(true);
   terminateSwapchain();

   initializeSwapchain();
   renderer->onSwapchainRecreated();
   initializeCommandBuffers();

   framebufferSizeChanged = false;

   return true;
}

void ForgeApplication::initializeGlfw()
{
   if (!glfwInit())
   {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   window = std::make_unique<Window>();

   window->bindOnFramebufferSizeChanged([this](int width, int height)
   {
      framebufferSizeChanged = true;
   });

   window->bindOnWindowRefreshRequested([this]()
   {
      render();
   });

   InputManager& inputManager = window->getInputManager();

   {
      std::array<KeyChord, 2> keys = { KeyChord(Key::F11), KeyChord(Key::Enter, KeyMod::Alt) };
      std::array<GamepadButtonChord, 1> gamepadButtons = { GamepadButtonChord(GamepadButton::Start) };
      inputManager.createButtonMapping(InputActions::kToggleFullscreen, keys, {}, gamepadButtons);

      inputManager.bindButtonMapping(InputActions::kToggleFullscreen, [this](bool pressed)
      {
         if (pressed)
         {
            window->toggleFullscreen();
         }
      });

      inputManager.createButtonMapping(InputActions::kReleaseCursor, KeyChord(Key::Escape), {}, {});

      inputManager.bindButtonMapping(InputActions::kReleaseCursor, [this](bool pressed)
      {
         if (pressed)
         {
            window->releaseCursor();
         }
      });
   }

   {
      std::array<KeyAxisChord, 2> moveForwardKeyAxes = { KeyAxisChord(Key::W, false), KeyAxisChord(Key::S, true) };
      std::array<GamepadAxisChord, 1> moveForwardGamepadAxes = { GamepadAxisChord(GamepadAxis::LeftY, false) };
      inputManager.createAxisMapping(InputActions::kMoveForward, moveForwardKeyAxes, {}, moveForwardGamepadAxes);

      std::array<KeyAxisChord, 2> moveRightKeyAxes = { KeyAxisChord(Key::D, false), KeyAxisChord(Key::A, true) };
      std::array<GamepadAxisChord, 1> moveRightGamepadAxes = { GamepadAxisChord(GamepadAxis::LeftX, false) };
      inputManager.createAxisMapping(InputActions::kMoveRight, moveRightKeyAxes, {}, moveRightGamepadAxes);

      std::array<KeyAxisChord, 2> moveUpKeyAxes = { KeyAxisChord(Key::Space, false), KeyAxisChord(Key::LeftControl, true) };
      std::array<GamepadAxisChord, 2> moveUpGamepadAxes = { GamepadAxisChord(GamepadAxis::RightTrigger, false), GamepadAxisChord(GamepadAxis::LeftTrigger, true) };
      inputManager.createAxisMapping(InputActions::kMoveUp, moveUpKeyAxes, {}, moveUpGamepadAxes);

      inputManager.createAxisMapping(InputActions::kLookRight, {}, CursorAxisChord(CursorAxis::X), GamepadAxisChord(GamepadAxis::RightX));
      inputManager.createAxisMapping(InputActions::kLookUp, {}, CursorAxisChord(CursorAxis::Y), GamepadAxisChord(GamepadAxis::RightY));
   }
}

void ForgeApplication::terminateGlfw()
{
   ASSERT(window);
   window = nullptr;

   glfwTerminate();
}

void ForgeApplication::initializeVulkan()
{
   context = std::make_unique<GraphicsContext>(*window);
   resourceManager = std::make_unique<ResourceManager>(*context);
}

void ForgeApplication::terminateVulkan()
{
   resourceManager = nullptr;
   context = nullptr;
}

void ForgeApplication::initializeSwapchain()
{
   swapchain = std::make_unique<Swapchain>(*context, window->getExtent());
   NAME_POINTER(context->getDevice(), swapchain, "Swapchain");
   context->setSwapchain(swapchain.get());
}

void ForgeApplication::terminateSwapchain()
{
   context->setSwapchain(nullptr);
   swapchain = nullptr;
}

void ForgeApplication::initializeRenderer()
{
   renderer = std::make_unique<Renderer>(*context, *resourceManager);

   InputManager& inputManager = window->getInputManager();

   inputManager.createButtonMapping(InputActions::kToggleMSAA, KeyChord(Key::M), {}, {});
   inputManager.bindButtonMapping(InputActions::kToggleMSAA, [this](bool pressed)
   {
      if (pressed && renderer)
      {
         renderer->toggleMSAA();
      }
   });

#if FORGE_WITH_DEBUG_UTILS
   inputManager.createButtonMapping(InputActions::kToggleLabels, KeyChord(Key::L), {}, {});
   inputManager.bindButtonMapping(InputActions::kToggleLabels, [](bool pressed)
   {
      if (pressed)
      {
         DebugUtils::SetLabelsEnabled(!DebugUtils::AreLabelsEnabled());
      }
   });
#endif // FORGE_WITH_DEBUG_UTILS
}

void ForgeApplication::terminateRenderer()
{
   renderer = nullptr;
}

void ForgeApplication::initializeCommandBuffers()
{
   if (!commandPool)
   {
      vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
         .setQueueFamilyIndex(context->getQueueFamilyIndices().graphicsFamily)
         .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);

      commandPool = context->getDevice().createCommandPool(commandPoolCreateInfo);
   }

   vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
      .setCommandPool(commandPool)
      .setLevel(vk::CommandBufferLevel::ePrimary)
      .setCommandBufferCount(GraphicsContext::kMaxFramesInFlight);

   ASSERT(commandBuffers.empty());
   commandBuffers = context->getDevice().allocateCommandBuffers(commandBufferAllocateInfo);
}

void ForgeApplication::terminateCommandBuffers(bool keepPoolAlive)
{
   if (commandPool)
   {
      if (keepPoolAlive)
      {
         context->getDevice().freeCommandBuffers(commandPool, commandBuffers);
      }
      else
      {
         // Command buffers will get cleaned up with the pool
         context->getDevice().destroyCommandPool(commandPool);
         commandPool = nullptr;
      }
   }

   commandBuffers.clear();
}

void ForgeApplication::initializeSyncObjects()
{
   imageAvailableSemaphores.resize(GraphicsContext::kMaxFramesInFlight);
   renderFinishedSemaphores.resize(GraphicsContext::kMaxFramesInFlight);
   frameFences.resize(GraphicsContext::kMaxFramesInFlight);
   swapchainFences.resize(swapchain->getImageCount());

   vk::SemaphoreCreateInfo semaphoreCreateInfo;
   vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo()
      .setFlags(vk::FenceCreateFlagBits::eSignaled);

   for (uint32_t i = 0; i < GraphicsContext::kMaxFramesInFlight; ++i)
   {
      imageAvailableSemaphores[i] = context->getDevice().createSemaphore(semaphoreCreateInfo);
      renderFinishedSemaphores[i] = context->getDevice().createSemaphore(semaphoreCreateInfo);
      frameFences[i] = context->getDevice().createFence(fenceCreateInfo);
   }
}

void ForgeApplication::terminateSyncObjects()
{
   for (vk::Fence frameFence : frameFences)
   {
      context->getDevice().destroyFence(frameFence);
   }
   frameFences.clear();

   for (vk::Semaphore renderFinishedSemaphore : renderFinishedSemaphores)
   {
      context->getDevice().destroySemaphore(renderFinishedSemaphore);
   }
   renderFinishedSemaphores.clear();

   for (vk::Semaphore imageAvailableSemaphore : imageAvailableSemaphores)
   {
      context->getDevice().destroySemaphore(imageAvailableSemaphore);
   }
   imageAvailableSemaphores.clear();
}

void ForgeApplication::loadScene()
{
   {
      static const float kCameraMoveSpeed = 3.0f;
      static const float kCameraLookSpeed = 180.0f;

      Entity cameraEntity = scene.createEntity();
      scene.setActiveCamera(cameraEntity);

      cameraEntity.createComponent<CameraComponent>();
      Transform& transform = cameraEntity.createComponent<TransformComponent>().transform;
      transform.orientation = glm::quat(glm::radians(glm::vec3(-10.0f, 0.0f, -70.0f)));
      transform.position = glm::vec3(-6.0f, -0.8f, 2.0f);

      InputManager& inputManager = window->getInputManager();

      inputManager.bindAxisMapping(InputActions::kMoveForward, [this, cameraEntity](float value) mutable
      {
         Transform& cameraTransform = cameraEntity.getComponent<TransformComponent>().transform;
         cameraTransform.translateBy(cameraTransform.getForwardVector() * value * kCameraMoveSpeed * scene.getRawDeltaTime());
      });

      inputManager.bindAxisMapping(InputActions::kMoveRight, [this, cameraEntity](float value) mutable
      {
         Transform& cameraTransform = cameraEntity.getComponent<TransformComponent>().transform;
         cameraTransform.translateBy(cameraTransform.getRightVector() * value * kCameraMoveSpeed * scene.getRawDeltaTime());
      });

      inputManager.bindAxisMapping(InputActions::kMoveUp, [this, cameraEntity](float value) mutable
      {
         Transform& cameraTransform = cameraEntity.getComponent<TransformComponent>().transform;
         cameraTransform.translateBy(cameraTransform.getUpVector() * value * kCameraMoveSpeed * scene.getRawDeltaTime());
      });

      inputManager.bindAxisMapping(InputActions::kLookRight, [this, cameraEntity](float value) mutable
      {
         Transform& cameraTransform = cameraEntity.getComponent<TransformComponent>().transform;
         cameraTransform.rotateBy(glm::angleAxis(glm::radians(value * kCameraLookSpeed * scene.getRawDeltaTime()), -MathUtils::kUpVector));
      });

      inputManager.bindAxisMapping(InputActions::kLookUp, [this, cameraEntity](float value) mutable
      {
         Transform& cameraTransform = cameraEntity.getComponent<TransformComponent>().transform;
         glm::vec3 euler = glm::degrees(glm::eulerAngles(cameraTransform.orientation));
         euler.x = glm::clamp(euler.x + value * kCameraLookSpeed * 0.75f * scene.getRawDeltaTime(), -89.0f, 89.0f);
         cameraTransform.orientation = glm::quat(glm::radians(euler));
      });
   }

   {
      Entity sponzaEntity = scene.createEntity();
      sponzaEntity.createComponent<TransformComponent>();
      MeshComponent& meshComponent = sponzaEntity.createComponent<MeshComponent>();

      MeshLoadOptions meshLoadOptions;
      meshLoadOptions.scale = 0.01f;
      meshLoadOptions.interpretTextureAlphaAsMask = true;
      meshComponent.meshHandle = resourceManager->loadMesh("Resources/Meshes/Sponza/Sponza.gltf", meshLoadOptions);
   }

   {
      Entity bunnyEntity = scene.createEntity();
      TransformComponent& transformComponent = bunnyEntity.createComponent<TransformComponent>();
      transformComponent.transform.position = glm::vec3(0.0f, 1.0f, 0.0f);
      transformComponent.transform.scaleBy(glm::vec3(5.0f));

      MeshComponent& meshComponent = bunnyEntity.createComponent<MeshComponent>();
      meshComponent.meshHandle = resourceManager->loadMesh("Resources/Meshes/Bunny.obj");
   }

   {
      Entity directionalLightEntity = scene.createEntity();

      directionalLightEntity.createComponent<TransformComponent>();

      DirectionalLightComponent& directionalLightComponent = directionalLightEntity.createComponent<DirectionalLightComponent>();
      directionalLightComponent.setColor(glm::vec3(3.0f));
      directionalLightComponent.setShadowWidth(20.0f);
      directionalLightComponent.setShadowHeight(15.0f);
      directionalLightComponent.setShadowDepth(20.0f);

      scene.addTickDelegate([this, directionalLightEntity](float dt) mutable
      {
         float time = scene.getTime();

         TransformComponent& transformComponent = directionalLightEntity.getComponent<TransformComponent>();
         float pitchOffset = 25.0f * glm::sin(time * 0.45f - 0.5f);
         float yawOffset = 25.0f * glm::cos(time * 0.25f + 3.14f);
         transformComponent.transform.orientation = glm::angleAxis(glm::radians(-90.0f + pitchOffset), MathUtils::kRightVector) * glm::angleAxis(glm::radians(yawOffset), MathUtils::kUpVector);
      });
   }

   {
      Entity pointLightEntity = scene.createEntity();

      pointLightEntity.createComponent<TransformComponent>();

      PointLightComponent& pointLightComponent = pointLightEntity.createComponent<PointLightComponent>();
      pointLightComponent.setColor(glm::vec3(0.1f, 0.3f, 0.8f) * 70.0f);
      pointLightComponent.setRadius(50.0f);

      scene.addTickDelegate([this, pointLightEntity](float dt) mutable
      {
         float time = scene.getTime();

         TransformComponent& transformComponent = pointLightEntity.getComponent<TransformComponent>();
         transformComponent.transform.position = glm::vec3(glm::sin(time) * 5.0f, glm::cos(time * 0.7f) * 1.5f, glm::sin(time * 1.1f) * 2.0f + 3.0f);
      });
   }

   {
      Entity spotLightEntity = scene.createEntity();
      spotLightEntity.createComponent<TransformComponent>();

      SpotLightComponent& spotLightComponent = spotLightEntity.createComponent<SpotLightComponent>();
      spotLightComponent.setColor(glm::vec3(0.8f, 0.1f, 0.3f) * 70.0f);
      spotLightComponent.setRadius(50.0f);

      scene.addTickDelegate([this, spotLightEntity](float dt) mutable
      {
         float time = scene.getTime();

         TransformComponent& transformComponent = spotLightEntity.getComponent<TransformComponent>();
         transformComponent.transform.position = glm::vec3(glm::cos(time * 0.6f) * 8.0f, glm::sin(time * 0.3f) - 4.5f, glm::cos(time * 1.3f) * 1.5f + 1.5f);
      });
   }
}
