#pragma once

#include "Math/Transform.h"

#include <glm/glm.hpp>

#include <vector>

class Material;
class Mesh;
class View;

struct MeshRenderInfo
{
   Transform transform;
   glm::mat4 localToWorld;

   std::vector<uint32_t> visibleOpaqueSections;
   std::vector<uint32_t> visibleTranslucentSections;
   std::vector<const Material*> materials;
   const Mesh* mesh = nullptr;

   MeshRenderInfo(const Mesh& m, const Transform& t)
      : mesh(&m)
      , transform(t)
      , localToWorld(t.toMatrix())
   {
   }
};

struct LightRenderInfo
{
   glm::vec3 color;
};

struct PointLightRenderInfo : public LightRenderInfo
{
   glm::vec3 position;
   float radius = 0.0f;
};

struct SpotLightRenderInfo : public LightRenderInfo
{
   glm::vec3 position;
   glm::vec3 direction;
   float radius = 0.0f;
   float beamAngle = 0.0f;
   float cutoffAngle = 0.0f;
};

struct DirectionalLightRenderInfo : public LightRenderInfo
{
   glm::vec3 direction;
};

struct SceneRenderInfo
{
   const View& view;

   std::vector<MeshRenderInfo> meshes;

   std::vector<PointLightRenderInfo> pointLights;
   std::vector<SpotLightRenderInfo> spotLights;
   std::vector<DirectionalLightRenderInfo> directionalLights;

   SceneRenderInfo(const View& v)
      : view(v)
   {
   }
};
