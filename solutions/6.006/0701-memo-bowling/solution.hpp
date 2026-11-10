#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class Memo {
public:
    template <typename Relation>
    const Value& get(const Key& key, Relation&& relation) {
        if (const auto found = table_.find(key); found != table_.end()) {
            if (!found->second.has_value()) {
                throw std::logic_error("cyclic subproblem dependency");
            }
            return *found->second;
        }
        // A reference to a mapped value survives the rehashes that recursive inserts trigger;
        // an iterator would not.
        std::optional<Value>& entry = table_.emplace(key, std::nullopt).first->second;
        Rollback rollback{table_, key};
        auto recurse = [&](const Key& next) -> const Value& { return get(next, relation); };
        entry = relation(key, recurse);
        rollback.cancelled = true;
        return *entry;
    }

    std::size_t size() const { return table_.size(); }

private:
    using Table = std::unordered_map<Key, std::optional<Value>, Hash>;

    // Removes an in-progress entry if its relation exits by exception.
    struct Rollback {
        Table& table;
        const Key& key;
        bool cancelled = false;
        ~Rollback() {
            if (!cancelled) {
                table.erase(key);
            }
        }
    };

    Table table_;
};

class BowlingStream {
public:
    void push(int pin) {
        const std::int64_t value = pin;
        std::int64_t next = std::max(best_, best_ + value);
        if (has_previous_) {
            next = std::max(next, before_previous_ + previous_ * value);
        }
        before_previous_ = best_;
        previous_ = value;
        has_previous_ = true;
        best_ = next;
    }

    std::int64_t best() const { return best_; }

private:
    std::int64_t best_ = 0;             // best over all pins seen
    std::int64_t before_previous_ = 0;  // best over all pins except the latest
    std::int64_t previous_ = 0;
    bool has_previous_ = false;
};

inline std::int64_t bowling_max_score(std::span<const int> pins) {
    BowlingStream stream;
    for (const int pin : pins) {
        stream.push(pin);
    }
    return stream.best();
}

struct Throw {
    std::size_t first;
    std::size_t last;
};

inline std::vector<Throw> bowling_plan(std::span<const int> pins) {
    const std::size_t n = pins.size();
    // best[i]: highest score using only pins i..n-1.
    std::vector<std::int64_t> best(n + 2, 0);
    const auto pair_score = [&](std::size_t i) {
        return static_cast<std::int64_t>(pins[i]) * pins[i + 1] + best[i + 2];
    };
    for (std::size_t i = n; i-- > 0;) {
        best[i] = std::max(best[i + 1], best[i + 1] + pins[i]);
        if (i + 1 < n) {
            best[i] = std::max(best[i], pair_score(i));
        }
    }

    std::vector<Throw> plan;
    for (std::size_t i = 0; i < n;) {
        if (best[i] == best[i + 1]) {
            ++i;
        } else if (best[i] == best[i + 1] + pins[i]) {
            plan.push_back({i, i});
            ++i;
        } else {
            plan.push_back({i, i + 1});
            i += 2;
        }
    }
    return plan;
}
