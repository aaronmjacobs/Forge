#pragma once

#include "Core/Math/Transform.h"

#include <glm/glm.hpp>

#include <vector>

class Material;
class Mesh;
class View;

struct MeshRenderInfo
{
   Transform transform;
   glm::mat4 localToWorld;

   std::vector<bool> visibilityMask;
   std::vector<const Material*> materials;
   const Mesh& mesh;

   MeshRenderInfo(const Mesh& m, const Transform& t)
      : mesh(m)
      , transform(t)
      , localToWorld(t.toMatrix())
   {
   }
};

struct SceneRenderInfo
{
   const View& view;
   std::vector<MeshRenderInfo> meshes;

   SceneRenderInfo(const View& v)
      : view(v)
   {
   }
};
