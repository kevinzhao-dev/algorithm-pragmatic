// scale.hpp — fork2 plus a deterministic work/span meter, in the spirit of Cilkscale.
//
// Parallelize with scale::fork2 instead of practice::fork2. Normally it simply forwards there.
// Inside scale::measure() it runs both branches on the calling thread and accumulates
//     work = total charged units,      span = longest chain of charged units,
// so the available parallelism work / span does not depend on the machine or its current load.
//
// charge(n) is called by whatever you want to count; the tests charge one unit per pair test.
#pragma once

#include <algorithm>
#include <cstdint>
#include <utility>

#include "forkjoin.hpp"

namespace scale {

struct Report {
    std::uint64_t work = 0;
    std::uint64_t span = 0;

    double parallelism() const {
        return span == 0 ? 0.0 : static_cast<double>(work) / static_cast<double>(span);
    }
};

namespace detail {
// Written only outside parallel regions; read concurrently but never written concurrently.
inline bool measuring = false;
inline std::uint64_t work = 0;
inline std::uint64_t span = 0;
}  // namespace detail

template <typename F, typename G>
void fork2(F&& f, G&& g) {
    if (!detail::measuring) {
        practice::fork2(std::forward<F>(f), std::forward<G>(g));
        return;
    }
    const std::uint64_t before = detail::span;
    detail::span = 0;
    f();
    const std::uint64_t left = detail::span;
    detail::span = 0;
    g();
    detail::span = before + std::max(left, detail::span);
}

inline void charge(std::uint64_t units = 1) {
    if (detail::measuring) {
        detail::work += units;
        detail::span += units;
    }
}

// Call only from outside any parallel region.
template <typename Body>
Report measure(Body&& body) {
    detail::measuring = true;
    detail::work = 0;
    detail::span = 0;
    body();
    detail::measuring = false;
    return {detail::work, detail::span};
}

}  // namespace scale
