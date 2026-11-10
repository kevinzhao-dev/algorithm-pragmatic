#pragma once

#include <algorithm>
#include <vector>

#include "06_net_sim.hpp"

class LayeredBfs {
public:
    enum class Kind { Search, ParentYes, ParentNo, Ready, Done };
    struct Message {
        Kind kind = Kind::Search;
        int value = 0;  // Search: sender's level; Done: 1 if the subtree found new nodes
    };

    explicit LayeredBfs(bool is_root) : is_root_(is_root), level_(is_root ? 0 : -1) {}

    void on_start(netsim::Context<Message>& ctx) {
        if (is_root_) {
            start_search(ctx);
        }
    }

    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) {
        switch (m.kind) {
            case Kind::Search:
                if (parent_ == -1 && !is_root_) {
                    parent_ = port;
                    level_ = m.value + 1;
                    frontier_ = true;
                    ctx.output(port);
                    ctx.send(port, {Kind::ParentYes, 0});
                } else {
                    ctx.send(port, {Kind::ParentNo, 0});
                }
                break;
            case Kind::ParentYes:
                children_.push_back(port);
                new_nodes_ = true;
                note_response(ctx);
                break;
            case Kind::ParentNo:
                note_response(ctx);
                break;
            case Kind::Ready:
                on_ready(ctx);
                break;
            case Kind::Done:
                new_nodes_ = new_nodes_ || m.value != 0;
                ++dones_;
                if (dones_ == static_cast<int>(expected_children_)) {
                    finish_wave(ctx);
                }
                break;
        }
    }

    bool root_done() const { return done_; }
    int parent_port() const { return parent_; }
    int level() const { return level_; }

private:
    // Frontier behaviour: probe every neighbour except the parent.
    void start_search(netsim::Context<Message>& ctx) {
        searching_ = true;
        responses_ = 0;
        new_nodes_ = false;
        expected_responses_ = 0;
        for (int port = 0; port < ctx.ports(); ++port) {
            if (port != parent_) {
                ctx.send(port, {Kind::Search, level_});
                ++expected_responses_;
            }
        }
        if (expected_responses_ == 0) {
            finish_frontier(ctx);
        }
    }

    void note_response(netsim::Context<Message>& ctx) {
        if (++responses_ == expected_responses_) {
            finish_frontier(ctx);
        }
    }

    // A frontier node (or the root in phase 1) has heard all its neighbours.
    void finish_frontier(netsim::Context<Message>& ctx) {
        searching_ = false;
        if (is_root_) {
            // Phase 1 finished at the root: start the next phase if anything new appeared.
            if (new_nodes_) {
                start_ready_wave(ctx);
            } else {
                done_ = true;
            }
            return;
        }
        ctx.send(parent_, {Kind::Done, new_nodes_ ? 1 : 0});
    }

    void start_ready_wave(netsim::Context<Message>& ctx) {
        new_nodes_ = false;
        dones_ = 0;
        expected_children_ = children_.size();
        for (int child : children_) {
            ctx.send(child, {Kind::Ready, 0});
        }
        if (children_.empty()) {
            done_ = true;
        }
    }

    void on_ready(netsim::Context<Message>& ctx) {
        if (frontier_) {
            frontier_ = false;
            start_search(ctx);
            return;
        }
        new_nodes_ = false;
        dones_ = 0;
        expected_children_ = children_.size();
        if (children_.empty()) {
            // A childless interior node reports nothing new immediately.
            ctx.send(parent_, {Kind::Done, 0});
            return;
        }
        for (int child : children_) {
            ctx.send(child, {Kind::Ready, 0});
        }
    }

    void finish_wave(netsim::Context<Message>& ctx) {
        if (is_root_) {
            if (new_nodes_) {
                start_ready_wave(ctx);
            } else {
                done_ = true;
            }
        } else {
            ctx.send(parent_, {Kind::Done, new_nodes_ ? 1 : 0});
        }
    }

    bool is_root_;
    int level_;
    int parent_ = -1;
    bool frontier_ = false;
    bool searching_ = false;
    int responses_ = 0;
    int expected_responses_ = 0;
    bool new_nodes_ = false;
    int dones_ = 0;
    std::size_t expected_children_ = 0;
    std::vector<int> children_;
    bool done_ = false;
};
