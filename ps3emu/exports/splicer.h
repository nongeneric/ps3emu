#pragma once

#include "ps3emu/int.h"
#include <functional>
#include <optional>

void spliceFunction(uint32_t ea, std::function<void()> handler);
std::optional<std::string> fnidToName(uint32_t fnid);
