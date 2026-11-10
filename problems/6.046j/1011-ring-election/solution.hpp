#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"

// Stage 1
template <typename P>
bool symmetry_preserved(std::function<P()> make, int n, int rounds) {
    PRACTICE_TODO();
}

// Stage 2
class RandomizedElection {
public:
    using Message = std::int64_t;
    RandomizedElection(int n, std::uint64_t seed) { PRACTICE_TODO(); }

    void send(std::int64_t round, netsim::Outbox<Message>& out) { PRACTICE_TODO(); }
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox) {
        PRACTICE_TODO();
    }
    bool halted() const { PRACTICE_TODO(); }
    bool is_leader() const { PRACTICE_TODO(); }
    int phases() const { PRACTICE_TODO(); }
};

// Stage 3
struct MirrorResult {
    int leaders_small = 0;
    int leaders_large = 0;
};

template <typename P, typename Make>
MirrorResult mirror_attack(Make make, int n, std::uint64_t seed, std::int64_t max_rounds) {
    PRACTICE_TODO();
}
