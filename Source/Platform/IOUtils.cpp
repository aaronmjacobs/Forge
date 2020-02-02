#include "Platform/IOUtils.h"

#include "Core/Assert.h"
#include "Platform/OSUtils.h"

#include <fstream>

namespace
{
   std::optional<std::filesystem::path> getAbsolutePath(const std::optional<std::filesystem::path>& base, const std::filesystem::path& relativePath)
   {
      ASSERT(relativePath.is_relative());

      std::optional<std::filesystem::path> absolutePath;
      if (base)
      {
         ASSERT(base->is_absolute());

         std::error_code errorCode;
         std::filesystem::path canonicalAbsolutePath = std::filesystem::weakly_canonical(*base / relativePath, errorCode);
         if (!errorCode)
         {
            absolutePath = canonicalAbsolutePath;
         }
      }

      return absolutePath;
   }
}

namespace IOUtils
{
   std::optional<std::string> readTextFile(const std::filesystem::path& path)
   {
      ASSERT(!path.empty(), "Trying to read text file with empty path");
      ASSERT(path.has_filename());

      std::optional<std::string> data;
      std::ifstream in(path);
      if (in)
      {
         data = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
      }

      return data;
   }

   std::optional<std::vector<uint8_t>> readBinaryFile(const std::filesystem::path& path)
   {
      ASSERT(!path.empty(), "Trying to read binary file with empty path");
      ASSERT(path.has_filename());

      std::optional<std::vector<uint8_t>> data;
      std::ifstream in(path, std::ifstream::binary);
      if (in)
      {
         std::streampos start = in.tellg();
         in.seekg(0, std::ios_base::end);
         std::streamoff size = in.tellg() - start;
         ASSERT(size >= 0, "Invalid file size");
         in.seekg(0, std::ios_base::beg);

         data = std::vector<uint8_t>(static_cast<size_t>(size));
         in.read(reinterpret_cast<char*>(data->data()), size);
      }

      return data;
   }

   bool writeTextFile(const std::filesystem::path& path, std::string_view data)
   {
      ASSERT(!path.empty(), "Trying to write text file with empty path");
      ASSERT(path.has_filename());

      std::error_code errorCode;
      if (std::filesystem::create_directories(path.parent_path(), errorCode))
      {
         std::ofstream out(path);
         if (out)
         {
            out << data;
            return true;
         }
      }

      return false;
   }

   bool writeBinaryFile(const std::filesystem::path& path, const std::vector<uint8_t>& data)
   {
      ASSERT(!path.empty(), "Trying to write binary file with empty path");
      ASSERT(path.has_filename());

      std::error_code errorCode;
      if (std::filesystem::create_directories(path.parent_path(), errorCode))
      {
         std::ofstream out(path, std::ofstream::binary);
         if (out)
         {
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
            return true;
         }
      }

      return false;
   }

   std::optional<std::filesystem::path> getResourceDirectory()
   {
      static const char* kResourceFolderName = "Resources";
      static std::optional<std::filesystem::path> cachedResourceDirectory;

      if (cachedResourceDirectory)
      {
         return *cachedResourceDirectory;
      }

      std::optional<std::filesystem::path> resourceDirectory;
      if (std::optional<std::filesystem::path> executeablePath = OSUtils::getExecutablePath())
      {
         std::filesystem::path executableDirectory = executeablePath->parent_path();

         // The executable directory location depends on the build environment
         std::filesystem::path potentialResourceDirectory = executableDirectory / ".." / kResourceFolderName;
         if (!std::filesystem::exists(potentialResourceDirectory))
         {
            potentialResourceDirectory = executableDirectory / ".." / ".." / kResourceFolderName;
         }

         if (std::filesystem::exists(potentialResourceDirectory))
         {
            std::error_code errorCode;
            std::filesystem::path canonicalResourceDirectory = std::filesystem::canonical(potentialResourceDirectory, errorCode);
            if (!errorCode)
            {
               resourceDirectory = canonicalResourceDirectory;
            }
         }
      }

      cachedResourceDirectory = resourceDirectory;
      return resourceDirectory;
   }

   std::optional<std::filesystem::path> getAbsoluteResourcePath(const std::filesystem::path& relativePath)
   {
      return getAbsolutePath(getResourceDirectory(), relativePath);
   }

   std::optional<std::filesystem::path> getAbsoluteAppDataPath(std::string_view appName, const std::filesystem::path& relativePath)
   {
      return getAbsolutePath(OSUtils::getAppDataDirectory(appName), relativePath);
   }
}
