#pragma once
#include "pch.h"

namespace Utils
{
	std::vector<std::uint8_t> ReadFile(const std::filesystem::path& shaderPath);
}
