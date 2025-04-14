#include <pch.h>
#include <utils.h>

bool streq(const gsl::czstring s1, const gsl::czstring s2)
{
	return std::strcmp(s1, s2) == 0;
}
