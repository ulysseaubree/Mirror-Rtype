#pragma once

#include <bitset>
#include <cstdint>

namespace ecs {

using Entity = std::uint32_t;
inline constexpr Entity MAX_ENTITIES = 5000;

using ComponentType = std::uint8_t;
inline constexpr ComponentType MAX_COMPONENTS = 64;

using Signature = std::bitset<MAX_COMPONENTS>;

} // namespace ecs

