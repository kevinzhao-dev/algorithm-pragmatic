#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <span>
#include <vector>

#include "06_net_sim.hpp"

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;

class CliqueLeader {
public:
    using Message = std::int64_t;
    explicit CliqueLeader(std::int64_t uid) : uid_(uid) {}

    void send(std::int64_t, netsim::Outbox<Message>& out) { out.broadcast(uid_); }

    void receive(std::int64_t, std::span<const std::optional<Message>> inbox) {
        std::int64_t best = uid_;
        for (const auto& slot : inbox) {
            if (slot) {
                best = std::max(best, *slot);
            }
        }
        leader_ = best == uid_;
        done_ = true;
    }

    bool halted() const { return done_; }
    bool is_leader() const { return leader_; }

private:
    std::int64_t uid_;
    bool leader_ = false;
    bool done_ = false;
};

class BfsTree {
public:
    enum class Kind { Search, Accept, Reject, Done };
    struct Message {
        Kind kind = Kind::Search;
        int depth = 0;
    };

    explicit BfsTree(bool is_root) : is_root_(is_root) {
        if (is_root_) {
            joined_ = true;
            depth_ = 0;
            search_pending_ = true;
        }
    }

    void send(std::int64_t, netsim::Outbox<Message>& out) {
        for (const Pending& p : replies_) {
            out.send(p.port, {p.accepted ? Kind::Accept : Kind::Reject, 0});
        }
        replies_.clear();
        if (search_pending_) {
            search_pending_ = false;
            for (int port = 0; port < out.ports(); ++port) {
                if (port != parent_ && std::find(skip_.begin(), skip_.end(), port) == skip_.end()) {
                    out.send(port, {Kind::Search, depth_});
                    ++searches_sent_;
                }
            }
            search_sent_ = true;
            skip_.clear();
        }
        if (done_pending_) {
            done_pending_ = false;
            if (!is_root_) {
                out.send(parent_, {Kind::Done, 0});
            }
            halted_ = true;
        }
    }

    void receive(std::int64_t, std::span<const std::optional<Message>> inbox) {
        std::vector<int> senders;
        for (int port = 0; port < static_cast<int>(inbox.size()); ++port) {
            if (!inbox[port]) {
                continue;
            }
            switch (inbox[port]->kind) {
                case Kind::Search:
                    senders.push_back(port);
                    break;
                case Kind::Accept:
                    children_.push_back(port);
                    ++responses_;
                    break;
                case Kind::Reject:
                    ++responses_;
                    break;
                case Kind::Done:
                    ++done_children_;
                    break;
            }
        }
        for (int port : senders) {
            if (!joined_) {
                joined_ = true;
                parent_ = port;
                depth_ = inbox[port]->depth + 1;
                replies_.push_back({port, true});
                search_pending_ = true;
            } else {
                replies_.push_back({port, false});
            }
            if (port != parent_ && search_pending_) {
                skip_.push_back(port);
            }
        }
        // Finished when all search messages are answered and all children reported.
        if (joined_ && search_sent_ && !finished_ && responses_ == searches_sent_ &&
            done_children_ == static_cast<int>(children_.size())) {
            finished_ = true;
            done_pending_ = true;
        }
    }

    bool halted() const { return halted_; }
    int parent_port() const { return parent_; }
    std::vector<int> children_ports() const {
        auto sorted_children = children_;
        std::sort(sorted_children.begin(), sorted_children.end());
        return sorted_children;
    }
    int depth() const { return depth_; }
    bool finished() const { return finished_; }

private:
    struct Pending {
        int port;
        bool accepted;
    };

    bool is_root_;
    bool joined_ = false;
    int parent_ = -1;
    int depth_ = -1;
    std::vector<Pending> replies_;
    std::vector<int> skip_;  // ports whose search crossed ours: no need to search them
    bool search_pending_ = false;
    bool search_sent_ = false;
    int searches_sent_ = 0;
    int responses_ = 0;
    int done_children_ = 0;
    std::vector<int> children_;
    bool done_pending_ = false;
    bool finished_ = false;
    bool halted_ = false;
};

class Luby {
public:
    struct Message {
        bool joined = false;
        std::uint64_t value = 0;
    };

    Luby(std::uint64_t seed, std::uint64_t n_upper) : rng_(seed) {
        const std::uint64_t n = std::max<std::uint64_t>(n_upper, 2);
        limit_ = n * n * n * n * n;
    }

    void send(std::int64_t round, netsim::Outbox<Message>& out) {
        if (round % 2 == 1) {
            value_ = std::uniform_int_distribution<std::uint64_t>(1, limit_)(rng_);
            out.broadcast({false, value_});
        } else if (joined_) {
            out.broadcast({true, 0});
        }
    }

    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox) {
        if (round % 2 == 1) {
            bool wins = true;
            for (const auto& slot : inbox) {
                if (slot && !slot->joined && slot->value >= value_) {
                    wins = false;
                }
            }
            joined_ = wins;
        } else {
            if (joined_) {
                decided_ = true;
                in_mis_ = true;
            } else {
                for (const auto& slot : inbox) {
                    if (slot && slot->joined) {
                        decided_ = true;
                    }
                }
            }
        }
    }

    bool halted() const { return decided_; }
    bool decided() const { return decided_; }
    bool in_mis() const { return in_mis_; }

private:
    std::mt19937_64 rng_;
    std::uint64_t limit_ = 0;
    std::uint64_t value_ = 0;
    bool joined_ = false;
    bool decided_ = false;
    bool in_mis_ = false;
};

class SyncBellmanFord {
public:
    using Message = long long;
    SyncBellmanFord(bool is_root, std::vector<long long> port_weights, int rounds)
        : weights_(std::move(port_weights)), rounds_(rounds) {
        if (is_root) {
            dist_ = 0;
        }
    }

    void send(std::int64_t, netsim::Outbox<Message>& out) {
        if (dist_ != kInf) {
            out.broadcast(dist_);
        }
    }

    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox) {
        for (int port = 0; port < static_cast<int>(inbox.size()); ++port) {
            if (inbox[port] && *inbox[port] + weights_[port] < dist_) {
                dist_ = *inbox[port] + weights_[port];
                parent_ = port;
            }
        }
        done_ = round >= rounds_;
    }

    bool halted() const { return done_; }
    long long distance() const { return dist_; }
    int parent_port() const { return parent_; }

private:
    std::vector<long long> weights_;
    int rounds_;
    long long dist_ = kInf;
    int parent_ = -1;
    bool done_ = false;
};
