#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace IOUtils
{
   std::optional<std::string> readTextFile(const std::filesystem::path& path);
   std::optional<std::vector<uint8_t>> readBinaryFile(const std::filesystem::path& path);

   bool writeTextFile(const std::filesystem::path&, std::string_view data);
   bool writeBinaryFile(const std::filesystem::path&, const std::vector<uint8_t>& data);

   std::optional<std::filesystem::path> getResourceDirectory();

   std::optional<std::filesystem::path> getAbsoluteResourcePath(const std::filesystem::path& relativePath);
   std::optional<std::filesystem::path> getAbsoluteAppDataPath(std::string_view appName, const std::filesystem::path& relativePath);
}
