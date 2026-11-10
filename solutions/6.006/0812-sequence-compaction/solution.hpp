#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

struct Removal {
    std::size_t first;
    std::size_t second;
};

struct CompactionPlan {
    std::int64_t score;
    std::vector<Removal> removals;
};

namespace detail {

// Partner choices for interval [first, last).
inline constexpr std::int32_t kSkip = -2;
inline constexpr std::int32_t kAlone = -1;

// Index 0 of the flag means "free to leave records", index 1 means "must clear everything".
class CompactionTable {
public:
    explicit CompactionTable(std::span<const int> records)
        : v_(records.begin(), records.end()),
          n_(records.size()),
          score_(4 * (n_ + 1) * (n_ + 1), 0),
          partner_(4 * (n_ + 1) * (n_ + 1), kSkip) {
        for (std::size_t first = n_; first-- > 0;) {
            for (std::size_t last = first + 1; last <= n_; ++last) {
                for (const int must_clear : {0, 1}) {
                    solve(first, last, must_clear);
                }
            }
        }
    }

    std::int64_t value(int must_clear) const { return score(0, n_, must_clear); }

    std::vector<Removal> plan() const {
        std::vector<Removal> removals;
        emit(0, n_, 0, removals);
        return removals;
    }

private:
    std::size_t at(std::size_t first, std::size_t last, int must_clear) const {
        return ((first * (n_ + 1) + last) << 2) + static_cast<std::size_t>(must_clear);
    }

    std::int64_t score(std::size_t first, std::size_t last, int must_clear) const {
        return score_[at(first, last, must_clear)];
    }

    void solve(std::size_t first, std::size_t last, int must_clear) {
        std::int64_t best = std::numeric_limits<std::int64_t>::min();
        std::int32_t choice = kAlone;
        const auto consider = [&](std::int64_t candidate, std::int32_t option) {
            if (candidate > best) {
                best = candidate;
                choice = option;
            }
        };
        if (!must_clear) {
            consider(score(first + 1, last, 0), kSkip);
        }
        consider(v_[first] + score(first + 1, last, must_clear), kAlone);
        for (std::size_t partner = first + 1; partner < last; ++partner) {
            consider(static_cast<std::int64_t>(v_[first]) * v_[partner] +
                         score(first + 1, partner, 1) + score(partner + 1, last, must_clear),
                     static_cast<std::int32_t>(partner));
        }
        score_[at(first, last, must_clear)] = best;
        partner_[at(first, last, must_clear)] = choice;
    }

    void emit(std::size_t first, std::size_t last, int must_clear,
              std::vector<Removal>& out) const {
        if (first == last) {
            return;
        }
        const std::int32_t choice = partner_[at(first, last, must_clear)];
        if (choice == kSkip) {
            emit(first + 1, last, must_clear, out);
        } else if (choice == kAlone) {
            out.push_back({first, first});
            emit(first + 1, last, must_clear, out);
        } else {
            const auto partner = static_cast<std::size_t>(choice);
            emit(first + 1, partner, 1, out);  // the inside must be gone before the pair
            out.push_back({first, partner});
            emit(partner + 1, last, must_clear, out);
        }
    }

    std::vector<int> v_;
    std::size_t n_;
    std::vector<std::int64_t> score_;
    std::vector<std::int32_t> partner_;
};

}  // namespace detail

inline std::int64_t max_compaction_score(std::span<const int> records) {
    return detail::CompactionTable(records).value(0);
}

inline std::int64_t max_full_compaction_score(std::span<const int> records) {
    return detail::CompactionTable(records).value(1);
}

inline CompactionPlan compaction_plan(std::span<const int> records) {
    const detail::CompactionTable table(records);
    return {table.value(0), table.plan()};
}

inline std::vector<std::pair<std::size_t, std::size_t>> max_noncrossing_wires(
    std::span<const bool> is_source) {
    const std::size_t n = is_source.size();
    const auto matchable = [&](std::size_t a, std::size_t b) { return is_source[a] != is_source[b]; };
    const auto index = [&](std::size_t first, std::size_t last) { return first * (n + 1) + last; };
    std::vector<std::int32_t> wires((n + 1) * (n + 1), 0);
    std::vector<std::int32_t> choice((n + 1) * (n + 1), -1);  // partner of `first`, or -1

    const auto value = [&](std::size_t first, std::size_t last) -> std::int32_t& {
        return wires[index(first, last)];
    };

    for (std::size_t first = n; first-- > 0;) {
        for (std::size_t last = first + 1; last <= n; ++last) {
            std::int32_t best = value(first + 1, last);
            std::int32_t partner = -1;
            for (std::size_t t = first + 2; t < last; ++t) {
                // On the whole circle, port 0 and port n - 1 are neighbors.
                if ((first == 0 && last == n && t == n - 1) || !matchable(first, t)) {
                    continue;
                }
                const std::int32_t candidate = 1 + value(first + 1, t) + value(t + 1, last);
                if (candidate > best) {
                    best = candidate;
                    partner = static_cast<std::int32_t>(t);
                }
            }
            value(first, last) = best;
            choice[index(first, last)] = partner;
        }
    }

    std::vector<std::pair<std::size_t, std::size_t>> result;
    // Explicit stack of intervals left to expand.
    std::vector<std::pair<std::size_t, std::size_t>> pending{{0, n}};
    while (!pending.empty()) {
        const auto [first, last] = pending.back();
        pending.pop_back();
        if (first >= last) {
            continue;
        }
        const std::int32_t partner = choice[index(first, last)];
        if (partner < 0) {
            pending.emplace_back(first + 1, last);
        } else {
            const auto t = static_cast<std::size_t>(partner);
            result.emplace_back(first, t);
            pending.emplace_back(first + 1, t);
            pending.emplace_back(t + 1, last);
        }
    }
    std::ranges::sort(result);
    return result;
}
