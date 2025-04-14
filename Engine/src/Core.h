#pragma once
#include <memory>

#ifdef GX_DEBUG

	#define GX_DEBUGBREAK() __debugbreak()
	#define GX_ENABLE_ASSERTS
	bool EnableValidationLayers = true;

#else

	#define GX_DEBUGBREAK()
	constexpr bool EnableValidationLayers = false;

#endif

#define GX_EXPAND_MACRO(x) x
#define GX_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)
#define GX_BIND(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using Weak = std::weak_ptr<T>;
