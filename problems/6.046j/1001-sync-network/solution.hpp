#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;

// Stage 1
class CliqueLeader {
public:
    using Message = std::int64_t;
    explicit CliqueLeader(std::int64_t uid) { PRACTICE_TODO(); }

    void send(std::int64_t round, netsim::Outbox<Message>& out) { PRACTICE_TODO(); }
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox) {
        PRACTICE_TODO();
    }
    bool halted() const { PRACTICE_TODO(); }
    bool is_leader() const { PRACTICE_TODO(); }
};

// Stage 2
class BfsTree {
public:
    struct Message {
        int kind = 0;   // define your own message types
        int depth = 0;
    };
    explicit BfsTree(bool is_root) { PRACTICE_TODO(); }

    void send(std::int64_t round, netsim::Outbox<Message>& out) { PRACTICE_TODO(); }
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox) {
        PRACTICE_TODO();
    }
    bool halted() const { PRACTICE_TODO(); }

    int parent_port() const { PRACTICE_TODO(); }
    std::vector<int> children_ports() const { PRACTICE_TODO(); }
    int depth() const { PRACTICE_TODO(); }
    bool finished() const { PRACTICE_TODO(); }
};

// Stage 3
class Luby {
public:
    struct Message {
        bool joined = false;
        std::uint64_t value = 0;
    };
    Luby(std::uint64_t seed, std::uint64_t n_upper) { PRACTICE_TODO(); }

    void send(std::int64_t round, netsim::Outbox<Message>& out) { PRACTICE_TODO(); }
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox) {
        PRACTICE_TODO();
    }
    bool halted() const { PRACTICE_TODO(); }

    bool decided() const { PRACTICE_TODO(); }
    bool in_mis() const { PRACTICE_TODO(); }
};

// Stage 4
class SyncBellmanFord {
public:
    using Message = long long;
    SyncBellmanFord(bool is_root, std::vector<long long> port_weights, int rounds) {
        PRACTICE_TODO();
    }

    void send(std::int64_t round, netsim::Outbox<Message>& out) { PRACTICE_TODO(); }
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox) {
        PRACTICE_TODO();
    }
    bool halted() const { PRACTICE_TODO(); }

    long long distance() const { PRACTICE_TODO(); }
    int parent_port() const { PRACTICE_TODO(); }
};
