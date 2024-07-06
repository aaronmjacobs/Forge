#pragma once

#include "Resources/ResourceLoader.h"

#include "Core/Delegate.h"
#include "Core/Features.h"

#include "Graphics/ShaderModule.h"

#include <filesystem>
#include <string>

#if FORGE_WITH_SHADER_HOT_RELOADING
#  include "Core/Task.h"

#  include <PlatformUtils/OSUtils.h>

#  include <unordered_map>
#  include <unordered_set>
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
   void onFileModified(OSUtils::DirectoryWatchEvent event, const std::filesystem::path& directory, const std::filesystem::path& file);
   void pollCompilationResults();

   void compile(const std::filesystem::path& sourcePath);
   void hotReload(const std::string& canonicalBinaryPathString, const std::vector<uint8_t>& code);
   void updateIncludeMap(const std::filesystem::path& binaryPath);
   void updateIncludesRecursive(const std::string& sourcePathString, const std::filesystem::path& currentPath, int recursionDepth);

   struct CompilationResult
   {
      std::optional<OSUtils::ProcessExitInfo> exitInfo;
      std::optional<std::vector<uint8_t>> code;
   };

   HotReloadDelegate hotReloadDelegate;
   std::unordered_map<std::string, Task<CompilationResult>> compilationResults;
   std::unordered_map<std::string, std::unordered_set<std::string>> includeMap;

   OSUtils::DirectoryWatcher shaderSourceDirectoryWatcher;
#endif // FORGE_WITH_SHADER_HOT_RELOADING
};
