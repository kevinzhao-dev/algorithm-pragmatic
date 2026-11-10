#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include "01_exec.hpp"

namespace detail_0617 {

template <typename Exec, typename T, typename Op>
T reduce_range(Exec& exec, std::span<const T> x, Op& op) {
    if (x.size() == 1) {
        return x[0];
    }
    const std::size_t mid = x.size() / 2;
    std::optional<T> left;
    std::optional<T> right;
    exec.fork2([&] { left.emplace(reduce_range(exec, x.first(mid), op)); },
               [&] { right.emplace(reduce_range(exec, x.subspan(mid), op)); });
    exec.charge();
    return op(*left, *right);
}

template <typename Exec, typename T, typename Op>
void scan2(Exec& exec, std::span<const T> x, std::vector<T>& y, std::size_t begin, std::size_t end,
           Op& op) {
    if (end - begin == 1) {
        y[begin] = x[begin];
        return;
    }
    const std::size_t mid = begin + (end - begin) / 2;
    exec.fork2([&] { scan2(exec, x, y, begin, mid, op); },
               [&] { scan2(exec, x, y, mid, end, op); });
    // Every y[l] in the right half is prefixed by the left half's total, y[mid - 1].
    exec.parallel_for(mid, end, [&](std::size_t l) {
        exec.charge();
        y[l] = op(y[mid - 1], y[l]);
    });
}

// Returns x[begin] ⊗ … ⊗ x[end-1] and stores, for each split point, the left part's total.
template <typename Exec, typename T, typename Op>
T scan_up(Exec& exec, std::span<const T> x, std::vector<T>& t, std::size_t begin, std::size_t end,
          Op& op) {
    if (end - begin == 1) {
        return x[begin];
    }
    const std::size_t mid = begin + (end - begin) / 2;
    std::optional<T> right;
    exec.fork2([&] { t[mid - 1] = scan_up(exec, x, t, begin, mid, op); },
               [&] { right.emplace(scan_up(exec, x, t, mid, end, op)); });
    exec.charge();
    return op(t[mid - 1], *right);
}

// `carry` holds everything to the left of this range.
template <typename Exec, typename T, typename Op>
void scan_down(Exec& exec, const T& carry, std::span<const T> x, const std::vector<T>& t,
               std::vector<T>& y, std::size_t begin, std::size_t end, Op& op) {
    if (end - begin == 1) {
        exec.charge();
        y[begin] = op(carry, x[begin]);
        return;
    }
    const std::size_t mid = begin + (end - begin) / 2;
    exec.charge();
    const T carry_right = op(carry, t[mid - 1]);
    exec.fork2([&] { scan_down(exec, carry, x, t, y, begin, mid, op); },
               [&] { scan_down(exec, carry_right, x, t, y, mid, end, op); });
}

}  // namespace detail_0617

template <typename Exec, typename T, typename Op>
T p_reduce(Exec& exec, std::span<const T> x, Op op) {
    if (x.empty()) {
        throw std::invalid_argument("p_reduce needs at least one element");
    }
    return detail_0617::reduce_range(exec, x, op);
}

template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_1(Exec& exec, std::span<const T> x, Op op) {
    std::vector<T> y(x.size());
    exec.parallel_for(0, x.size(), [&](std::size_t i) {
        y[i] = detail_0617::reduce_range(exec, x.first(i + 1), op);
    });
    return y;
}

template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_2(Exec& exec, std::span<const T> x, Op op) {
    std::vector<T> y(x.size());
    if (!x.empty()) {
        detail_0617::scan2(exec, x, y, 0, x.size(), op);
    }
    return y;
}

template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_3(Exec& exec, std::span<const T> x, Op op) {
    std::vector<T> y(x.size());
    if (x.empty()) {
        return y;
    }
    y[0] = x[0];
    if (x.size() > 1) {
        std::vector<T> t(x.size());
        detail_0617::scan_up(exec, x, t, 1, x.size(), op);
        detail_0617::scan_down(exec, x[0], x, t, y, 1, x.size(), op);
    }
    return y;
}

template <typename Exec, typename T, typename Pred>
std::vector<T> parallel_filter(Exec& exec, std::span<const T> x, Pred pred) {
    const std::size_t n = x.size();
    if (n == 0) {
        return {};
    }
    std::vector<int> flags(n);
    exec.parallel_for(0, n, [&](std::size_t i) {
        exec.charge();
        flags[i] = pred(x[i]) ? 1 : 0;
    });
    const std::vector<int> positions =
        p_scan_3(exec, std::span<const int>(flags), [](int a, int b) { return a + b; });
    std::vector<T> out(static_cast<std::size_t>(positions.back()));
    exec.parallel_for(0, n, [&](std::size_t i) {
        if (flags[i] != 0) {
            exec.charge();
            out[static_cast<std::size_t>(positions[i] - 1)] = x[i];
        }
    });
    return out;
}
