#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <random>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using netsim::SyncNetwork;
using netsim::Topology;
using practice::Rng;

namespace {

// A deterministic process with a tiny bit of computation; identical copies stay identical.
class CountingEcho {
public:
    using Message = std::int64_t;
    void send(std::int64_t round, netsim::Outbox<Message>& out) {
        if (!done_) {
            out.broadcast(state_ * round);
        }
    }
    void receive(std::int64_t, std::span<const std::optional<Message>> inbox) {
        for (const auto& slot : inbox) {
            if (slot) {
                state_ = (state_ * 31 + *slot) % 1'000'003;
            }
        }
        ++ticks_;
        done_ = ticks_ >= 20;
    }
    bool halted() const { return done_; }
    std::string state_digest() const {
        return std::to_string(state_) + ":" + std::to_string(ticks_);
    }

private:
    std::int64_t state_ = 7;
    int ticks_ = 0;
    bool done_ = false;
};

// Not anonymous: remembers an internal instance number. Symmetry check must notice.
class SecretlyDifferent {
public:
    using Message = int;
    SecretlyDifferent() : id_(counter()++) {}
    void send(std::int64_t, netsim::Outbox<Message>& out) { out.broadcast(1); }
    void receive(std::int64_t, std::span<const std::optional<Message>>) { ++rounds_; }
    bool halted() const { return rounds_ >= 5; }
    std::string state_digest() const { return std::to_string(id_) + "/" + std::to_string(rounds_); }
    static int& counter() {
        static int c = 0;
        return c;
    }

private:
    int id_;
    int rounds_ = 0;
};

}  // namespace

TEST(1, "identical deterministic processes stay in lockstep") {
    for (int n : {2, 5, 16}) {
        for (int rounds : {1, 10, 30}) {
            INFO("n=", n, " rounds=", rounds);
            CHECK(symmetry_preserved<CountingEcho>([] { return CountingEcho{}; }, n, rounds));
        }
    }
}

TEST(1, "a hidden difference breaks symmetry") {
    SecretlyDifferent::counter() = 0;
    CHECK(!symmetry_preserved<SecretlyDifferent>([] { return SecretlyDifferent{}; }, 4, 3));
}

TEST(1, "a process that halts early still compares states") {
    SecretlyDifferent::counter() = 0;
    // Only the first process differs; once it is created with id 0 the others follow.
    CHECK(!symmetry_preserved<SecretlyDifferent>([] { return SecretlyDifferent{}; }, 2, 0));
}

