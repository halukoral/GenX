#pragma once
#include <memory>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

// GLM experimental features must be enabled before any GLM includes
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifdef GX_DEBUG
	#define GX_DEBUGBREAK() __debugbreak()
	#define GX_ENABLE_ASSERTS
	inline bool EnableValidationLayers = true;
#else
#define GX_DEBUGBREAK()
inline bool EnableValidationLayers = false;
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