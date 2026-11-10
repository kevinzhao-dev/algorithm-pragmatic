#pragma once

#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "forkjoin.hpp"

template <typename M>
concept Monoid = requires(typename M::value_type& left, typename M::value_type&& right) {
    { M::identity() } -> std::convertible_to<typename M::value_type>;
    M::reduce(left, std::move(right));
};

template <Monoid M>
class Reducer {
public:
    using value_type = typename M::value_type;

    Reducer() : root_(M::identity()) {}
    explicit Reducer(value_type initial) : root_(std::move(initial)) {}

    value_type& view() { return current_ != nullptr ? *current_ : root_; }

    template <typename F, typename G>
    void fork2(F&& f, G&& g) {
        value_type& left = view();
        std::optional<value_type> right_view;
        std::atomic<bool> left_finished{false};
        bool continued_on_left = false;
        practice::fork2(
            [&] {
                const StrandScope scope(&left);
                f();
                left_finished.store(true, std::memory_order_release);
            },
            [&] {
                if (left_finished.load(std::memory_order_acquire)) {
                    // f is complete: its view holds everything before g in serial order, and
                    // no other strand can touch it until this fork2 returns.
                    continued_on_left = true;
                    const StrandScope scope(&left);
                    g();
                } else {
                    right_view.emplace(M::identity());
                    const StrandScope scope(&*right_view);
                    g();
                }
            });
        if (!continued_on_left) {
            M::reduce(left, std::move(*right_view));
        }
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

    value_type take() { return std::move(root_); }

private:
    // RAII installation of the view that the code running on this thread should see.
    class StrandScope {
    public:
        explicit StrandScope(value_type* view) : previous_(current_) { current_ = view; }
        ~StrandScope() { current_ = previous_; }
        StrandScope(const StrandScope&) = delete;
        StrandScope& operator=(const StrandScope&) = delete;

    private:
        value_type* previous_;
    };

    static inline thread_local value_type* current_ = nullptr;

    value_type root_;
};

namespace detail_0513 {

struct BoardLog {
    using value_type = std::vector<std::uint64_t>;
    static value_type identity() { return {}; }
    static void reduce(value_type& left, value_type&& right) {
        left.insert(left.end(), right.begin(), right.end());
    }
};

inline void solve(Reducer<BoardLog>& reducer, int n, int row, std::uint32_t down, std::uint32_t left,
                  std::uint32_t right, std::uint64_t board, int serial_depth) {
    if (row == n) {
        reducer.view().push_back(board);
        return;
    }
    const std::uint32_t all = (std::uint32_t{1} << n) - 1;
    std::uint32_t available = ~(down | left | right) & all;
    if (row < serial_depth) {
        const auto branch = [&](std::uint32_t candidates, const auto& self) -> void {
            if (candidates == 0) {
                return;
            }
            if ((candidates & (candidates - 1)) == 0) {
                const std::uint32_t place = candidates;
                solve(reducer, n, row + 1, down | place, ((left | place) << 1) & all, (right | place) >> 1,
                      board | (static_cast<std::uint64_t>(std::countr_zero(place)) << (4 * row)),
                      serial_depth);
                return;
            }
            std::uint32_t low = 0;
            std::uint32_t rest = candidates;
            for (int to_take = std::popcount(candidates) / 2; to_take > 0; --to_take) {
                const std::uint32_t place = rest & (~rest + 1);
                low |= place;
                rest ^= place;
            }
            reducer.fork2([&] { self(low, self); }, [&] { self(rest, self); });
        };
        branch(available, branch);
        return;
    }
    while (available != 0) {
        const std::uint32_t place = available & (~available + 1);
        available ^= place;
        solve(reducer, n, row + 1, down | place, ((left | place) << 1) & all, (right | place) >> 1,
              board | (static_cast<std::uint64_t>(std::countr_zero(place)) << (4 * row)), serial_depth);
    }
}

}  // namespace detail_0513

inline std::vector<std::uint64_t> queens_ordered(int n, int serial_depth) {
    Reducer<detail_0513::BoardLog> reducer;
    detail_0513::solve(reducer, n, 0, 0, 0, 0, 0, serial_depth);
    return reducer.take();
}
