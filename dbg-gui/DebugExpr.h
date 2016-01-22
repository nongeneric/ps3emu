#pragma once

#include <string>

class PPUThread;
class SPUThread;

template <typename TH>
uint64_t evalExpr(std::string const& text, TH* th);