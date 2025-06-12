#include <pch.h>
#include <utils.h>

namespace Utils
{

	std::vector<std::uint8_t> Utils::ReadFile(const std::filesystem::path& shaderPath)
	{
		// Qualifiers are not needed because of ADL
		if(!exists(shaderPath))
		{
			//spdlog::error("shaderPath not found!");
			return {};
		}

		if(!is_regular_file(shaderPath))
		{
			//spdlog::error("files are not regular file!");
			return {};
		}

		std::ifstream file(shaderPath, std::ios::binary);
		if(!file.is_open())
		{
			//spdlog::error("file could not be opened!");
			return {};
		}

		const auto size = file_size(shaderPath);
		std::vector<uint8_t> buffer(size);
		file.read(reinterpret_cast<char*>(buffer.data()), (long long)size);
		return buffer;
	}
}
