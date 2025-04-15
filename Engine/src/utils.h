#pragma once
#include "pch.h"

namespace Utils
{
	bool streq(gsl::czstring s1, gsl::czstring s2);

	std::vector<std::uint8_t> ReadFile(const std::filesystem::path& shaderPath);
}
