#include "UI/UI.h"

#include "Core/Enum.h"

#include "Graphics/GraphicsContext.h"

#include "Math/MathUtils.h"

#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/RenderSettings.h"

#include "Resources/ResourceManager.h"

#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/NameComponent.h"
#include "Scene/Components/SkyboxComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"

#include "UI/UIConfig.h"

#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <string>
#include <unordered_map>

namespace
{
   bool DragScalarNFormat(const char* label, ImGuiDataType data_type, void* p_data, int components, float v_speed = 1.0f, const void* p_min = NULL, const void* p_max = NULL, const char** format = NULL, ImGuiSliderFlags flags = 0)
   {
      static const ImGuiDataTypeInfo GDataTypeInfo[] =
      {
          { sizeof(char),             "S8",   "%d",   "%d"    },  // ImGuiDataType_S8
          { sizeof(unsigned char),    "U8",   "%u",   "%u"    },
          { sizeof(short),            "S16",  "%d",   "%d"    },  // ImGuiDataType_S16
          { sizeof(unsigned short),   "U16",  "%u",   "%u"    },
          { sizeof(int),              "S32",  "%d",   "%d"    },  // ImGuiDataType_S32
          { sizeof(unsigned int),     "U32",  "%u",   "%u"    },
      #ifdef _MSC_VER
          { sizeof(ImS64),            "S64",  "%I64d","%I64d" },  // ImGuiDataType_S64
          { sizeof(ImU64),            "U64",  "%I64u","%I64u" },
      #else
          { sizeof(ImS64),            "S64",  "%lld", "%lld"  },  // ImGuiDataType_S64
          { sizeof(ImU64),            "U64",  "%llu", "%llu"  },
      #endif
          { sizeof(float),            "float", "%.3f","%f"    },  // ImGuiDataType_Float (float are promoted to double in va_arg)
          { sizeof(double),           "double","%f",  "%lf"   },  // ImGuiDataType_Double
      };

      ImGuiWindow* window = ImGui::GetCurrentWindow();
      if (window->SkipItems)
         return false;

      ImGuiContext& g = *GImGui;
      bool value_changed = false;
      ImGui::BeginGroup();
      ImGui::PushID(label);
      ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
      size_t type_size = GDataTypeInfo[data_type].Size;
      for (int i = 0; i < components; i++)
      {
         ImGui::PushID(i);
         if (i > 0)
            ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
         value_changed |= ImGui::DragScalar("", data_type, p_data, v_speed, p_min, p_max, format[i], flags);
         ImGui::PopID();
         ImGui::PopItemWidth();
         p_data = (void*)((char*)p_data + type_size);
      }
      ImGui::PopID();

      const char* label_end = ImGui::FindRenderedTextEnd(label);
      if (label != label_end)
      {
         ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
         ImGui::TextEx(label, label_end);
      }

      ImGui::EndGroup();
      return value_changed;
   }

   bool DragFloat2Format(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format1 = "%.3f", const char* format2 = "%.3f", ImGuiSliderFlags flags = 0)
   {
      std::array<const char*, 2> formats = { format1, format2 };
      return DragScalarNFormat(label, ImGuiDataType_Float, v, 2, v_speed, &v_min, &v_max, formats.data(), flags);
   }

   bool DragFloat3Format(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format1 = "%.3f", const char* format2 = "%.3f", const char* format3 = "%.3f", ImGuiSliderFlags flags = 0)
   {
      std::array<const char*, 3> formats = { format1, format2, format3 };
      return DragScalarNFormat(label, ImGuiDataType_Float, v, 3, v_speed, &v_min, &v_max, formats.data(), flags);
   }

   bool DragFloat4Format(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format1 = "%.3f", const char* format2 = "%.3f", const char* format3 = "%.3f", const char* format4 = "%.3f", ImGuiSliderFlags flags = 0)
   {
      std::array<const char*, 4> formats = { format1, format2, format3, format4 };
      return DragScalarNFormat(label, ImGuiDataType_Float, v, 4, v_speed, &v_min, &v_max, formats.data(), flags);
   }

   glm::vec3 srgbToLinear(const glm::vec3& srgb)
   {
      return glm::convertSRGBToLinear(srgb);
   }

