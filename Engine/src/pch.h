#pragma once

#include <iostream>
#include <functional>
#include <algorithm>
#include <array>
#include <set>
#include <optional>
#include <filesystem>
#include <fstream>
#include <map>
#include <cassert>
#include <cstring>
#include <unordered_map>

// GLM configuration - Enable experimental features before including GLM
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// GLM includes for 3D math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "utils.h"
#include "Logger.h"

#include <vulkan/vulkan_core.h>