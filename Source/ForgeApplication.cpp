#include "ForgeApplication.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Mesh.h"
#include "Graphics/Swapchain.h"

#if FORGE_WITH_MIDI
#  include "Platform/Midi.h"
#endif // FORGE_WITH_MIDI
#include "Platform/Window.h"

#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/Renderer.h"

#include "Resources/ResourceManager.h"

#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/NameComponent.h"
#include "Scene/Components/OscillatingMovementComponent.h"
#include "Scene/Components/SkyboxComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Scene/Systems/CameraSystem.h"
#include "Scene/Systems/OscillatingMovementSystem.h"

#include "UI/UI.h"

#include <GLFW/glfw3.h>

#include <array>
#include <stdexcept>

namespace
{
   namespace InputActions
   {
      const char* kToggleFullscreen = "ToggleFullscreen";
      const char* kReleaseCursor = "ReleaseCursor";

      const char* kToggleHDR = "ToggleHDR";
      const char* kToggleTonemapper = "ToggleTonemapper";
      const char* kToggleLabels = "ToggleLabels";
   }

   void glfwErrorCallback(int errorCode, const char* description)
   {
      ASSERT(false, "Encountered GLFW error %d: %s", errorCode, description);
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
   initializeUI();
   initializeCommandBuffers();
   initializeSyncObjects();

   loadScene();
}

ForgeApplication::~ForgeApplication()
{
   unloadScene();

   terminateSyncObjects();
   terminateCommandBuffers(false);
   terminateUI();
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
   static const double kMaxDeltaTime = 0.2;
   static const double kFrameRateReportInterval = 0.25;

   double lastTime = glfwGetTime();
   while (!window->shouldClose())
   {
#if FORGE_WITH_MIDI
      Midi::update();
#endif // FORGE_WITH_MIDI

      window->pollEvents();

      double time = glfwGetTime();
      double dt = time - lastTime;
      if (dt > kMaxDeltaTime)
      {
         dt = 0.0;
      }
      lastTime = time;

#if FORGE_WITH_MIDI
      if (Midi::isConnected())
      {
         scene->setTimeScale(Midi::getState().groups[7].slider);
      }
#endif // FORGE_WITH_MIDI

      scene->tick(static_cast<float>(dt));

      render();
   }

   context->getDevice().waitIdle();
}

void ForgeApplication::render()
{
   RenderSettings newRenderSettings = renderSettings;
   ui->render(*context, *scene, renderCapabilities, newRenderSettings, *resourceManager);

   updateRenderSettings(newRenderSettings);

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

   if (resourceManager)
   {
      resourceManager->update();
   }

   vk::CommandBuffer commandBuffer = commandBuffers[swapchainIndex];
   {
      vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
         .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      commandBuffer.begin(commandBufferBeginInfo);

      renderer->render(commandBuffer, *scene);

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

void ForgeApplication::updateRenderSettings(const RenderSettings& newRenderSettings)
{
   if (renderSettings != newRenderSettings)
   {
      bool presentHDRChanged = newRenderSettings.presentHDR != renderSettings.presentHDR;

      renderSettings = newRenderSettings;

      if (presentHDRChanged)
      {
         recreateSwapchain();
      }

      if (renderer)
      {
         renderer->updateRenderSettings(newRenderSettings);
      }
   }
}

void ForgeApplication::initializeGlfw()
{
   glfwSetErrorCallback(glfwErrorCallback);

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
      inputManager.createAxisMapping(CameraSystemInputActions::kMoveForward, moveForwardKeyAxes, {}, moveForwardGamepadAxes);

      std::array<KeyAxisChord, 2> moveRightKeyAxes = { KeyAxisChord(Key::D, false), KeyAxisChord(Key::A, true) };
      std::array<GamepadAxisChord, 1> moveRightGamepadAxes = { GamepadAxisChord(GamepadAxis::LeftX, false) };
      inputManager.createAxisMapping(CameraSystemInputActions::kMoveRight, moveRightKeyAxes, {}, moveRightGamepadAxes);

      std::array<KeyAxisChord, 2> moveUpKeyAxes = { KeyAxisChord(Key::Space, false), KeyAxisChord(Key::LeftControl, true) };
      std::array<GamepadAxisChord, 2> moveUpGamepadAxes = { GamepadAxisChord(GamepadAxis::RightTrigger, false), GamepadAxisChord(GamepadAxis::LeftTrigger, true) };
      inputManager.createAxisMapping(CameraSystemInputActions::kMoveUp, moveUpKeyAxes, {}, moveUpGamepadAxes);

      inputManager.createAxisMapping(CameraSystemInputActions::kLookRight, {}, CursorAxisChord(CursorAxis::X), GamepadAxisChord(GamepadAxis::RightX));
      inputManager.createAxisMapping(CameraSystemInputActions::kLookUp, {}, CursorAxisChord(CursorAxis::Y), GamepadAxisChord(GamepadAxis::RightY));
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
   swapchain = std::make_unique<Swapchain>(*context, window->getExtent(), renderSettings.presentHDR);
   NAME_POINTER(context->getDevice(), swapchain, "Swapchain");
   context->setSwapchain(swapchain.get());

   renderCapabilities.canPresentHDR = swapchain->supportsHDR();
}

void ForgeApplication::terminateSwapchain()
{
   context->setSwapchain(nullptr);
   swapchain = nullptr;
}

void ForgeApplication::initializeRenderer()
{
   renderer = std::make_unique<Renderer>(*context, *resourceManager, renderSettings);

   InputManager& inputManager = window->getInputManager();

   inputManager.createButtonMapping(InputActions::kToggleHDR, KeyChord(Key::H), {}, {});
   inputManager.bindButtonMapping(InputActions::kToggleHDR, [this](bool pressed)
   {
      if (pressed)
      {
         RenderSettings newRenderSettings = renderSettings;
         newRenderSettings.presentHDR = !renderSettings.presentHDR;
         updateRenderSettings(newRenderSettings);
      }
   });

   inputManager.createButtonMapping(InputActions::kToggleTonemapper, KeyChord(Key::T), {}, {});
   inputManager.bindButtonMapping(InputActions::kToggleTonemapper, [this](bool pressed)
   {
      if (pressed)
      {
         RenderSettings newRenderSettings = renderSettings;
         switch (renderSettings.tonemappingAlgorithm)
         {
         case TonemappingAlgorithm::None:
            newRenderSettings.tonemappingAlgorithm = TonemappingAlgorithm::Curve;
            break;
         case TonemappingAlgorithm::Curve:
            newRenderSettings.tonemappingAlgorithm = TonemappingAlgorithm::Reinhard;
            break;
         case TonemappingAlgorithm::Reinhard:
            newRenderSettings.tonemappingAlgorithm = TonemappingAlgorithm::TonyMcMapface;
            break;
         case TonemappingAlgorithm::TonyMcMapface:
            newRenderSettings.tonemappingAlgorithm = TonemappingAlgorithm::None;
            break;
         default:
            break;
         }
         updateRenderSettings(newRenderSettings);
      }
   });

#if FORGE_WITH_DEBUG_UTILS
   inputManager.createButtonMapping(InputActions::kToggleLabels, KeyChord(Key::L), {}, {});
   inputManager.bindButtonMapping(InputActions::kToggleLabels, [](bool pressed)
   {
      if (pressed)
      {
         DebugUtils::setLabelsEnabled(!DebugUtils::labelsAreEnabled());
      }
   });
#endif // FORGE_WITH_DEBUG_UTILS
}

void ForgeApplication::terminateRenderer()
{
   renderer = nullptr;
}

void ForgeApplication::initializeUI()
{
   ui = std::make_unique<UI>();
}

void ForgeApplication::terminateUI()
{
   ui = nullptr;
}

void ForgeApplication::initializeCommandBuffers()
{
   if (!commandPool)
   {
      vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
         .setQueueFamilyIndex(context->getGraphicsFamilyIndex())
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
   scene = std::make_unique<Scene>();

   CameraSystem* cameraSystem = scene->createSystem<CameraSystem>(window->getInputManager());
   scene->createSystem<OscillatingMovementSystem>();

   {
      Entity cameraEntity = scene->createEntity();
      cameraSystem->setActiveCamera(cameraEntity);

      cameraEntity.createComponent<NameComponent>().name = "Camera";
      cameraEntity.createComponent<CameraComponent>();
      Transform& transform = cameraEntity.createComponent<TransformComponent>().transform;
      transform.orientation = glm::quat(glm::radians(glm::vec3(-10.0f, 0.0f, -70.0f)));
      transform.position = glm::vec3(-6.0f, -0.8f, 2.0f);
   }

   {
      Entity skyboxEntity = scene->createEntity();
      skyboxEntity.createComponent<NameComponent>().name = "Skybox";
      skyboxEntity.createComponent<SkyboxComponent>().textureHandle = resourceManager->loadTexture("Resources/Textures/Skybox/Kloofendal.dds");
   }

   {
      Entity sponzaEntity = scene->createEntity();
      sponzaEntity.createComponent<NameComponent>().name = "Sponza";
      sponzaEntity.createComponent<TransformComponent>();
      MeshComponent& meshComponent = sponzaEntity.createComponent<MeshComponent>();

      MeshLoadOptions meshLoadOptions;
      meshLoadOptions.interpretTextureAlphaAsMask = true;
      meshComponent.meshHandle = resourceManager->loadMesh("Resources/Meshes/Sponza/Sponza.gltf", meshLoadOptions);
   }

   {
      Entity bunnyEntity = scene->createEntity();
      bunnyEntity.createComponent<NameComponent>().name = "Bunny";
      TransformComponent& transformComponent = bunnyEntity.createComponent<TransformComponent>();
      transformComponent.transform.position = glm::vec3(0.0f, 1.0f, 0.0f);
      transformComponent.transform.scaleBy(glm::vec3(5.0f));

      MeshComponent& meshComponent = bunnyEntity.createComponent<MeshComponent>();
      meshComponent.meshHandle = resourceManager->loadMesh("Resources/Meshes/Bunny.obj");
      if (const Mesh* mesh = resourceManager->getMesh(meshComponent.meshHandle))
      {
         for (uint32_t i = 0; i < mesh->getNumSections(); ++i)
         {
            if (Material* material = resourceManager->getMaterial(mesh->getSection(i).materialHandle))
            {
               if (PhysicallyBasedMaterial* pbrMaterial = dynamic_cast<PhysicallyBasedMaterial*>(material))
               {
                  pbrMaterial->setEmissiveColor(glm::vec4(1.0f));
                  pbrMaterial->setEmissiveIntensity(100.0f);
               }
            }
         }
      }
   }

   {
      Entity directionalLightEntity = scene->createEntity();

      directionalLightEntity.createComponent<NameComponent>().name = "Directional Light";
      directionalLightEntity.createComponent<TransformComponent>().transform.orientation = glm::quat(glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)));

      DirectionalLightComponent& directionalLightComponent = directionalLightEntity.createComponent<DirectionalLightComponent>();
      directionalLightComponent.setBrightness(3.0f);
      directionalLightComponent.setShadowWidth(20.0f);
      directionalLightComponent.setShadowHeight(15.0f);
      directionalLightComponent.setShadowDepth(25.0f);

      OscillatingMovementComponent& oscillatingMovementComponent = directionalLightEntity.createComponent<OscillatingMovementComponent>();
      oscillatingMovementComponent.rotation.sin.timeScale = glm::vec3(0.45f, 0.0f, 0.0f);
      oscillatingMovementComponent.rotation.sin.valueScale = glm::vec3(25.0f, 0.0f, 0.0f);
      oscillatingMovementComponent.rotation.cos.timeOffset = glm::vec3(0.0f, 0.0f, 3.14f);
      oscillatingMovementComponent.rotation.cos.timeScale = glm::vec3(0.0f, 0.0f, 0.25f);
      oscillatingMovementComponent.rotation.cos.valueScale = glm::vec3(0.0f, 0.0f, 25.0f);
   }

   {
      Entity pointLightEntity = scene->createEntity();

      pointLightEntity.createComponent<NameComponent>().name = "Point Light";
      TransformComponent& transformComponent = pointLightEntity.createComponent<TransformComponent>();
      transformComponent.transform.position = glm::vec3(0.0f, 0.0f, 4.0f);

      PointLightComponent& pointLightComponent = pointLightEntity.createComponent<PointLightComponent>();
      pointLightComponent.setColor(glm::vec3(0.1f, 0.3f, 0.8f));
      pointLightComponent.setBrightness(70.0f);
      pointLightComponent.setRadius(50.0f);

      OscillatingMovementComponent& oscillatingMovementComponent = pointLightEntity.createComponent<OscillatingMovementComponent>();
      oscillatingMovementComponent.location.sin.timeScale = glm::vec3(1.0f, 0.7f, 1.1f);
      oscillatingMovementComponent.location.sin.valueScale = glm::vec3(5.5f, 1.0f, 2.5f);
   }


   {
      Entity spotLightEntity = scene->createEntity();

      spotLightEntity.createComponent<NameComponent>().name = "Spot Light";
      spotLightEntity.createComponent<TransformComponent>().transform.position = glm::vec3(8.0f, -3.5f, 2.0f);

      SpotLightComponent& spotLightComponent = spotLightEntity.createComponent<SpotLightComponent>();
      spotLightComponent.setColor(glm::vec3(0.8f, 0.1f, 0.3f));
      spotLightComponent.setBrightness(70.0f);
      spotLightComponent.setRadius(50.0f);

      OscillatingMovementComponent& oscillatingMovementComponent = spotLightEntity.createComponent<OscillatingMovementComponent>();
      oscillatingMovementComponent.location.sin.timeScale = glm::vec3(0.0f, 0.3f, 0.0f);
      oscillatingMovementComponent.location.sin.valueScale = glm::vec3(0.0f, 1.0f, 0.0f);
      oscillatingMovementComponent.location.cos.timeScale = glm::vec3(0.6f, 0.0f, 1.3f);
      oscillatingMovementComponent.location.cos.valueScale = glm::vec3(8.0f, 0.0f, 1.0f);
   }
}

void ForgeApplication::unloadScene()
{
   scene.reset();
}
