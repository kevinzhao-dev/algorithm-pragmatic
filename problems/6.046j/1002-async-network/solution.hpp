#pragma once

#include <limits>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;

// Stage 1
class MaxFlood {
public:
    using Message = long long;
    explicit MaxFlood(long long value) { PRACTICE_TODO(); }

    void on_start(netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    long long value() const { PRACTICE_TODO(); }
};

// Stage 2
class AsyncBfs {
public:
    using Message = long long;
    explicit AsyncBfs(bool is_root) { PRACTICE_TODO(); }

    void on_start(netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    long long distance() const { PRACTICE_TODO(); }
    int parent_port() const { PRACTICE_TODO(); }
};

// Stage 3
class CountingTree {
public:
    struct Message {
        int kind = 0;
        long long value = 0;
    };
    explicit CountingTree(bool is_root) { PRACTICE_TODO(); }

    void on_start(netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    long long root_count() const { PRACTICE_TODO(); }
};

// Stage 4
class AsyncBellmanFord {
public:
    using Message = long long;
    AsyncBellmanFord(bool is_root, std::vector<long long> port_weights) { PRACTICE_TODO(); }

    void on_start(netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    long long distance() const { PRACTICE_TODO(); }
    int parent_port() const { PRACTICE_TODO(); }
};