   glm::vec3 linearToSrgb(const glm::vec3& linear)
   {
      return glm::convertLinearToSRGB(linear);
   }

   glm::vec4 srgbToLinear(const glm::vec4& srgb)
   {
      glm::vec4 linear = glm::convertSRGBToLinear(srgb);
      linear.a = srgb.a;
      return linear;
   }

   glm::vec4 linearToSrgb(const glm::vec4& linear)
   {
      glm::vec4 srgb = glm::convertLinearToSRGB(linear);
      srgb.a = linear.a;
      return srgb;
   }

   const char* getEntityName(const Entity& entity, const char* defaultName)
   {
      if (entity)
      {
         if (const NameComponent* nameComponent = entity.tryGetComponent<NameComponent>())
         {
            return nameComponent->name.c_str();
         }
      }

      return defaultName;
   }

   bool renderTransform(Transform& transform)
   {
      bool anyModified = false;

      anyModified |= DragFloat3Format("Position", glm::value_ptr(transform.position), 0.01f, 0.0f, 0.0f, "X: %.3f", "Y: %.3f", "Z: %.3f");

      glm::vec3 rotation = glm::degrees(glm::eulerAngles(transform.orientation));
      if (DragFloat3Format("Rotation", glm::value_ptr(rotation), 0.1f, 0.0f, 0.0f, "Pitch: %.3f", "Roll: %.3f", "Yaw: %.3f"))
      {
         transform.orientation = glm::quat(glm::radians(rotation));
         anyModified = true;
      }

      anyModified |= DragFloat3Format("Scale", glm::value_ptr(transform.scale), 0.01f, 0.0f, 0.0f, "X: %.3f", "Y: %.3f", "Z: %.3f");

      return anyModified;
   }

   void renderTransformComponent(TransformComponent& transformComponent)
   {
      if (ImGui::BeginTabItem("Transform"))
      {
         if (transformComponent.getParentComponent())
         {
            ImGui::Text("Absolute");

            ImGui::PushID("Absolute");
            Transform absoluteTransform = transformComponent.getAbsoluteTransform();
            if (renderTransform(absoluteTransform))
            {
               transformComponent.setAbsoluteTransform(absoluteTransform);
            }
            ImGui::PopID();

            ImGui::Text("Relative");
         }

         renderTransform(transformComponent.transform);

         ImGui::EndTabItem();
      }
   }

   void renderCameraComponent(CameraComponent& cameraComponent)
   {
      if (ImGui::BeginTabItem("Camera"))
      {
         ImGui::SliderFloat("Field of View", &cameraComponent.fieldOfView, 5.0f, 140.0f);
         ImGui::DragFloatRange2("Clip Planes", &cameraComponent.nearPlane, &cameraComponent.farPlane, 0.01f, 0.01f, 1000.0f, "Near: %.2f", "Far: %.2f");

         ImGui::EndTabItem();
      }
   }

   void renderLightComponent(LightComponent& lightComponent)
   {
      glm::vec3 color = linearToSrgb(lightComponent.getColor());
      if (ImGui::ColorEdit3("Color", glm::value_ptr(color)))
      {
         lightComponent.setColor(srgbToLinear(color));
      }

      float brightness = lightComponent.getBrightness();
      if (ImGui::DragFloat("Brightness", &brightness, 0.01f, 0.0f, 100.0f))
      {
         lightComponent.setBrightness(brightness);
      }

      bool castShadows = lightComponent.castsShadows();
      if (ImGui::Checkbox("Cast Shadows", &castShadows))
      {
         lightComponent.setCastShadows(castShadows);
      }

      std::array<float, 3> shadowBiasValues = { lightComponent.getShadowBiasConstantFactor(), lightComponent.getShadowBiasSlopeFactor(), lightComponent.getShadowBiasClamp() };
      if (DragFloat3Format("Shadow Bias", shadowBiasValues.data(), 0.001f, 0.0f, 100.0f, "Constant: %.3f", "Slope: %.3f", "Clamp: %.3f"))
      {
         lightComponent.setShadowBiasConstantFactor(shadowBiasValues[0]);
         lightComponent.setShadowBiasSlopeFactor(shadowBiasValues[1]);
         lightComponent.setShadowBiasClamp(shadowBiasValues[2]);
      }
   }

