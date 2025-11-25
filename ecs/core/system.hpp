#pragma once

#include <vector>

#include "types.hpp"

namespace ecs {

// Base system: holds required signature and matching entities.
class System {
public:
    virtual ~System() = default;
    Signature signature{};
    std::vector<Entity> entities{};
};

} // namespace ecs

