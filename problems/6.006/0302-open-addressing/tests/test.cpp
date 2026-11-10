#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

namespace {

struct ConstantHash {
    std::size_t operator()(int) const { return 7; }
};

struct TrackedHash {
    std::size_t operator()(const Tracked& value) const {
        return std::hash<int>{}(value.value());
    }
};

using Set = OpenAddressingSet<std::uint64_t>;

}  // namespace

TEST(1, "a new set is empty and has room") {
    Set set;
    CHECK_EQ(set.size(), 0);
    CHECK(!set.contains(1));
    CHECK_GT(set.capacity(), 0);
}

TEST(1, "inserted keys are found") {
    Set set;
    CHECK(set.insert(5));
    CHECK(set.insert(9));
    CHECK(set.contains(5));
    CHECK(set.contains(9));
    CHECK(!set.contains(7));
    CHECK_EQ(set.size(), 2);
}

TEST(1, "inserting twice keeps one copy") {
    Set set;
    CHECK(set.insert(5));
    CHECK(!set.insert(5));
    CHECK_EQ(set.size(), 1);
}

TEST(1, "growth keeps every key and there is always an empty slot") {
    Set set;
    for (std::uint64_t key = 0; key < 20'000; ++key) {
        CHECK(set.insert(key * 7));
        REQUIRE(set.size() < set.capacity());
    }
    for (std::uint64_t key = 0; key < 20'000; ++key) {
        REQUIRE(set.contains(key * 7));
    }
    CHECK(!set.contains(1));
}

TEST(1, "string keys work") {
    OpenAddressingSet<std::string> set;
    CHECK(set.insert("hello"));
    CHECK(set.insert("world"));
    CHECK(!set.insert("hello"));
    CHECK(set.contains("world"));
    CHECK(!set.contains("nope"));
}

TEST(1, "a hash that collides on everything still works") {
    OpenAddressingSet<int, ConstantHash> set;
    for (int key = 0; key < 2000; ++key) {
        CHECK(set.insert(key));
    }
    CHECK_EQ(set.size(), 2000);
    for (int key = 0; key < 2000; key += 13) {
        CHECK(set.contains(key));
    }
    CHECK(!set.contains(99999));
}

