#include <algorithm>
#include <numeric>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using netsim::AsyncNetwork;
using netsim::DelayFn;
using netsim::random_delay;
using netsim::Topology;
using netsim::unit_delay;
using practice::Rng;

namespace {

int lg_ceil(int n) {
    int lg = 0;
    while ((1 << lg) < n) {
        ++lg;
    }
    return lg;
}

template <typename P>
std::pair<int, std::int64_t> run_ring(const std::vector<long long>& uids, DelayFn delay,
                                      int* leader_vertex = nullptr) {
    const int n = static_cast<int>(uids.size());
    std::vector<P> procs;
    for (long long uid : uids) {
        procs.emplace_back(uid);
    }
    AsyncNetwork<P> net(Topology::ring(n), std::move(procs), std::move(delay));
    const auto stats = net.run();
    int leaders = 0;
    for (int v = 0; v < n; ++v) {
        if (net.process(v).is_leader()) {
            ++leaders;
            if (leader_vertex != nullptr) {
                *leader_vertex = v;
            }
        }
    }
    return {leaders, stats.messages};
}

std::vector<long long> ascending(int n) {
    std::vector<long long> uids(n);
    std::iota(uids.begin(), uids.end(), 1);
    return uids;
}

std::vector<long long> descending(int n) {
    auto uids = ascending(n);
    std::reverse(uids.begin(), uids.end());
    return uids;
}

}  // namespace

TEST(1, "a two-node ring elects the larger uid") {
    int leader = -1;
    const auto [leaders, messages] = run_ring<LcrProcess>({1, 2}, unit_delay(), &leader);
    CHECK_EQ(leaders, 1);
    CHECK_EQ(leader, 1);
}

TEST(1, "the largest uid wins on random rings") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 60));
        auto perm = rng.permutation(static_cast<std::size_t>(n));
        std::vector<long long> uids(perm.begin(), perm.end());
        for (auto& u : uids) {
            u = u * 3 + 7;
        }
        int leader = -1;
        const auto [leaders, messages] = run_ring<LcrProcess>(uids, random_delay(round + 1u, 11), &leader);
        INFO("round ", round, " n=", n);
        CHECK_EQ(leaders, 1);
        REQUIRE(leader >= 0);
        CHECK_EQ(uids[leader], *std::max_element(uids.begin(), uids.end()));
        CHECK_LE(messages, n * (n + 1) / 2);
    }
}

TEST(1, "descending clockwise order is the quadratic worst case") {
    for (int n : {3, 8, 40, 200}) {
        const auto [leaders, messages] = run_ring<LcrProcess>(descending(n), unit_delay());
        INFO("n=", n);
        CHECK_EQ(leaders, 1);
        CHECK_EQ(messages, n * (n + 1) / 2);
    }
}

TEST(1, "ascending clockwise order needs only linear messages") {
    for (int n : {3, 8, 40, 200}) {
        const auto [leaders, messages] = run_ring<LcrProcess>(ascending(n), unit_delay());
        INFO("n=", n);
        CHECK_EQ(leaders, 1);
        CHECK_EQ(messages, 2 * n - 1);
    }
}

TEST(2, "exactly one leader, the largest uid") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 64));
        auto perm = rng.permutation(static_cast<std::size_t>(n));
        std::vector<long long> uids(perm.begin(), perm.end());
        for (auto& u : uids) {
            u = u * 5 + 2;
        }
        int leader = -1;
        const DelayFn delay = round % 2 == 0 ? unit_delay() : random_delay(round + 9u, 13);
        const auto [leaders, messages] = run_ring<HsProcess>(uids, delay, &leader);
        INFO("round ", round, " n=", n);
        CHECK_EQ(leaders, 1);
        REQUIRE(leader >= 0);
        CHECK_EQ(uids[leader], *std::max_element(uids.begin(), uids.end()));
        CHECK_LE(messages, 8 * n * (lg_ceil(n) + 1));
    }
}

TEST(2, "the message bound holds on sorted rings too") {
    for (int n : {2, 3, 7, 16, 33, 128, 257, 1024}) {
        for (const auto& uids : {ascending(n), descending(n)}) {
            const auto [leaders, messages] = run_ring<HsProcess>(uids, unit_delay());
            INFO("n=", n);
            CHECK_EQ(leaders, 1);
            CHECK_LE(messages, 8 * n * (lg_ceil(n) + 1));
        }
    }
}

TEST(2, "hirschberg-sinclair beats lcr by a wide margin on bad rings") {
    const int n = 2048;
    const auto [lcr_leaders, lcr_messages] = run_ring<LcrProcess>(descending(n), unit_delay());
    const auto [hs_leaders, hs_messages] = run_ring<HsProcess>(descending(n), unit_delay());
    CHECK_EQ(lcr_leaders, 1);
    CHECK_EQ(hs_leaders, 1);
    CHECK_GE(lcr_messages, 20 * hs_messages);
}
