#pragma once

#include <functional>
#include <vector>
#include "types.hpp"

namespace ecs {

class Coordinator;

struct SystemData {
    std::vector<Entity> entities{};
};

using SystemUpdateFunc = std::function<void(Coordinator&, const std::vector<Entity>&, float)>;
using SystemUpdateFuncNoTime = std::function<void(Coordinator&, const std::vector<Entity>&)>;

} // namespace ecs
