#pragma once

#include <string>
#include "../ps3emu/PPUThread.h"

uint64_t evalExpr(std::string const& expr, PPUThread* th);