   void renderDirectionalLightComponent(DirectionalLightComponent& directionalLightComponent)
   {
      if (ImGui::BeginTabItem("Directional Light"))
      {
         renderLightComponent(directionalLightComponent);

         std::array<float, 3> shadowProjectionValues = { directionalLightComponent.getShadowWidth(), directionalLightComponent.getShadowHeight(), directionalLightComponent.getShadowDepth() };
         if (DragFloat3Format("Shadow Projection", shadowProjectionValues.data(), 0.01f, 0.0f, 100.0f, "Width: %.3f", "Height: %.3f", "Depth: %.3f"))
         {
            directionalLightComponent.setShadowWidth(shadowProjectionValues[0]);
            directionalLightComponent.setShadowHeight(shadowProjectionValues[1]);
            directionalLightComponent.setShadowDepth(shadowProjectionValues[2]);
         }

         ImGui::EndTabItem();
      }
   }

   void renderPointLightComponent(PointLightComponent& pointLightComponent)
   {
      if (ImGui::BeginTabItem("Point Light"))
      {
         renderLightComponent(pointLightComponent);

         float radius = pointLightComponent.getRadius();
         if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.0f, 100.0f))
         {
            pointLightComponent.setRadius(radius);
         }

         float shadowNearPlane = pointLightComponent.getShadowNearPlane();
         if (ImGui::DragFloat("Shadow Near Plane", &shadowNearPlane, 0.01f, 0.0f, 100.0f))
         {
            pointLightComponent.setShadowNearPlane(shadowNearPlane);
         }