TEST(2, "two processes eventually elect exactly one leader") {
    for (std::uint64_t seed = 1; seed <= 400; ++seed) {
        std::vector<RandomizedElection> procs;
        procs.emplace_back(2, seed * 2);
        procs.emplace_back(2, seed * 2 + 1);
        SyncNetwork<RandomizedElection> net(Topology::ring(2), std::move(procs));
        const auto stats = net.run(100'000);
        INFO("seed ", seed);
        CHECK(net.process(0).halted() && net.process(1).halted());
        CHECK(net.process(0).is_leader() != net.process(1).is_leader());
        const int phases = net.process(0).phases();
        CHECK_GE(phases, 1);
        CHECK_EQ(phases, net.process(1).phases());
        CHECK_LE(stats.rounds, 2 * phases);
        CHECK_LE(stats.messages, 4 * phases);
    }
}

TEST(2, "rings of many sizes never elect two leaders") {
    Rng rng;
    for (int n : {3, 4, 5, 7, 10, 13, 20, 32}) {
        for (int trial = 0; trial < 60; ++trial) {
            std::vector<RandomizedElection> procs;
            for (int v = 0; v < n; ++v) {
                procs.emplace_back(n, static_cast<std::uint64_t>(trial) * 1000003u + v);
            }
            SyncNetwork<RandomizedElection> net(Topology::ring(n), std::move(procs));
            const auto stats = net.run(1'000'000);
            int leaders = 0;
            INFO("n=", n, " trial ", trial);
            for (int v = 0; v < n; ++v) {
                CHECK(net.process(v).halted());
                leaders += net.process(v).is_leader() ? 1 : 0;
            }
            CHECK_EQ(leaders, 1);
            const int phases = net.process(0).phases();
            CHECK_LE(stats.rounds, n * phases);
            CHECK_LE(stats.messages, n * n * phases);
        }
    }
}

TEST(2, "the probability of needing many phases decays geometrically") {
    for (int n : {3, 5, 9}) {
        const int trials = 1500;
        std::map<int, int> tail;  // tail[h] = #runs needing more than h phases
        for (int trial = 0; trial < trials; ++trial) {
            std::vector<RandomizedElection> procs;
            for (int v = 0; v < n; ++v) {
                procs.emplace_back(n, static_cast<std::uint64_t>(trial) * 7919u + v * 104729u + 5u);
            }
            SyncNetwork<RandomizedElection> net(Topology::ring(n), std::move(procs));
            net.run(1'000'000);
            const int phases = net.process(0).phases();
            for (int h = 1; h <= 6; ++h) {
                tail[h] += phases > h ? 1 : 0;
            }
        }
        for (int h = 1; h <= 6; ++h) {
            const double fraction = static_cast<double>(tail[h]) / trials;
            INFO("n=", n, " h=", h);
            CHECK_LE(fraction, std::pow(0.5, h) + 0.06);
        }
    }
}

namespace {

// Candidate "unknown-n" algorithm: flood the largest id seen, become leader on hearing yourself.
class SeeOwnId {
public:
    using Message = std::int64_t;
    explicit SeeOwnId(std::uint64_t seed) {
        std::mt19937_64 rng(seed);
        id_ = static_cast<std::int64_t>(rng() >> 2) + 1;
        best_ = id_;
    }
    void send(std::int64_t, netsim::Outbox<Message>& out) {
        out.send(Topology::kClockwise, best_);
    }
    void receive(std::int64_t, std::span<const std::optional<Message>> inbox) {
        const std::int64_t incoming = *inbox[Topology::kCounterclockwise];
        if (incoming == id_ && !leader_ && best_ == id_) {
            leader_ = true;
            done_ = true;
        }
        best_ = std::max(best_, incoming);
    }
    bool halted() const { return done_; }
    bool is_leader() const { return leader_; }

private:
    std::int64_t id_ = 0;
    std::int64_t best_ = 0;
    bool leader_ = false;
    bool done_ = false;
};

// A candidate that never elects anybody.
class Silent {
public:
    using Message = int;
    explicit Silent(std::uint64_t) {}
    void send(std::int64_t, netsim::Outbox<Message>&) {}
    void receive(std::int64_t, std::span<const std::optional<Message>>) { ++rounds_; }
    bool halted() const { return rounds_ >= 3; }
    bool is_leader() const { return false; }

private:
    int rounds_ = 0;
};

}  // namespace

TEST(3, "a successful election on the small ring duplicates on the mirror") {
    int elected = 0;
    for (std::uint64_t seed = 1; seed <= 200; ++seed) {
        for (int n : {2, 3, 4, 7}) {
            const MirrorResult result = mirror_attack<SeeOwnId>(
                [](std::uint64_t s) { return SeeOwnId(s); }, n, seed * 113u, 4 * n);
            INFO("seed ", seed, " n=", n);
            CHECK_EQ(result.leaders_large, 2 * result.leaders_small);
            elected += result.leaders_small >= 1 ? 1 : 0;
        }
    }
    CHECK_GE(elected, 500);  // the candidate almost always succeeds on the small ring
}

TEST(3, "no leader on the small ring means none on the mirror") {
    const MirrorResult result =
        mirror_attack<Silent>([](std::uint64_t s) { return Silent(s); }, 5, 42, 100);
    CHECK_EQ(result.leaders_small, 0);
    CHECK_EQ(result.leaders_large, 0);
}
