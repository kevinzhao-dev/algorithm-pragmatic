#pragma once

#include "06_net_sim.hpp"

class LcrProcess {
public:
    using Message = long long;
    explicit LcrProcess(long long uid) : uid_(uid) {}

    void on_start(netsim::Context<Message>& ctx) { ctx.send(netsim::Topology::kClockwise, uid_); }

    void on_message(int, const Message& m, netsim::Context<Message>& ctx) {
        if (m > uid_) {
            ctx.send(netsim::Topology::kClockwise, m);
        } else if (m == uid_) {
            leader_ = true;
        }
    }

    bool is_leader() const { return leader_; }

private:
    long long uid_;
    bool leader_ = false;
};

class HsProcess {
public:
    struct Message {
        bool reply = false;
        long long uid = 0;
        int hops = 0;
    };

    explicit HsProcess(long long uid) : uid_(uid) {}

    void on_start(netsim::Context<Message>& ctx) { probe(ctx); }

    void on_message(int port, const Message& m, netsim::Context<Message>& ctx) {
        const int onward = 1 - port;
        if (!m.reply) {
            if (m.uid == uid_) {
                leader_ = true;  // the probe made a full circle
            } else if (m.uid > uid_) {
                if (m.hops > 1) {
                    ctx.send(onward, Message{false, m.uid, m.hops - 1});
                } else {
                    ctx.send(port, Message{true, m.uid, 0});
                }
            }
            return;
        }
        if (m.uid != uid_) {
            ctx.send(onward, m);
            return;
        }
        if (++replies_ == 2) {
            replies_ = 0;
            ++phase_;
            probe(ctx);
        }
    }

    bool is_leader() const { return leader_; }

private:
    void probe(netsim::Context<Message>& ctx) {
        const int hops = 1 << phase_;
        ctx.send(netsim::Topology::kClockwise, Message{false, uid_, hops});
        ctx.send(netsim::Topology::kCounterclockwise, Message{false, uid_, hops});
    }

    long long uid_;
    int phase_ = 0;
    int replies_ = 0;
    bool leader_ = false;
};
