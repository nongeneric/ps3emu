#pragma once

#include <set>
#include <algorithm>

template <typename T>
std::set<T> operator-(std::set<T> const& left, std::set<T> const& right) {
    std::set<T> diff;
    std::set_difference(begin(left),
                        end(left),
                        begin(right),
                        end(right),
                        std::inserter(diff, begin(diff)));
    return diff;
}

template <typename T>
std::set<T> operator+(std::set<T> const& left, std::set<T> const& right) {
    std::set<T> diff;
    std::set_union(begin(left),
                   end(left),
                   begin(right),
                   end(right),
                   std::inserter(diff, begin(diff)));
    return diff;
}

template <typename T>
std::set<T> toSet(std::vector<T> container) {
    return std::set<T>(begin(container), end(container));
}
