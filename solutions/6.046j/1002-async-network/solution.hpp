#pragma once

#include <limits>
#include <vector>

#include "06_net_sim.hpp"

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;

class MaxFlood {
public:
    using Message = long long;
    explicit MaxFlood(long long value) : max_(value) {}

    void on_start(netsim::Context<Message>& ctx) { ctx.broadcast(max_); }
    void on_message(int, const Message& m, netsim::Context<Message>& ctx) {
        if (m > max_) {
            max_ = m;
            ctx.broadcast(max_);
        }
    }
    long long value() const { return max_; }

private:
    long long max_;
};

class AsyncBfs {
public:
    using Message = long long;
    explicit AsyncBfs(bool is_root) : dist_(is_root ? 0 : kInf), is_root_(is_root) {}

    void on_start(netsim::Context<Message>& ctx) {
        if (is_root_) {
            ctx.broadcast(0);
        }
    }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) {
        if (m + 1 < dist_) {
            dist_ = m + 1;
            parent_ = port;
            ctx.broadcast(dist_);
        }
    }
    long long distance() const { return dist_; }
    int parent_port() const { return parent_; }

private:
    long long dist_;
    bool is_root_;
    int parent_ = -1;
};

class CountingTree {
public:
    enum class Kind { Search, Accept, Reject, Done };
    struct Message {
        Kind kind = Kind::Search;
        long long value = 0;
    };

    explicit CountingTree(bool is_root) : is_root_(is_root) {}

    void on_start(netsim::Context<Message>& ctx) {
        if (is_root_) {
            joined_ = true;
            expected_ = ctx.ports();
            ctx.broadcast({Kind::Search, 0});
            maybe_finish(ctx);  // an isolated root
        }
    }

    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) {
        switch (m.kind) {
            case Kind::Search:
                if (!joined_) {
                    joined_ = true;
                    parent_ = port;
                    ctx.send(port, {Kind::Accept, 0});
                    expected_ = ctx.ports() - 1;
                    for (int p = 0; p < ctx.ports(); ++p) {
                        if (p != port) {
                            ctx.send(p, {Kind::Search, 0});
                        }
                    }
                } else {
                    ctx.send(port, {Kind::Reject, 0});
                }
                break;
            case Kind::Accept:
                ++children_;
                ++responses_;
                break;
            case Kind::Reject:
                ++responses_;
                break;
            case Kind::Done:
                ++done_children_;
                total_ += m.value;
                break;
        }
        maybe_finish(ctx);
    }

    long long root_count() const { return root_count_; }

private:
    void maybe_finish(netsim::Context<Message>& ctx) {
        if (!finished_ && joined_ && responses_ == expected_ && done_children_ == children_) {
            finished_ = true;
            const long long subtree = total_ + 1;
            if (is_root_) {
                root_count_ = subtree;
            } else {
                ctx.send(parent_, {Kind::Done, subtree});
            }
        }
    }

    bool is_root_;
    bool joined_ = false;
    int parent_ = -1;
    int expected_ = 0;
    int responses_ = 0;
    int children_ = 0;
    int done_children_ = 0;
    long long total_ = 0;
    bool finished_ = false;
    long long root_count_ = -1;
};

class AsyncBellmanFord {
public:
    using Message = long long;
    AsyncBellmanFord(bool is_root, std::vector<long long> port_weights)
        : weights_(std::move(port_weights)), dist_(is_root ? 0 : kInf), is_root_(is_root) {}

    void on_start(netsim::Context<Message>& ctx) {
        if (is_root_) {
            ctx.broadcast(0);
        }
    }
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) {
        if (m + weights_[port] < dist_) {
            dist_ = m + weights_[port];
            parent_ = port;
            ctx.broadcast(dist_);
        }
    }
    long long distance() const { return dist_; }
    int parent_port() const { return parent_; }

private:
    std::vector<long long> weights_;
    long long dist_;
    bool is_root_;
    int parent_ = -1;
};
