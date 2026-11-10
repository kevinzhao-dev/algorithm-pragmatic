#pragma once

#include "06_net_sim.hpp"
#include "practice.hpp"

// Stage 1
class LcrProcess {
public:
    using Message = long long;
    explicit LcrProcess(long long uid) { PRACTICE_TODO(); }

    void on_start(netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    bool is_leader() const { PRACTICE_TODO(); }
};

// Stage 2
class HsProcess {
public:
    struct Message {
        bool reply = false;
        long long uid = 0;
        int hops = 0;
    };
    explicit HsProcess(long long uid) { PRACTICE_TODO(); }

    void on_start(netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    bool is_leader() const { PRACTICE_TODO(); }
};
