#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "06_net_sim.hpp"

template <typename P>
bool symmetry_preserved(std::function<P()> make, int n, int rounds) {
    std::vector<P> processes;
    processes.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        processes.push_back(make());
    }
    netsim::SyncNetwork<P> net(netsim::Topology::ring(n), std::move(processes));
    const auto same_state = [&] {
        const std::string first = net.process(0).state_digest();
        for (int i = 1; i < n; ++i) {
            if (net.process(i).state_digest() != first) {
                return false;
            }
        }
        return true;
    };
    if (!same_state()) {
        return false;
    }
    for (int round = 0; round < rounds; ++round) {
        if (!net.step()) {
            return true;
        }
        if (!same_state()) {
            return false;
        }
    }
    return true;
}

class RandomizedElection {
public:
    using Message = std::int64_t;

    RandomizedElection(int n, std::uint64_t seed) : n_(n), rng_(seed) {}

    void send(std::int64_t, netsim::Outbox<Message>& out) {
        if (step_ == 0) {
            my_id_ = std::uniform_int_distribution<std::int64_t>(1, static_cast<std::int64_t>(n_) * n_)(rng_);
            seen_.clear();
            carrying_ = my_id_;
            ++phases_;
        }
        out.send(netsim::Topology::kClockwise, carrying_);
    }

    void receive(std::int64_t, std::span<const std::optional<Message>> inbox) {
        carrying_ = *inbox[netsim::Topology::kCounterclockwise];
        seen_.push_back(carrying_);
        if (++step_ == n_) {
            const std::int64_t best = *std::max_element(seen_.begin(), seen_.end());
            if (std::count(seen_.begin(), seen_.end(), best) == 1) {
                leader_ = my_id_ == best;
                done_ = true;
            }
            step_ = 0;
        }
    }

    bool halted() const { return done_; }
    bool is_leader() const { return leader_; }
    int phases() const { return phases_; }

private:
    int n_;
    std::mt19937_64 rng_;
    int step_ = 0;
    std::int64_t my_id_ = 0;
    std::int64_t carrying_ = 0;
    std::vector<std::int64_t> seen_;
    int phases_ = 0;
    bool leader_ = false;
    bool done_ = false;
};

struct MirrorResult {
    int leaders_small = 0;
    int leaders_large = 0;
};

template <typename P, typename Make>
MirrorResult mirror_attack(Make make, int n, std::uint64_t seed, std::int64_t max_rounds) {
    const auto run = [&](int size, const std::function<std::uint64_t(int)>& seed_of) {
        std::vector<P> processes;
        processes.reserve(static_cast<std::size_t>(size));
        for (int i = 0; i < size; ++i) {
            processes.push_back(make(seed_of(i)));
        }
        netsim::SyncNetwork<P> net(netsim::Topology::ring(size), std::move(processes));
        net.run(max_rounds);
        int leaders = 0;
        for (int i = 0; i < size; ++i) {
            leaders += net.process(i).is_leader() ? 1 : 0;
        }
        return leaders;
    };
    MirrorResult result;
    result.leaders_small = run(n, [seed](int i) { return seed + static_cast<std::uint64_t>(i); });
    result.leaders_large =
        run(2 * n, [seed, n](int i) { return seed + static_cast<std::uint64_t>(i % n); });
    return result;
}
