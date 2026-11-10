#pragma once

#include "06_net_sim.hpp"
#include "practice.hpp"

class LayeredBfs {
public:
    struct Message {
        int kind = 0;   // define your own message types
        int value = 0;
    };

    explicit LayeredBfs(bool is_root) { PRACTICE_TODO(); }

    void on_start(netsim::Context<Message>& ctx) { PRACTICE_TODO(); }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) { PRACTICE_TODO(); }

    bool root_done() const { PRACTICE_TODO(); }
    int parent_port() const { PRACTICE_TODO(); }
    int level() const { PRACTICE_TODO(); }
};
