#include "Graphics/Shader.h"

#include "Core/Assert.h"

#include "Resources/ResourceManager.h"

#include <utility>

namespace
{
   StrongShaderModuleHandle loadShaderModule(ResourceManager& resourceManager, const char* path)
   {
      if (!path)
      {
         return StrongShaderModuleHandle{};
      }

      StrongShaderModuleHandle handle = resourceManager.loadShaderModule(path);
      const ShaderModule* shaderModule = resourceManager.getShaderModule(handle);
      if (!shaderModule)
      {
         throw std::runtime_error(std::string("Failed to load shader module: ") + path);
      }

      return handle;
   }
}

Shader::Shader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const InitializationInfo& info)
   : GraphicsResource(graphicsContext)
{
   if (StrongShaderModuleHandle vertShaderModuleHandle = loadShaderModule(resourceManager, info.vertShaderModulePath))
   {
      const ShaderModule* vertShaderModule = resourceManager.getShaderModule(vertShaderModuleHandle);
      ASSERT(vertShaderModule);

      vk::PipelineShaderStageCreateInfo vertCreateInfo = vk::PipelineShaderStageCreateInfo()
         .setStage(vk::ShaderStageFlagBits::eVertex)
         .setModule(vertShaderModule->getShaderModule())
         .setPName(info.vertShaderModuleEntryPoint);

      if (info.specializationInfo.empty())
      {
         vertStageCreateInfo.push_back(vertCreateInfo);
      }
      else
      {
         vertStageCreateInfo.resize(info.specializationInfo.size());
         for (std::size_t i = 0; i < info.specializationInfo.size(); ++i)
         {
            vertStageCreateInfo[i] = vertCreateInfo.setPSpecializationInfo(&info.specializationInfo[i]);
         }
      }

      shaderModuleHandles.push_back(vertShaderModuleHandle);
   }

   if (StrongShaderModuleHandle fragShaderModuleHandle = loadShaderModule(resourceManager, info.fragShaderModulePath))
   {
      const ShaderModule* fragShaderModule = resourceManager.getShaderModule(fragShaderModuleHandle);
      ASSERT(fragShaderModule);

      vk::PipelineShaderStageCreateInfo fragCreateInfo = vk::PipelineShaderStageCreateInfo()
         .setStage(vk::ShaderStageFlagBits::eFragment)
         .setModule(fragShaderModule->getShaderModule())
         .setPName(info.fragShaderModuleEntryPoint);

      if (info.specializationInfo.empty())
      {
         fragStageCreateInfo.push_back(fragCreateInfo);
      }
      else
      {
         fragStageCreateInfo.resize(info.specializationInfo.size());
         for (std::size_t i = 0; i < info.specializationInfo.size(); ++i)
         {
            fragStageCreateInfo[i] = fragCreateInfo.setPSpecializationInfo(&info.specializationInfo[i]);
         }
      }

      shaderModuleHandles.push_back(fragShaderModuleHandle);
   }
}

std::vector<vk::PipelineShaderStageCreateInfo> Shader::getStagesForPermutation(uint32_t permutationIndex) const
{
   std::vector<vk::PipelineShaderStageCreateInfo> stages;
   stages.reserve((vertStageCreateInfo.empty() ? 0 : 1) + (fragStageCreateInfo.empty() ? 0 : 1));

   if (permutationIndex < vertStageCreateInfo.size())
   {
      stages.push_back(vertStageCreateInfo[permutationIndex]);
   }
   if (permutationIndex < fragStageCreateInfo.size())
   {
      stages.push_back(fragStageCreateInfo[permutationIndex]);
   }

   return stages;
}
