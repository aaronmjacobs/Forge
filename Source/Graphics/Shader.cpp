#include "Graphics/Shader.h"

#include "Core/Assert.h"

#include "Resources/ResourceManager.h"

#include <filesystem>
#include <utility>

namespace
{
   StrongShaderModuleHandle loadShaderModule(ResourceManager& resourceManager, const std::string& name, const char* extension)
   {
      if (name.empty())
      {
         return StrongShaderModuleHandle{};
      }

      std::filesystem::path path = "Resources/Shaders/" + name + "." + extension + ".spv";
      StrongShaderModuleHandle handle = resourceManager.loadShaderModule(path);
      const ShaderModule* shaderModule = resourceManager.getShaderModule(handle);
      if (!shaderModule)
      {
         throw std::runtime_error(std::string("Failed to load shader module: ") + path.string());
      }

      return handle;
   }
}

Shader::Shader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const InitializationInfo& info)
   : GraphicsResource(graphicsContext)
   , initializationInfo(info)
   , vertShaderModuleHandle(loadShaderModule(resourceManager, info.vertShaderModuleName, "vert"))
   , fragShaderModuleHandle(loadShaderModule(resourceManager, info.fragShaderModuleName, "frag"))
{
   initializeStageCreateInfo();

#if FORGE_WITH_SHADER_HOT_RELOADING
   hotReloadDelegateHandle = resourceManager.addShaderModuleHotReloadDelegate([this](ShaderModuleHandle hotReloadedShaderModuleHandle)
   {
      if (hotReloadedShaderModuleHandle == vertShaderModuleHandle || hotReloadedShaderModuleHandle == fragShaderModuleHandle)
      {
         initializeStageCreateInfo();
      }
   });
#endif // FORGE_WITH_SHADER_HOT_RELOADING
}

Shader::~Shader()
{
#if FORGE_WITH_SHADER_HOT_RELOADING
   ResourceManager* resourceManager = vertShaderModuleHandle.getResourceManager();
   if (!resourceManager)
   {
      resourceManager = fragShaderModuleHandle.getResourceManager();
   }

   if (resourceManager)
   {
      resourceManager->removeShaderModuleHotReloadDelegate(hotReloadDelegateHandle);
   }
#endif // FORGE_WITH_SHADER_HOT_RELOADING
}

DelegateHandle Shader::addOnInitialize(InitializeDelegate::FuncType&& function)
{
   return onInitialize.add(std::move(function));
}

void Shader::removeOnInitialize(DelegateHandle& handle)
{
   if (onInitialize.remove(handle))
   {
      handle.invalidate();
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

void Shader::initializeStageCreateInfo()
{
   vertStageCreateInfo.clear();
   if (const ShaderModule* vertShaderModule = vertShaderModuleHandle.getResource())
   {
      vk::PipelineShaderStageCreateInfo vertCreateInfo = vk::PipelineShaderStageCreateInfo()
         .setStage(vk::ShaderStageFlagBits::eVertex)
         .setModule(vertShaderModule->getShaderModule())
         .setPName(initializationInfo.vertShaderModuleEntryPoint.c_str());

      if (initializationInfo.specializationInfo.empty())
      {
         vertStageCreateInfo.push_back(vertCreateInfo);
      }
      else
      {
         vertStageCreateInfo.resize(initializationInfo.specializationInfo.size());
         for (std::size_t i = 0; i < initializationInfo.specializationInfo.size(); ++i)
         {
            vertStageCreateInfo[i] = vertCreateInfo.setPSpecializationInfo(&initializationInfo.specializationInfo[i]);
         }
      }
   }

   fragStageCreateInfo.clear();
   if (const ShaderModule* fragShaderModule = fragShaderModuleHandle.getResource())
   {
      vk::PipelineShaderStageCreateInfo fragCreateInfo = vk::PipelineShaderStageCreateInfo()
         .setStage(vk::ShaderStageFlagBits::eFragment)
         .setModule(fragShaderModule->getShaderModule())
         .setPName(initializationInfo.fragShaderModuleEntryPoint.c_str());

      if (initializationInfo.specializationInfo.empty())
      {
         fragStageCreateInfo.push_back(fragCreateInfo);
      }
      else
      {
         fragStageCreateInfo.resize(initializationInfo.specializationInfo.size());
         for (std::size_t i = 0; i < initializationInfo.specializationInfo.size(); ++i)
         {
            fragStageCreateInfo[i] = fragCreateInfo.setPSpecializationInfo(&initializationInfo.specializationInfo[i]);
         }
      }
   }

   onInitialize.broadcast();
}
