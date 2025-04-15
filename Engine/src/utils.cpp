#include <pch.h>
#include <utils.h>

namespace Utils
{
	bool streq(const gsl::czstring s1, const gsl::czstring s2)
	{
		return std::strcmp(s1, s2) == 0;
	}

	std::vector<std::uint8_t> Utils::ReadFile(const std::filesystem::path& shaderPath)
	{
		// Qualifiers are not needed because of ADL
		if(!exists(shaderPath))
			return {};

		if(!is_regular_file(shaderPath))
			return {};

		std::ifstream file(shaderPath, std::ios::binary);
		if(!file.is_open())
			return {};

		const uint32_t size = file_size(shaderPath);
		std::vector<uint8_t> buffer(size);
		file.read(reinterpret_cast<char*>(buffer.data()), size);
		return buffer;
	}
}
