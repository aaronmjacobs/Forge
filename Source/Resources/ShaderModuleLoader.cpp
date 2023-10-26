#include "Resources/ShaderModuleLoader.h"

#include "Core/Log.h"

#include "Graphics/DebugUtils.h"

#include <PlatformUtils/IOUtils.h>
#include <PlatformUtils/OSUtils.h>

#include <sstream>
#include <utility>

namespace
{
#if FORGE_WITH_SHADER_HOT_RELOADING
   std::optional<std::filesystem::path> findGLSLC()
   {
#if FORGE_PLATFORM_WINDOWS
      static const char kPathSeparator = ';';
      static const char* kExecutableName = "glslc.exe";
#else
      static const char kPathSeparator = ':';
      static const char* kExecutableName = "glslc";
#endif

#if FORGE_PLATFORM_WINDOWS
      // Windows has a special environment variable, so we check that first
      if (const char* sdkPath = std::getenv("VULKAN_SDK"))
      {
         std::filesystem::path glslcPath = std::filesystem::path(sdkPath) / "Bin" / kExecutableName;
         if (std::filesystem::is_regular_file(glslcPath))
         {
            return glslcPath;
         }
      }
#endif // FORGE_PLATFORM_WINDOWS

      if (const char* path = std::getenv("PATH"))
      {
         std::istringstream split(path);

         std::string pathEntry;
         while (std::getline(split, pathEntry, kPathSeparator))
         {
            std::filesystem::path glslcPath = std::filesystem::path(pathEntry) / kExecutableName;
            if (std::filesystem::is_regular_file(glslcPath))
            {
               return glslcPath;
            }
         }
      }

#if FORGE_PLATFORM_MACOS
      // Xcode has its own sanitized PATH, which doesn't include /usr/local/bin, so we need to manually check it
      std::filesystem::path localBinGlslcPath = std::filesystem::path("/usr/local/bin") / kExecutableName;
      if (std::filesystem::is_regular_file(localBinGlslcPath))
      {
         return localBinGlslcPath;
      }
#endif // FORGE_PLATFORM_MACOS

      return std::nullopt;
   }

   std::optional<std::filesystem::path> getBinaryPath(const std::filesystem::path& sourcePath)
   {
      return IOUtils::getAboluteProjectPath("Resources/Shaders" / sourcePath.filename().concat(".spv"));
   }
#endif // FORGE_WITH_SHADER_HOT_RELOADING
}

ShaderModuleLoader::ShaderModuleLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceLoader(graphicsContext, owningResourceManager)
{
#if FORGE_WITH_SHADER_HOT_RELOADING
   if (std::optional<std::filesystem::path> shaderSourceDirectory = IOUtils::getAboluteProjectPath("Shaders"))
   {
      shaderSourceDirectoryWatcher.addWatch(*shaderSourceDirectory, false, [this](OSUtils::DirectoryWatchEvent event, const std::filesystem::path& directory, const std::filesystem::path& file)
      {
         if (event == OSUtils::DirectoryWatchEvent::Modify)
         {
            compile(directory / file);
         }
      });
   }
#endif // FORGE_WITH_SHADER_HOT_RELOADING
}

void ShaderModuleLoader::update()
{
#if FORGE_WITH_SHADER_HOT_RELOADING
   pollCompilationResults();
   shaderSourceDirectoryWatcher.update();
#endif // FORGE_WITH_SHADER_HOT_RELOADING
}

ShaderModuleHandle ShaderModuleLoader::load(const std::filesystem::path& path)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceLoadHelpers::makeCanonical(path))
   {
      std::string canonicalPathString = canonicalPath->string();
      if (Handle cachedHandle = container.findHandle(canonicalPathString))
      {
         return cachedHandle;
      }

      std::optional<std::vector<uint8_t>> code = IOUtils::readBinaryFile(*canonicalPath);
      if (code.has_value() && code->size() > 0)
      {
         ShaderModuleHandle handle = container.emplace(canonicalPathString, context, *code);
         NAME_POINTER(context.getDevice(), get(handle), ResourceLoadHelpers::getName(*canonicalPath));

         return handle;
      }
   }

   return ShaderModuleHandle{};
}

#if FORGE_WITH_SHADER_HOT_RELOADING
DelegateHandle ShaderModuleLoader::addHotReloadDelegate(HotReloadDelegate::FuncType&& function)
{
   return hotReloadDelegate.add(std::move(function));
}

void ShaderModuleLoader::removeHotReloadDelegate(DelegateHandle& handle)
{
   if (hotReloadDelegate.remove(handle))
   {
      handle.invalidate();
   }
}

void ShaderModuleLoader::pollCompilationResults()
{
   for (auto itr = compilationResults.begin(); itr != compilationResults.end();)
   {
      const std::string& canonicalPathString = itr->first;
      std::future<CompilationResult>& future = itr->second;

      ASSERT(future.valid());

      if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
      {
         CompilationResult compilationResult = future.get();
         if (compilationResult.code && compilationResult.code->size() > 0)
         {
            hotReload(canonicalPathString, compilationResult.code.value());
         }
         else
         {
            if (compilationResult.exitInfo)
            {
               LOG_ERROR("glslc failed:\n" << compilationResult.exitInfo->stdErr);
            }
            else
            {
               LOG_ERROR("Failed to launch glslc");
            }
         }

         itr = compilationResults.erase(itr);
      }
      else
      {
         ++itr;
      }
   }
}

void ShaderModuleLoader::compile(const std::filesystem::path& sourcePath)
{
   static const std::optional<std::filesystem::path> glslcPath = findGLSLC();
   if (!glslcPath)
   {
      return;
   }

   if (std::optional<std::filesystem::path> binaryPath = getBinaryPath(sourcePath))
   {
      if (std::optional<std::filesystem::path> canonicalPath = ResourceLoadHelpers::makeCanonical(*binaryPath))
      {
         std::string canonicalPathString = canonicalPath->string();
         if (compilationResults.contains(canonicalPathString))
         {
            // Compilation already in progress
            return;
         }

         OSUtils::ProcessStartInfo glslcStartInfo;
         glslcStartInfo.path = *glslcPath;
         glslcStartInfo.args = { "-o", binaryPath->string(), sourcePath.string() };
         glslcStartInfo.readOutput = true;

         compilationResults.emplace(canonicalPathString, std::async(std::launch::async, [canonicalPathString, processStartInfo = std::move(glslcStartInfo)]()
         {
            CompilationResult result;
            result.exitInfo = OSUtils::executeProcess(processStartInfo);

            if (result.exitInfo && result.exitInfo->exitCode == 0)
            {
               result.code = IOUtils::readBinaryFile(canonicalPathString);
            }

            return result;
         }));
      }
   }
}

void ShaderModuleLoader::hotReload(const std::string& canonicalPathString, const std::vector<uint8_t>& code)
{
   if (Handle cachedHandle = container.findHandle(canonicalPathString))
   {
      container.replace(cachedHandle, context, code);
      hotReloadDelegate.broadcast(cachedHandle);
   }
}
#endif // FORGE_WITH_SHADER_HOT_RELOADING
