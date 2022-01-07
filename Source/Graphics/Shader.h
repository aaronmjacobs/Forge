#pragma once

#include "Graphics/GraphicsResource.h"

#include <span>
#include <vector>

class ResourceManager;

class Shader : public GraphicsResource
{
public:
   struct InitializationInfo
   {
      const char* vertShaderModulePath = nullptr;
      const char* fragShaderModulePath = nullptr;

      const char* vertShaderModuleEntryPoint = "main";
      const char* fragShaderModuleEntryPoint = "main";

      std::span<const vk::SpecializationInfo> specializationInfo;
   };

   Shader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const InitializationInfo& info);

protected:
   std::vector<vk::PipelineShaderStageCreateInfo> getStagesForPermutation(uint32_t permutationIndex) const;

private:
   std::vector<vk::PipelineShaderStageCreateInfo> vertStageCreateInfo;
   std::vector<vk::PipelineShaderStageCreateInfo> fragStageCreateInfo;
};
