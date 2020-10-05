#pragma once

#include "ps3emu/enum.h"
#include <vector>
#include <tuple>
#include <boost/algorithm/string.hpp>

template<typename E>
void patchEnumValues(std::string& text) {
    auto values = enum_traits<E>::values();
    auto names = enum_traits<E>::names();
    auto name = enum_traits<E>::name();
    std::vector<std::tuple<std::string, unsigned>> pairs;
    for (auto i = 0u; i < values.size(); ++i) {
        pairs.push_back(std::make_tuple(std::string(names[i]), (unsigned)values[i]));
    }
    std::sort(begin(pairs), end(pairs), [&](auto l, auto r) {
        return std::get<0>(l).size() > std::get<0>(r).size();
    });
    for (auto i = 0u; i < pairs.size(); ++i) {
        boost::replace_all(text,
                           sformat("{}::{}", name, std::get<0>(pairs[i])),
                           sformat("{}", std::get<1>(pairs[i])));
    }
}