TEST(1, "every key is released") {
    const auto before = Tracked::live();
    {
        OpenAddressingSet<Tracked, TrackedHash> set;
        for (int i = 0; i < 3000; ++i) {
            set.insert(Tracked(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random inserts match std::unordered_set") {
    Rng rng;
    Set set;
    std::unordered_set<std::uint64_t> model;
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        const auto key = static_cast<std::uint64_t>(rng.uniform(0, 5000));
        if (rng.coin()) {
            REQUIRE_EQ(set.insert(key), model.insert(key).second);
        } else {
            REQUIRE_EQ(set.contains(key), model.contains(key));
        }
        REQUIRE_EQ(set.size(), model.size());
    }
}

PERF(1, "keys that differ only in high bits do not cluster", 1500) {
    Set set;
    for (std::uint64_t i = 0; i < 400'000; ++i) {
        set.insert(i << 24);
    }
    for (std::uint64_t i = 0; i < 400'000; ++i) {
        REQUIRE(set.contains(i << 24));
    }
    CHECK_EQ(set.size(), 400'000);
}

TEST(2, "erase removes a key and reports whether it was there") {
    Set set;
    set.insert(1);
    set.insert(2);
    CHECK(set.erase(1));
    CHECK(!set.contains(1));
    CHECK(set.contains(2));
    CHECK_EQ(set.size(), 1);
    CHECK(!set.erase(1));
    CHECK(!set.erase(99));
}

TEST(2, "erasing a key in the middle of a probe path leaves later keys reachable") {
    OpenAddressingSet<int, ConstantHash> set;
    for (int key = 0; key < 8; ++key) {
        set.insert(key);
    }
    // Everything hashes to the same home slot, so these form one long probe path.
    CHECK(set.erase(3));
    CHECK(set.erase(0));
    for (int key = 0; key < 8; ++key) {
        INFO("key ", key);
        CHECK_EQ(set.contains(key), key != 3 && key != 0);
    }
}

TEST(2, "inserting a key that lives past a tombstone does not duplicate it") {
    OpenAddressingSet<int, ConstantHash> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    set.erase(1);
    CHECK(!set.insert(3));
    CHECK_EQ(set.size(), 2);
    CHECK(set.erase(3));
    CHECK(!set.contains(3));
    CHECK_EQ(set.size(), 1);
}

TEST(2, "a key can come back after being erased") {
    Set set;
    set.insert(10);
    CHECK(set.erase(10));
    CHECK(set.insert(10));
    CHECK(set.contains(10));
    CHECK_EQ(set.size(), 1);
}

TEST(2, "deleting everything and refilling works") {
    Set set;
    for (std::uint64_t key = 0; key < 5000; ++key) {
        set.insert(key);
    }
    for (std::uint64_t key = 0; key < 5000; ++key) {
        CHECK(set.erase(key));
    }
    CHECK_EQ(set.size(), 0);
    for (std::uint64_t key = 0; key < 5000; ++key) {
        CHECK(set.insert(key + 100'000));
    }
    CHECK_EQ(set.size(), 5000);
}

TEST(2, "random inserts and erases match std::unordered_set") {
    Rng rng;
    Set set;
    std::unordered_set<std::uint64_t> model;
    for (int step = 0; step < 200'000; ++step) {
        INFO("step ", step);
        const auto key = static_cast<std::uint64_t>(rng.uniform(0, 2000));
        const auto op = rng.uniform(0, 2);
        if (op == 0) {
            REQUIRE_EQ(set.insert(key), model.insert(key).second);
        } else if (op == 1) {
            REQUIRE_EQ(set.erase(key), model.erase(key) > 0);
        } else {
            REQUIRE_EQ(set.contains(key), model.contains(key));
        }
        REQUIRE_EQ(set.size(), model.size());
    }
}

TEST(2, "colliding keys under random churn match a model") {
    Rng rng;
    OpenAddressingSet<int, ConstantHash> set;
    std::unordered_set<int> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 40));
        if (rng.coin()) {
            REQUIRE_EQ(set.insert(key), model.insert(key).second);
        } else {
            REQUIRE_EQ(set.erase(key), model.erase(key) > 0);
        }
        REQUIRE_EQ(set.size(), model.size());
    }
    for (int key = 0; key <= 40; ++key) {
        CHECK_EQ(set.contains(key), model.contains(key));
    }
}

TEST(3, "capacity shrinks after most keys are gone") {
    Set set;
    for (std::uint64_t key = 0; key < 100'000; ++key) {
        set.insert(key);
    }
    for (std::uint64_t key = 0; key < 99'990; ++key) {
        set.erase(key);
        REQUIRE(set.capacity() <= 8 * set.size() + 64);
    }
    for (std::uint64_t key = 99'990; key < 100'000; ++key) {
        CHECK(set.contains(key));
    }
}

TEST(3, "unbounded churn keeps the table bounded") {
    Set set;
    Rng rng;
    std::deque<std::uint64_t> live;
    for (std::uint64_t step = 0; step < 200'000; ++step) {
        set.insert(step);
        live.push_back(step);
        if (live.size() > 64) {
            REQUIRE(set.erase(live.front()));
            live.pop_front();
        }
        REQUIRE_EQ(set.size(), live.size());
        REQUIRE(set.capacity() <= 8 * set.size() + 64);
    }
    for (std::uint64_t key : live) {
        CHECK(set.contains(key));
    }
}

// Without tombstone cleanup the big table degrades into a linear scan on every miss.
PERF(3, "misses stay fast after millions of erases", 2000) {
    Set set;
    for (std::uint64_t key = 0; key < 300'000; ++key) {
        set.insert(key);
    }
    for (std::uint64_t key = 0; key < 299'900; ++key) {
        set.erase(key);
    }
    std::uint64_t hits = 0;
    std::uint64_t next = 1'000'000;
    for (int round = 0; round < 3'000'000; ++round) {
        const std::uint64_t key = next + static_cast<std::uint64_t>(round);
        set.insert(key);
        hits += set.contains(key) ? 1 : 0;
        hits += set.contains(~key) ? 1 : 0;  // a miss
        set.erase(key);
    }
    CHECK_EQ(hits, 3'000'000);
    CHECK_LE(set.capacity(), 8 * set.size() + 64);
}

TEST(4, "the first sighting is accepted, repeats inside the window are not") {
    DedupWindow<std::string> window(10);
    CHECK(window.offer("a", 100));
    CHECK(!window.offer("a", 105));
    CHECK(!window.offer("a", 109));
    CHECK_EQ(window.size(), 1);
}

TEST(4, "the key is forgotten at exactly ttl ticks after it was accepted") {
    DedupWindow<int> window(5);
    CHECK(window.offer(1, 0));
    CHECK(!window.offer(1, 4));
    CHECK(window.offer(1, 5));
    CHECK(!window.offer(1, 9));
    CHECK(window.offer(1, 10));
}

TEST(4, "repeats do not extend the window") {
    DedupWindow<int> window(5);
    CHECK(window.offer(1, 0));
    CHECK(!window.offer(1, 4));
    CHECK(window.offer(1, 5));  // accepted again even though it was seen at 4
}

TEST(4, "equal timestamps are all inside the window") {
    DedupWindow<int> window(1);
    CHECK(window.offer(1, 7));
    CHECK(!window.offer(1, 7));
    CHECK(window.offer(1, 8));
}

TEST(4, "expired keys are actually dropped") {
    DedupWindow<int> window(3);
    for (int key = 0; key < 1000; ++key) {
        CHECK(window.offer(key, key));
        REQUIRE(window.size() <= 3);
    }
}

TEST(4, "ttl must be positive") {
    CHECK_THROWS_AS(DedupWindow<int>(0), std::invalid_argument);
    CHECK_THROWS_AS(DedupWindow<int>(-1), std::invalid_argument);
}

TEST(4, "random stream matches a brute-force model") {
    Rng rng;
    const std::int64_t ttl = 25;
    DedupWindow<int> window(ttl);
    std::unordered_map<int, std::int64_t> accepted;
    std::int64_t now = 0;
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        now += rng.uniform(0, 2);
        const int key = static_cast<int>(rng.uniform(0, 60));
        const auto it = accepted.find(key);
        const bool fresh = it == accepted.end() || it->second <= now - ttl;
        if (fresh) {
            accepted[key] = now;
        }
        REQUIRE_EQ(window.offer(key, now), fresh);
        std::size_t alive = 0;
        for (const auto& [_, when] : accepted) {
            alive += when > now - ttl ? 1 : 0;
        }
        REQUIRE_EQ(window.size(), alive);
    }
}

PERF(4, "dedup handles a long stream with bounded memory", 2000) {
    DedupWindow<std::uint64_t> window(1000);
    Rng rng(99);
    std::uint64_t accepted = 0;
    for (std::int64_t now = 0; now < 4'000'000; ++now) {
        const auto key = static_cast<std::uint64_t>(rng.uniform(0, 5000));
        accepted += window.offer(key, now) ? 1 : 0;
        if ((now & 0xFFFFF) == 0) {
            CHECK_LE(window.size(), 1000);
        }
    }
    CHECK_GT(accepted, 0);
}
