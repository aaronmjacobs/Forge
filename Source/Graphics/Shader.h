#pragma once

#include "Core/Delegate.h"
#include "Core/Features.h"

#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceTypes.h"

#include <filesystem>
#include <string>
#include <span>
#include <vector>

class ResourceManager;

class Shader : public GraphicsResource
{
public:
   struct InitializationInfo
   {
      std::filesystem::path vertShaderModulePath;
      std::filesystem::path fragShaderModulePath;

      std::string vertShaderModuleEntryPoint = "main";
      std::string fragShaderModuleEntryPoint = "main";

      std::span<const vk::SpecializationInfo> specializationInfo;
   };

   Shader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const InitializationInfo& info);
   virtual ~Shader();

   using InitializeDelegate = MulticastDelegate<void>;

   DelegateHandle addOnInitialize(InitializeDelegate::FuncType&& function);
   void removeOnInitialize(DelegateHandle& handle);

protected:
   std::vector<vk::PipelineShaderStageCreateInfo> getStagesForPermutation(uint32_t permutationIndex) const;

private:
   void initializeStageCreateInfo();

   InitializationInfo initializationInfo;
   InitializeDelegate onInitialize;

   StrongShaderModuleHandle vertShaderModuleHandle;
   StrongShaderModuleHandle fragShaderModuleHandle;

   std::vector<vk::PipelineShaderStageCreateInfo> vertStageCreateInfo;
   std::vector<vk::PipelineShaderStageCreateInfo> fragStageCreateInfo;

#if FORGE_WITH_SHADER_HOT_RELOADING
   DelegateHandle hotReloadDelegateHandle;
#endif // FORGE_WITH_SHADER_HOT_RELOADING
};
