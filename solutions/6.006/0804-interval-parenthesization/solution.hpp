#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace detail {

// How an interval reached its extreme: the split point and which child extreme fed each side.
struct Choice {
    std::size_t split = 0;
    std::uint8_t left_max = 0;
    std::uint8_t right_max = 0;
};

struct ParenTable {
    std::size_t n;
    std::vector<std::int64_t> lowest;
    std::vector<std::int64_t> highest;
    std::vector<Choice> min_choice;
    std::vector<Choice> max_choice;

    std::size_t at(std::size_t first, std::size_t last) const { return first * n + last; }
};

inline void validate_expression(std::span<const std::int64_t> operands, std::string_view ops) {
    if (operands.empty() || ops.size() + 1 != operands.size()) {
        throw std::invalid_argument("expression has the wrong shape");
    }
    for (const char op : ops) {
        if (op != '+' && op != '*') {
            throw std::invalid_argument("unknown operator");
        }
    }
}

inline ParenTable build_paren_table(std::span<const std::int64_t> operands, std::string_view ops) {
    validate_expression(operands, ops);
    const std::size_t n = operands.size();
    ParenTable table{n,
                     std::vector<std::int64_t>(n * n),
                     std::vector<std::int64_t>(n * n),
                     std::vector<Choice>(n * n),
                     std::vector<Choice>(n * n)};
    for (std::size_t i = 0; i < n; ++i) {
        table.lowest[table.at(i, i)] = operands[i];
        table.highest[table.at(i, i)] = operands[i];
    }
    for (std::size_t length = 2; length <= n; ++length) {
        for (std::size_t first = 0; first + length <= n; ++first) {
            const std::size_t last = first + length - 1;
            const std::size_t here = table.at(first, last);
            std::int64_t low = std::numeric_limits<std::int64_t>::max();
            std::int64_t high = std::numeric_limits<std::int64_t>::min();
            for (std::size_t split = first; split < last; ++split) {
                const std::size_t left = table.at(first, split);
                const std::size_t right = table.at(split + 1, last);
                for (const std::uint8_t left_max : {std::uint8_t{0}, std::uint8_t{1}}) {
                    for (const std::uint8_t right_max : {std::uint8_t{0}, std::uint8_t{1}}) {
                        const std::int64_t a = left_max ? table.highest[left] : table.lowest[left];
                        const std::int64_t b =
                            right_max ? table.highest[right] : table.lowest[right];
                        const std::int64_t value = ops[split] == '+' ? a + b : a * b;
                        if (value < low) {
                            low = value;
                            table.min_choice[here] = {split, left_max, right_max};
                        }
                        if (value > high) {
                            high = value;
                            table.max_choice[here] = {split, left_max, right_max};
                        }
                    }
                }
            }
            table.lowest[here] = low;
            table.highest[here] = high;
        }
    }
    return table;
}

inline void render_expression(const ParenTable& table, std::span<const std::int64_t> operands,
                              std::string_view ops, std::size_t first, std::size_t last,
                              bool want_max, std::string& out) {
    if (first == last) {
        out += std::to_string(operands[first]);
        return;
    }
    const std::size_t here = table.at(first, last);
    const Choice choice = want_max ? table.max_choice[here] : table.min_choice[here];
    out += '(';
    render_expression(table, operands, ops, first, choice.split, choice.left_max != 0, out);
    out += ops[choice.split];
    render_expression(table, operands, ops, choice.split + 1, last, choice.right_max != 0, out);
    out += ')';
}

struct ChainTable {
    std::size_t matrices;
    std::vector<std::int64_t> cost;
    std::vector<std::size_t> split;

    std::size_t at(std::size_t first, std::size_t last) const { return first * matrices + last; }
};

inline ChainTable build_chain_table(std::span<const std::int64_t> dims) {
    if (dims.size() < 2) {
        throw std::invalid_argument("need at least one matrix");
    }
    const std::size_t n = dims.size() - 1;
    ChainTable table{n, std::vector<std::int64_t>(n * n, 0), std::vector<std::size_t>(n * n, 0)};
    for (std::size_t length = 2; length <= n; ++length) {
        for (std::size_t first = 0; first + length <= n; ++first) {
            const std::size_t last = first + length - 1;
            std::int64_t best = std::numeric_limits<std::int64_t>::max();
            for (std::size_t cut = first; cut < last; ++cut) {
                const std::int64_t cost = table.cost[table.at(first, cut)] +
                                          table.cost[table.at(cut + 1, last)] +
                                          dims[first] * dims[cut + 1] * dims[last + 1];
                if (cost < best) {
                    best = cost;
                    table.split[table.at(first, last)] = cut;
                }
            }
            table.cost[table.at(first, last)] = best;
        }
    }
    return table;
}

inline void render_chain(const ChainTable& table, std::size_t first, std::size_t last,
                         std::string& out) {
    if (first == last) {
        out += 'M';
        out += std::to_string(first);
        return;
    }
    const std::size_t cut = table.split[table.at(first, last)];
    out += '(';
    render_chain(table, first, cut, out);
    out += '*';
    render_chain(table, cut + 1, last, out);
    out += ')';
}

}  // namespace detail

inline std::int64_t max_value(std::span<const std::int64_t> operands, std::string_view ops) {
    const auto table = detail::build_paren_table(operands, ops);
    return table.highest[table.at(0, operands.size() - 1)];
}

inline std::string best_parenthesization(std::span<const std::int64_t> operands,
                                         std::string_view ops) {
    const auto table = detail::build_paren_table(operands, ops);
    std::string out;
    detail::render_expression(table, operands, ops, 0, operands.size() - 1, true, out);
    return out;
}

inline std::int64_t min_multiplications(std::span<const std::int64_t> dims) {
    const auto table = detail::build_chain_table(dims);
    return table.cost[table.at(0, dims.size() - 2)];
}

inline std::string best_multiplication_order(std::span<const std::int64_t> dims) {
    const auto table = detail::build_chain_table(dims);
    std::string out;
    detail::render_chain(table, 0, dims.size() - 2, out);
    return out;
}
