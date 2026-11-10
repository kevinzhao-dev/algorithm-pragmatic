// SPOILER: contains a complete Meter, which answers 6.172/0501 Stage 2. Finish 0501 first.
//
// 01_exec.hpp — write a parallel algorithm once, run it two ways.
//
//     template <typename Exec>
//     void my_algorithm(Exec& exec, ...) {
//         exec.charge(1);                 // one unit of work done here
//         exec.fork2([&] { left(); }, [&] { right(); });
//     }
//
// exec::Meter runs serially and computes exact work and span, so tests can assert asymptotic
// bounds deterministically. exec::ForkJoin runs on the harness's work-stealing pool, so the
// same code is exercised under ThreadSanitizer.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "forkjoin.hpp"

namespace exec {

class ForkJoin {
public:
    void charge(std::int64_t = 1) const {}

    template <typename F, typename G>
    void fork2(F&& f, G&& g) const {
        practice::fork2(std::forward<F>(f), std::forward<G>(g));
    }

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) const {
        practice::parallel_for(begin, end, std::forward<Body>(body), grain);
    }
};

// A fork costs one unit on the forking strand; the two branches then run "at once": their work
// adds and only the longer span counts.
class Meter {
public:
    void charge(std::int64_t units = 1) {
        current_.work += units;
        current_.span += units;
    }

    template <typename F, typename G>
    void fork2(F&& f, G&& g) {
        ++spawns_;
        charge();
        const Cost before = current_;
        current_ = Cost{};
        f();
        const Cost left = current_;
        current_ = Cost{};
        g();
        const Cost right = current_;
        current_ = Cost{before.work + left.work + right.work,
                        before.span + std::max(left.span, right.span)};
    }

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) {
        if (end <= begin) {
            return;
        }
        if (end - begin <= std::max<std::size_t>(grain, 1)) {
            for (std::size_t i = begin; i < end; ++i) {
                body(i);
            }
            return;
        }
        const std::size_t middle = begin + (end - begin) / 2;
        fork2([&] { parallel_for(begin, middle, body, grain); },
              [&] { parallel_for(middle, end, body, grain); });
    }

    std::int64_t work() const { return current_.work; }
    std::int64_t span() const { return current_.span; }
    std::int64_t spawns() const { return spawns_; }
    double parallelism() const {
        return current_.span == 0 ? 1.0
                                  : static_cast<double>(current_.work) /
                                        static_cast<double>(current_.span);
    }

private:
    struct Cost {
        std::int64_t work = 0;
        std::int64_t span = 0;
    };

    Cost current_;
    std::int64_t spawns_ = 0;
};

}  // namespace exec
