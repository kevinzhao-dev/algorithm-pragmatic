#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

struct Transition {
    int from;
    int to;
    std::int64_t delta;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

// Compressed adjacency: incident transition indices per state, keyed by `from` or `to`.
class Incidence {
public:
    Incidence(int states, std::span<const Transition> transitions, int Transition::*endpoint)
        : offsets_(idx(states) + 1, 0), items_(transitions.size()) {
        for (const Transition& t : transitions) {
            ++offsets_[idx(t.*endpoint) + 1];
        }
        for (std::size_t i = 1; i < offsets_.size(); ++i) {
            offsets_[i] += offsets_[i - 1];
        }
        std::vector<int> fill = offsets_;
        for (std::size_t i = 0; i < transitions.size(); ++i) {
            items_[idx(fill[idx(transitions[i].*endpoint)]++)] = static_cast<int>(i);
        }
    }

    int begin(int v) const { return offsets_[idx(v)]; }
    int end(int v) const { return offsets_[idx(v) + 1]; }
    int at(int i) const { return items_[idx(i)]; }

private:
    std::vector<int> offsets_;
    std::vector<int> items_;
};

struct Frame {
    int state;
    int cursor;
};

// Kosaraju's algorithm with explicit stacks: finish order on the forward graph, then flood the
// reverse graph in reverse finish order.
inline std::vector<int> strongly_connected_components(int states,
                                                       std::span<const Transition> transitions,
                                                       const Incidence& outgoing) {
    std::vector<char> seen(idx(states), 0);
    std::vector<int> finish_order;
    finish_order.reserve(idx(states));
    std::vector<Frame> stack;
    for (int root = 0; root < states; ++root) {
        if (seen[idx(root)] != 0) {
            continue;
        }
        seen[idx(root)] = 1;
        stack.push_back({root, outgoing.begin(root)});
        while (!stack.empty()) {
            const int state = stack.back().state;
            if (stack.back().cursor == outgoing.end(state)) {
                finish_order.push_back(state);
                stack.pop_back();
                continue;
            }
            const int next = transitions[idx(outgoing.at(stack.back().cursor++))].to;
            if (seen[idx(next)] == 0) {
                seen[idx(next)] = 1;
                stack.push_back({next, outgoing.begin(next)});
            }
        }
    }

    const Incidence incoming(states, transitions, &Transition::to);
    std::vector<int> component(idx(states), -1);
    std::vector<int> pending;
    int components = 0;
    for (auto it = finish_order.rbegin(); it != finish_order.rend(); ++it) {
        if (component[idx(*it)] != -1) {
            continue;
        }
        component[idx(*it)] = components;
        pending.push_back(*it);
        while (!pending.empty()) {
            const int state = pending.back();
            pending.pop_back();
            for (int cursor = incoming.begin(state); cursor != incoming.end(state); ++cursor) {
                const int previous = transitions[idx(incoming.at(cursor))].from;
                if (component[idx(previous)] == -1) {
                    component[idx(previous)] = components;
                    pending.push_back(previous);
                }
            }
        }
        ++components;
    }
    return component;
}

// Bellman-Ford from an implicit supernode (every state starts at 0). `sign` = -1 turns a positive
// cycle into a negative one. Returns transition indices along a cycle of the requested sign.
inline std::optional<std::vector<std::size_t>> cycle_of_sign(
    int states, std::span<const Transition> transitions, int sign) {
    std::vector<std::int64_t> dist(idx(states), 0);
    std::vector<int> via(idx(states), -1);
    int witness = -1;
    for (int round = 0; round < states; ++round) {
        witness = -1;
        for (std::size_t i = 0; i < transitions.size(); ++i) {
            const Transition& t = transitions[i];
            const std::int64_t candidate = dist[idx(t.from)] + sign * t.delta;
            if (candidate < dist[idx(t.to)]) {
                dist[idx(t.to)] = candidate;
                via[idx(t.to)] = static_cast<int>(i);
                witness = t.to;
            }
        }
        if (witness == -1) {
            return std::nullopt;
        }
    }
    // A state relaxed in the last round reaches a predecessor cycle within `states` steps.
    for (int step = 0; step < states; ++step) {
        witness = transitions[idx(via[idx(witness)])].from;
    }
    std::vector<std::size_t> cycle;
    int at = witness;
    do {
        const auto used = static_cast<std::size_t>(via[idx(at)]);
        cycle.push_back(used);
        at = transitions[used].from;
    } while (at != witness);
    std::reverse(cycle.begin(), cycle.end());
    return cycle;
}

}  // namespace detail

// Inside one strongly connected component every cycle has total 0 exactly when each state can be
// assigned a potential with potential[to] == potential[from] + delta on every internal transition.
inline bool is_conservative(int states, std::span<const Transition> transitions) {
    const detail::Incidence outgoing(states, transitions, &Transition::from);
    const std::vector<int> component = detail::strongly_connected_components(states, transitions, outgoing);

    std::vector<std::int64_t> potential(detail::idx(states), 0);
    std::vector<char> assigned(detail::idx(states), 0);
    std::vector<int> pending;
    for (int root = 0; root < states; ++root) {
        if (assigned[detail::idx(root)] != 0) {
            continue;
        }
        assigned[detail::idx(root)] = 1;
        pending.push_back(root);
        while (!pending.empty()) {
            const int state = pending.back();
            pending.pop_back();
            for (int cursor = outgoing.begin(state); cursor != outgoing.end(state); ++cursor) {
                const Transition& t = transitions[detail::idx(outgoing.at(cursor))];
                if (component[detail::idx(t.to)] == component[detail::idx(state)] &&
                    assigned[detail::idx(t.to)] == 0) {
                    assigned[detail::idx(t.to)] = 1;
                    potential[detail::idx(t.to)] = potential[detail::idx(state)] + t.delta;
                    pending.push_back(t.to);
                }
            }
        }
    }
    for (const Transition& t : transitions) {
        if (component[detail::idx(t.from)] == component[detail::idx(t.to)] &&
            potential[detail::idx(t.to)] != potential[detail::idx(t.from)] + t.delta) {
            return false;
        }
    }
    return true;
}

inline std::optional<std::vector<std::size_t>> find_unbalanced_cycle(
    int states, std::span<const Transition> transitions) {
    if (auto negative = detail::cycle_of_sign(states, transitions, 1)) {
        return negative;
    }
    return detail::cycle_of_sign(states, transitions, -1);
}