         ImGui::EndTabItem();
      }
   }

   void renderSpotLightComponent(SpotLightComponent& spotLightComponent)
   {
      if (ImGui::BeginTabItem("Spot Light"))
      {
         renderLightComponent(spotLightComponent);

         float radius = spotLightComponent.getRadius();
         if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.0f, 100.0f))
         {
            spotLightComponent.setRadius(radius);
         }

         float beamAngle = spotLightComponent.getBeamAngle();
         float cutoffAngle = spotLightComponent.getCutoffAngle();
         if (ImGui::DragFloatRange2("Spot Angle", &beamAngle, &cutoffAngle, 0.1f, 0.0f, 179.0f, "Beam: %.1f°", "Cutoff: %.1f°", ImGuiSliderFlags_AlwaysClamp))
         {
            spotLightComponent.setBeamAngle(beamAngle);
            spotLightComponent.setCutoffAngle(cutoffAngle);
         }

         float shadowNearPlane = spotLightComponent.getShadowNearPlane();
         if (ImGui::DragFloat("Shadow Near Plane", &shadowNearPlane, 0.01f, 0.0f, 100.0f))
         {
            spotLightComponent.setShadowNearPlane(shadowNearPlane);
         }

         ImGui::EndTabItem();
      }
   }

   const char* getBlendModePreviewText(BlendMode blendMode)
   {
      switch (blendMode)
      {
      case BlendMode::Opaque:
         return "Opaque";
      case BlendMode::Masked:
         return "Masked";
      case BlendMode::Translucent:
         return "Translucent";
      default:
         return "Invalid";
      }
   }

   void renderMaterial(Material& material)
   {
      if (ImGui::CollapsingHeader("Material", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
      {
         static const std::array<BlendMode, 3> kBlendModeValues = { BlendMode::Opaque, BlendMode::Masked, BlendMode::Translucent };
         if (ImGui::BeginCombo("Blend Mode", getBlendModePreviewText(material.getBlendMode())))
         {
            for (BlendMode blendMode : kBlendModeValues)
            {
               bool isSelected = material.getBlendMode() == blendMode;
               if (ImGui::Selectable(getBlendModePreviewText(blendMode), isSelected))
               {
                  material.setBlendMode(blendMode);
               }

               if (isSelected)
               {
                  ImGui::SetItemDefaultFocus();
               }
            }

            ImGui::EndCombo();
         }

         bool twoSided = material.isTwoSided();
         if (ImGui::Checkbox("Two-Sided", &twoSided))
         {
            material.setTwoSided(twoSided);
         }

         if (PhysicallyBasedMaterial* pbrMaterial = dynamic_cast<PhysicallyBasedMaterial*>(&material))
         {
            glm::vec4 albedo = linearToSrgb(pbrMaterial->getAlbedoColor());
            if (ImGui::ColorEdit4("Albedo", glm::value_ptr(albedo)))
            {
               pbrMaterial->setAlbedoColor(srgbToLinear(albedo));
            }

            glm::vec4 emissive = linearToSrgb(pbrMaterial->getEmissiveColor());
            if (ImGui::ColorEdit4("Emissive", glm::value_ptr(emissive)))
            {
               pbrMaterial->setEmissiveColor(srgbToLinear(emissive));
            }

            float emissiveIntensity = pbrMaterial->getEmissiveIntensity();
            if (ImGui::DragFloat("Emissive Intensity", &emissiveIntensity, 0.01f, 0.0f, FLT_MAX))
            {
               pbrMaterial->setEmissiveIntensity(emissiveIntensity);
            }

            float roughness = pbrMaterial->getRoughness();
            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
            {
               pbrMaterial->setRoughness(roughness);
            }

            float metalness = pbrMaterial->getMetalness();
            if (ImGui::SliderFloat("Metalness", &metalness, 0.0f, 1.0f))
            {
               pbrMaterial->setMetalness(metalness);
            }

            float ambientOcclusion = pbrMaterial->getAmbientOcclusion();
            if (ImGui::SliderFloat("Ambient Occlusion", &ambientOcclusion, 0.0f, 1.0f))
            {
               pbrMaterial->setAmbientOcclusion(ambientOcclusion);
            }
         }
      }
   }

   void renderMesh(Mesh& mesh, ResourceManager& resourceManager, uint32_t& selectedMeshSection)
   {
      ImGui::BeginChild("Section List", ImVec2(100, 0), true);
      for (uint32_t i = 0; i < mesh.getNumSections(); ++i)
      {
         std::string label = "Section " + std::to_string(i);
         if (ImGui::Selectable(label.c_str(), selectedMeshSection == i))
         {
            selectedMeshSection = i;
         }
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (selectedMeshSection < mesh.getNumSections())
      {
         MeshSection& section = mesh.getSection(selectedMeshSection);

         ImGui::BeginChild("Selected Section");

         ImGui::Text("Indices: %u", section.numIndices);
         ImGui::Text("Has valid texture coordinates: %s", section.hasValidTexCoords ? "true" : "false");

         if (Material* material = resourceManager.getMaterial(section.materialHandle))
         {
            renderMaterial(*material);
         }

         ImGui::EndChild();
      }
   }

   void renderMeshComponent(MeshComponent& meshComponent, ResourceManager& resourceManager, uint32_t& selectedMeshSection)
   {
      if (ImGui::BeginTabItem("Mesh"))
      {
         const std::string* meshPath = resourceManager.getMeshPath(meshComponent.meshHandle);
         const char* pathString = meshPath ? meshPath->c_str() : "None";
         ImGui::Text("%s", pathString);

         ImGui::Checkbox("Cast Shadows", &meshComponent.castsShadows);

         if (Mesh* mesh = resourceManager.getMesh(meshComponent.meshHandle))
         {
            renderMesh(*mesh, resourceManager, selectedMeshSection);
         }

         ImGui::EndTabItem();
      }
   }

   void renderSkyboxComponent(const SkyboxComponent& skyboxComponent, const ResourceManager& resourceManager)
   {
      if (ImGui::BeginTabItem("Skybox"))
      {
         const std::string* texturePath = resourceManager.getTexturePath(skyboxComponent.textureHandle);
         const char* pathString = texturePath ? texturePath->c_str() : "None";
         ImGui::Text("%s", pathString);

         ImGui::EndTabItem();
      }
   }

   void renderEntity(const Entity& entity, const std::unordered_map<Entity, std::vector<Entity>>& entityTree, Entity& selectedEntity)
   {
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
      if (entity == selectedEntity)
      {
         flags |= ImGuiTreeNodeFlags_Selected;
      }
      bool open = ImGui::TreeNodeEx(getEntityName(entity, "Entity"), flags);

      if (ImGui::IsItemClicked())
      {
         selectedEntity = entity;
      }

      if (open)
      {
         auto location = entityTree.find(entity);
         if (location != entityTree.end())
         {
            for (const Entity& child : location->second)
            {
               renderEntity(child, entityTree, selectedEntity);
            }
         }

         ImGui::TreePop();
      }
   }

   const char* getMSAAPreviewText(vk::SampleCountFlagBits sampleCount)
   {
      switch (sampleCount)
      {
      case vk::SampleCountFlagBits::e1:
         return "Disabled";
      case vk::SampleCountFlagBits::e2:
         return "2x";
      case vk::SampleCountFlagBits::e4:
         return "4x";
      case vk::SampleCountFlagBits::e8:
         return "8x";
      case vk::SampleCountFlagBits::e16:
         return "16x";
      case vk::SampleCountFlagBits::e32:
         return "32x";
      case vk::SampleCountFlagBits::e64:
         return "64x";
      default:
         return "Invalid";
      }
   }

   const float kSceneWindowHeight = 320.0f;

   const std::array<const char*, 4> kRenderQualityNames = { "Disabled", "Low", "Medium", "High" };
   const std::array<const char*, 5> kTonemappingAlgorithmNames = { "None", "Curve", "Reinhard", "Tony McMapface", "Double Fine" };
}

// static
void UI::initialize(struct GLFWwindow* window, bool installCallbacks)
{
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForVulkan(window, installCallbacks);
}

// static
void UI::terminate()
{
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

// static
bool UI::isVisible()
{
   return visible;
}

// static
void UI::setVisible(bool newVisible)
{
   visible = newVisible;
}

// static
bool UI::wantsMouseInput()
{
   if (ImGui::GetCurrentContext())
   {
      return ImGui::GetIO().WantCaptureMouse;
   }

   return false;
}

// static
bool UI::wantsKeyboardInput()
{
   if (ImGui::GetCurrentContext())
   {
      return ImGui::GetIO().WantCaptureKeyboard;
   }

   return false;
}

// static
void UI::setIgnoreMouse(bool ignore)
{
   if (ImGui::GetCurrentContext())
   {
      ImGuiIO& io = ImGui::GetIO();
      if (ignore)
      {
         io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
      }
      else
      {
         io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
      }
   }
}

// static
bool UI::visible = true;

void UI::render(const GraphicsContext& graphicsContext, Scene& scene, const RenderCapabilities& capabilities, RenderSettings& settings, ResourceManager& resourceManager)
{
   static const float kTimeBetweenFrameRateUpdates = 1.0f / frameRates.size();

   if (!ImGui::GetCurrentContext())
   {
      return;
   }
   ImGuiIO& io = ImGui::GetIO();

   io.DeltaTime = scene.getRawDeltaTime();
   timeUntilFrameRateUpdate -= scene.getRawDeltaTime();
   if (timeUntilFrameRateUpdate <= 0.0f)
   {
      timeUntilFrameRateUpdate = glm::max(timeUntilFrameRateUpdate + kTimeBetweenFrameRateUpdates, 0.0f);

      maxFrameRate = glm::max(maxFrameRate, io.Framerate);
      frameRates[frameIndex] = io.Framerate;
      frameIndex = (frameIndex + 1) % frameRates.size();
   }

   ImGui_ImplGlfw_NewFrame();
   ImGui_ImplVulkan_NewFrame();
   ImGui::NewFrame();

   if (isVisible())
   {
      renderSceneWindow(graphicsContext, scene, capabilities, settings, resourceManager);
   }

   ImGui::Render();
}

void UI::renderSceneWindow(const GraphicsContext& graphicsContext, Scene& scene, const RenderCapabilities& capabilities, RenderSettings& settings, ResourceManager& resourceManager)
{
   ImGui::SetNextWindowSize(ImVec2(1150.0f, 0.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowContentSize(ImVec2(0.0f, kSceneWindowHeight));
   ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

   renderTime(scene);
   ImGui::SameLine();
   renderSettings(graphicsContext, capabilities, settings);
   ImGui::SameLine();
   renderEntityList(scene);
   ImGui::SameLine();
   renderSelectedEntity(resourceManager);

   ImGui::End();
}

void UI::renderTime(Scene& scene)
{
   ImGui::BeginChild("Time", ImVec2(ImGui::GetContentRegionAvail().x * 0.2f, kSceneWindowHeight), true, ImGuiWindowFlags_MenuBar);
   if (ImGui::BeginMenuBar())
   {
      if (ImGui::BeginMenu("Time", false))
      {
         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }

   float timeScale = scene.getTimeScale();
   if (ImGui::SliderFloat("Scale", &timeScale, 0.0f, 1.0f))
   {
      scene.setTimeScale(timeScale);
   }

   ImGui::PushItemWidth(-1);
   std::string overlay = std::to_string(static_cast<int>(ImGui::GetIO().Framerate + 0.5f)) + " FPS";
   ImGui::PlotLines("###Frame Rate", frameRates.data(), static_cast<int>(frameRates.size()), static_cast<int>(frameIndex), overlay.c_str(), 0.0f, maxFrameRate, ImVec2(0, 250.0f));
   ImGui::PopItemWidth();

   ImGui::EndChild();
}

void UI::renderSettings(const GraphicsContext& graphicsContext, const RenderCapabilities& capabilities, RenderSettings& settings)
{
   ImGui::BeginChild("Features", ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, kSceneWindowHeight), true, ImGuiWindowFlags_MenuBar);
   if (ImGui::BeginMenuBar())
   {
      if (ImGui::BeginMenu("Features", false))
      {
         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }

   const vk::PhysicalDeviceLimits& limits = graphicsContext.getPhysicalDeviceProperties().limits;
   vk::SampleCountFlags sampleCountFlags = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;

   static const std::array<vk::SampleCountFlagBits, 7> kSampleCountValues = { vk::SampleCountFlagBits::e1, vk::SampleCountFlagBits::e2, vk::SampleCountFlagBits::e4, vk::SampleCountFlagBits::e8, vk::SampleCountFlagBits::e16, vk::SampleCountFlagBits::e32, vk::SampleCountFlagBits::e64 };
   if (ImGui::BeginCombo("MSAA", getMSAAPreviewText(settings.msaaSamples)))
   {
      for (vk::SampleCountFlagBits sampleCount : kSampleCountValues)
      {
         if (sampleCount & sampleCountFlags)
         {
            bool isSelected = settings.msaaSamples == sampleCount;
            if (ImGui::Selectable(getMSAAPreviewText(sampleCount), isSelected))
            {
               settings.msaaSamples = sampleCount;
            }

            if (isSelected)
            {
               ImGui::SetItemDefaultFocus();
            }
         }
      }

      ImGui::EndCombo();
   }

   int ssaoQuality = Enum::cast(settings.ssaoQuality);
   ImGui::Combo("SSAO", &ssaoQuality, kRenderQualityNames.data(), static_cast<int>(kRenderQualityNames.size()));
   settings.ssaoQuality = static_cast<RenderQuality>(ssaoQuality);

   int bloomQuality = Enum::cast(settings.bloomQuality);
   ImGui::Combo("Bloom", &bloomQuality, kRenderQualityNames.data(), static_cast<int>(kRenderQualityNames.size()));
   settings.bloomQuality = static_cast<RenderQuality>(bloomQuality);

   int tonemappingAlgorithm = Enum::cast(settings.tonemapSettings.algorithm);
   ImGui::Combo("Tonemapping Algorithm", &tonemappingAlgorithm, kTonemappingAlgorithmNames.data(), static_cast<int>(kTonemappingAlgorithmNames.size()));
   settings.tonemapSettings.algorithm = static_cast<TonemappingAlgorithm>(tonemappingAlgorithm);

   ImGui::Checkbox("Show Tonemap Test Pattern", &settings.tonemapSettings.showTestPattern);

   ImGui::DragFloat("Bloom Strength", &settings.tonemapSettings.bloomStrength, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

   ImGui::DragFloat("Peak Brightness", &settings.tonemapSettings.peakBrightness, 1.0f, 100.0f, 10'000.0f);

   ImGui::SliderFloat("Toe", &settings.tonemapSettings.toe, 0.0f, 1.0f);
   ImGui::SliderFloat("Shoulder", &settings.tonemapSettings.shoulder, 0.0f, 1.0f);
   ImGui::SliderFloat("Hotspot", &settings.tonemapSettings.hotspot, 0.0f, 1.0f);
   ImGui::SliderFloat("Hue Preservation", &settings.tonemapSettings.huePreservation, 0.0f, 1.0f);

   ImGui::Checkbox("Limit Frame Rate", &settings.limitFrameRate);

   if (!capabilities.canPresentHDR)
   {
      ImGui::BeginDisabled();
   }
   ImGui::Checkbox("Present HDR", &settings.presentHDR);
   if (!capabilities.canPresentHDR)
   {
      ImGui::EndDisabled();
   }

   ImGui::EndChild();
}

void UI::renderEntityList(Scene& scene)
{
   ImGui::BeginChild("Entities", ImVec2(ImGui::GetContentRegionAvail().x * 0.28f, kSceneWindowHeight), true, ImGuiWindowFlags_MenuBar);
   if (ImGui::BeginMenuBar())
   {
      if (ImGui::BeginMenu("Entities", false))
      {
         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }

   std::vector<Entity> rootEntities;
   std::unordered_map<Entity, std::vector<Entity>> entityTree;
   Entity::forEach(scene, [this, &rootEntities, &entityTree](Entity entity)
   {
      bool hasParent = false;
      if (TransformComponent* transformComponent = entity.tryGetComponent<TransformComponent>())
      {
         if (Entity parent = transformComponent->parent)
         {
            hasParent = true;

            auto entityLocation = entityTree.find(parent);
            if (entityLocation == entityTree.end())
            {
               entityLocation = entityTree.emplace(parent, std::vector<Entity>{}).first;
            }

            entityLocation->second.push_back(entity);
         }
      }

      if (!hasParent)
      {
         rootEntities.push_back(entity);
      }
   });

   Entity lastSelectedEntity = selectedEntity;
   for (const Entity& rootEntity : rootEntities)
   {
      renderEntity(rootEntity, entityTree, selectedEntity);
   }

   if (selectedEntity != lastSelectedEntity)
   {
      selectedMeshSection = 0;
   }

   ImGui::EndChild();
}

void UI::renderSelectedEntity(ResourceManager& resourceManager)
{
   ImGui::BeginChild("SelectedEntity", ImVec2(0.0f, kSceneWindowHeight), true, ImGuiWindowFlags_MenuBar);
   if (ImGui::BeginMenuBar())
   {
      if (ImGui::BeginMenu((std::string(getEntityName(selectedEntity, "Selected Entity")) + "###SelectedEntity").c_str(), false))
      {
         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }

   ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.59f, 0.26f, 0.98f, 0.31f));
   ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.59f, 0.26f, 0.98f, 0.80f));
   ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.59f, 0.26f, 0.98f, 1.00f));
   if (selectedEntity)
   {
      if (ImGui::BeginTabBar("Components", ImGuiTabBarFlags_FittingPolicyScroll))
      {
         if (TransformComponent* transformComponent = selectedEntity.tryGetComponent<TransformComponent>())
         {
            renderTransformComponent(*transformComponent);
         }
         if (CameraComponent* cameraComponent = selectedEntity.tryGetComponent<CameraComponent>())
         {
            renderCameraComponent(*cameraComponent);
         }
         if (DirectionalLightComponent* directionalLightComponent = selectedEntity.tryGetComponent<DirectionalLightComponent>())
         {
            renderDirectionalLightComponent(*directionalLightComponent);
         }
         if (PointLightComponent* pointLightComponent = selectedEntity.tryGetComponent<PointLightComponent>())
         {
            renderPointLightComponent(*pointLightComponent);
         }
         if (SpotLightComponent* spotLightComponent = selectedEntity.tryGetComponent<SpotLightComponent>())
         {
            renderSpotLightComponent(*spotLightComponent);
         }
         if (MeshComponent* meshComponent = selectedEntity.tryGetComponent<MeshComponent>())
         {
            renderMeshComponent(*meshComponent, resourceManager, selectedMeshSection);
         }
         if (const SkyboxComponent* skyboxComponent = selectedEntity.tryGetComponent<SkyboxComponent>())
         {
            renderSkyboxComponent(*skyboxComponent, resourceManager);
         }

         ImGui::EndTabBar();
      }
   }
   ImGui::PopStyleColor(3);

   ImGui::EndChild();
}
