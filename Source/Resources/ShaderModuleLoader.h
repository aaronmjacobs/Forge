#pragma once

#include "Resources/ResourceLoader.h"

#include "Core/Delegate.h"
#include "Core/Features.h"

#include "Graphics/ShaderModule.h"

#include <filesystem>
#include <string>

#if FORGE_WITH_SHADER_HOT_RELOADING
#  include <PlatformUtils/OSUtils.h>

#  include <future>
#  include <unordered_map>
#endif // FORGE_WITH_SHADER_HOT_RELOADING

class ShaderModuleLoader : public ResourceLoader<std::string, ShaderModule>
{
public:
   ShaderModuleLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   void update();

   ShaderModuleHandle load(const std::filesystem::path& path);

public:
#if FORGE_WITH_SHADER_HOT_RELOADING
   using HotReloadDelegate = MulticastDelegate<void, ShaderModuleHandle>;

   DelegateHandle addHotReloadDelegate(HotReloadDelegate::FuncType&& function);
   void removeHotReloadDelegate(DelegateHandle& handle);
#endif // FORGE_WITH_SHADER_HOT_RELOADING

private:
#if FORGE_WITH_SHADER_HOT_RELOADING
   void pollCompilationResults();
   void compile(const std::filesystem::path& sourcePath);
   void hotReload(const std::string& canonicalPathString, const std::vector<uint8_t>& code);

   struct CompilationResult
   {
      std::optional<OSUtils::ProcessExitInfo> exitInfo;
      std::optional<std::vector<uint8_t>> code;
   };

   HotReloadDelegate hotReloadDelegate;
   std::unordered_map<std::string, std::future<CompilationResult>> compilationResults;

   OSUtils::DirectoryWatcher shaderSourceDirectoryWatcher;
#endif // FORGE_WITH_SHADER_HOT_RELOADING
};
