#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using Room = ChatRoom<int, std::string>;

namespace {

std::vector<int> viewer_ids(int n) {
    std::vector<int> ids(static_cast<std::size_t>(n));
    std::iota(ids.begin(), ids.end(), 1);
    return ids;
}

// Brute-force reference: the whole message log kept in chronological order.
struct Model {
    struct Entry {
        int viewer;
        std::string text;
    };
    std::vector<Entry> log;  // oldest first
    std::map<int, bool> banned;
    std::size_t limit = std::numeric_limits<std::size_t>::max();

    bool send(int viewer, std::string text) {
        const auto it = banned.find(viewer);
        if (it == banned.end() || it->second) {
            return false;
        }
        log.push_back({viewer, std::move(text)});
        // Per-viewer retention: drop this viewer's oldest message beyond the limit.
        std::size_t count = 0;
        for (const Entry& entry : log) {
            count += entry.viewer == viewer ? 1 : 0;
        }
        if (count > limit) {
            log.erase(std::find_if(log.begin(), log.end(), [&](const Entry& e) { return e.viewer == viewer; }));
        }
        return true;
    }

    bool ban(int viewer) {
        const auto it = banned.find(viewer);
        if (it == banned.end() || it->second) {
            return false;
        }
        it->second = true;
        std::erase_if(log, [&](const Entry& e) { return e.viewer == viewer; });
        return true;
    }

    std::vector<std::string> recent(std::size_t k) const {
        std::vector<std::string> out;
        for (auto it = log.rbegin(); it != log.rend() && out.size() < k; ++it) {
            out.push_back(it->text);
        }
        return out;
    }
};

}  // namespace

TEST(1, "messages come back newest first") {
    Room room(viewer_ids(2));
    CHECK(room.send(1, "a"));
    CHECK(room.send(2, "b"));
    CHECK(room.send(1, "c"));
    CHECK_EQ(room.recent(2), std::vector<std::string>{"c", "b"});
    CHECK_EQ(room.message_count(), 3);
}

TEST(1, "asking for more than exist returns everything") {
    Room room(viewer_ids(1));
    room.send(1, "only");
    CHECK_EQ(room.recent(5), std::vector<std::string>{"only"});
    CHECK_EQ(room.recent(1), std::vector<std::string>{"only"});
}

TEST(1, "asking for zero or from an empty room returns nothing") {
    Room room(viewer_ids(1));
    CHECK(room.recent(3).empty());
    room.send(1, "x");
    CHECK(room.recent(0).empty());
}

TEST(1, "unknown viewers cannot send") {
    Room room(viewer_ids(2));
    CHECK(!room.send(99, "spam"));
    CHECK(room.recent(1).empty());
    CHECK_EQ(room.message_count(), 0);
}

TEST(1, "duplicate viewers are rejected") {
    CHECK_THROWS_AS(Room(std::vector<int>{1, 2, 2}), std::invalid_argument);
}

TEST(1, "viewers need not be given in sorted order") {
    Room room(std::vector<int>{5, 1, 3});
    CHECK(room.send(3, "yes"));
    CHECK(room.send(5, "too"));
    CHECK(!room.send(4, "no"));
}

TEST(1, "send uses a logarithmic number of comparisons") {
    for (int n : {1, 2, 15, 1'000, 500'000}) {
        std::vector<Counted<int>> viewers;
        viewers.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            viewers.emplace_back(i * 2);
        }
        ChatRoom<Counted<int>, int> room(std::move(viewers));
        const auto budget = static_cast<std::size_t>(
            4 * std::ceil(std::log2(static_cast<double>(n) + 1.0)) + 4);
        INFO("n=", n);
        for (int key : {0, 1, n, 2 * n - 2, 2 * n + 5}) {
            Counted<int>::comparisons() = 0;
            room.send(Counted<int>(key), 0);
            REQUIRE(Counted<int>::comparisons() <= budget);
        }
    }
}

TEST(1, "random traffic matches a log model") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const int viewers = static_cast<int>(rng.uniform(1, 30));
        Room room(viewer_ids(viewers));
        Model model;
        for (int id = 1; id <= viewers; ++id) {
            model.banned[id] = false;
        }
        for (int step = 0; step < 400; ++step) {
            INFO("round ", round, " step ", step);
            if (rng.coin(0.7)) {
                const int viewer = static_cast<int>(rng.uniform(1, viewers + 2));
                const std::string text = "m" + std::to_string(step);
                REQUIRE_EQ(room.send(viewer, text), model.send(viewer, text));
            } else {
                const auto k = static_cast<std::size_t>(rng.uniform(0, 8));
                REQUIRE_EQ(room.recent(k), model.recent(k));
            }
            REQUIRE_EQ(room.message_count(), model.log.size());
        }
    }
}

// A linear scan over 500'000 viewers per send is hopeless here.
PERF(1, "send with half a million viewers", 800) {
    Rng rng;
    ChatRoom<int, int> room(viewer_ids(500'000));
    for (int i = 0; i < 400'000; ++i) {
        room.send(static_cast<int>(rng.uniform(1, 500'000)), i);
    }
    CHECK_EQ(room.message_count(), 400'000);
}

TEST(2, "banning removes all of the viewer's messages") {
    Room room(viewer_ids(2));
    room.send(1, "a");
    room.send(2, "b");
    room.send(1, "c");
    CHECK(room.ban(1));
    CHECK_EQ(room.recent(5), std::vector<std::string>{"b"});
    CHECK_EQ(room.message_count(), 1);
}

TEST(2, "banned viewers cannot send") {
    Room room(viewer_ids(1));
    CHECK(!room.is_banned(1));
    CHECK(room.ban(1));
    CHECK(room.is_banned(1));
    CHECK(!room.send(1, "hello"));
    CHECK(room.recent(1).empty());
}

TEST(2, "banning twice or banning strangers fails") {
    Room room(viewer_ids(1));
    CHECK(room.ban(1));
    CHECK(!room.ban(1));
    CHECK(!room.ban(42));
    CHECK(!room.is_banned(42));
}

TEST(2, "the recent list skips banned messages without gaps") {
    Room room(viewer_ids(3));
    room.send(1, "a1");
    room.send(2, "b1");
    room.send(3, "c1");
    room.send(2, "b2");
    room.send(1, "a2");
    room.ban(2);
    CHECK_EQ(room.recent(10), std::vector<std::string>{"a2", "c1", "a1"});
    room.send(3, "c2");
    CHECK_EQ(room.recent(2), std::vector<std::string>{"c2", "a2"});
}

TEST(2, "random traffic with bans matches a log model") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const int viewers = static_cast<int>(rng.uniform(1, 12));
        Room room(viewer_ids(viewers));
        Model model;
        for (int id = 1; id <= viewers; ++id) {
            model.banned[id] = false;
        }
        for (int step = 0; step < 500; ++step) {
            INFO("round ", round, " step ", step);
            const auto op = rng.uniform(0, 9);
            if (op <= 5) {
                const int viewer = static_cast<int>(rng.uniform(1, viewers + 1));
                const std::string text = "m" + std::to_string(step);
                REQUIRE_EQ(room.send(viewer, text), model.send(viewer, text));
            } else if (op <= 7) {
                const auto k = static_cast<std::size_t>(rng.uniform(0, 10));
                REQUIRE_EQ(room.recent(k), model.recent(k));
            } else if (op == 8) {
                const int viewer = static_cast<int>(rng.uniform(1, viewers + 1));
                REQUIRE_EQ(room.ban(viewer), model.ban(viewer));
            } else {
                const int viewer = static_cast<int>(rng.uniform(1, viewers + 1));
                const auto it = model.banned.find(viewer);
                REQUIRE_EQ(room.is_banned(viewer), it != model.banned.end() && it->second);
            }
            REQUIRE_EQ(room.message_count(), model.log.size());
        }
    }
}

// Scanning all messages per ban would be about 10^11 steps.
PERF(2, "banning cost does not depend on other viewers' messages", 3000) {
    const int viewers = 100'000;
    ChatRoom<int, int> room(viewer_ids(viewers));
    for (int message = 0; message < 1'000'000; ++message) {
        room.send(message % viewers + 1, message);
    }
    for (int id = 1; id <= viewers; ++id) {
        CHECK(room.ban(id));
    }
    CHECK_EQ(room.message_count(), 0);
}

// A tombstone scheme walks past a million deleted messages on every recent call: the five
// surviving messages are older than all the banned ones.
PERF(2, "recent stays O(k) after a wall of banned messages", 2500) {
    const int viewers = 1'000;
    ChatRoom<int, int> room(viewer_ids(viewers + 1));
    for (int i = 0; i < 5; ++i) {
        room.send(1, i);
    }
    for (int message = 0; message < 1'000'000; ++message) {
        room.send(message % viewers + 2, message);
    }
    for (int id = 2; id <= viewers + 1; ++id) {
        room.ban(id);
    }
    long long sum = 0;
    for (int query = 0; query < 200'000; ++query) {
        const auto messages = room.recent(5);
        sum += static_cast<long long>(messages.size());
    }
    CHECK_EQ(sum, 1'000'000);
}

TEST(3, "the per-viewer limit keeps only the newest messages of each viewer") {
    Room room(viewer_ids(2), 2);
    room.send(1, "a1");
    room.send(2, "b1");
    room.send(1, "a2");
    room.send(1, "a3");
    CHECK_EQ(room.recent(10), std::vector<std::string>{"a3", "a2", "b1"});
    CHECK_EQ(room.message_count(), 3);
}

TEST(3, "a limit of one replaces each viewer's message") {
    Room room(viewer_ids(1), 1);
    room.send(1, "first");
    room.send(1, "second");
    CHECK_EQ(room.recent(5), std::vector<std::string>{"second"});
}

TEST(3, "a zero limit is rejected") {
    CHECK_THROWS_AS(Room(viewer_ids(1), 0), std::invalid_argument);
}

TEST(3, "banning a viewer with a limit removes the retained messages") {
    Room room(viewer_ids(2), 1);
    room.send(1, "a1");
    room.send(1, "a2");
    room.send(2, "b");
    room.ban(1);
    CHECK_EQ(room.recent(10), std::vector<std::string>{"b"});
}

TEST(3, "random traffic with limits matches a log model") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const int viewers = static_cast<int>(rng.uniform(1, 8));
        const auto limit = static_cast<std::size_t>(rng.uniform(1, 4));
        Room room(viewer_ids(viewers), limit);
        Model model;
        model.limit = limit;
        for (int id = 1; id <= viewers; ++id) {
            model.banned[id] = false;
        }
        for (int step = 0; step < 500; ++step) {
            INFO("round ", round, " step ", step);
            const auto op = rng.uniform(0, 9);
            if (op <= 6) {
                const int viewer = static_cast<int>(rng.uniform(1, viewers));
                const std::string text = "m" + std::to_string(step);
                REQUIRE_EQ(room.send(viewer, text), model.send(viewer, text));
            } else if (op == 7) {
                const int viewer = static_cast<int>(rng.uniform(1, viewers));
                REQUIRE_EQ(room.ban(viewer), model.ban(viewer));
            } else {
                const auto k = static_cast<std::size_t>(rng.uniform(0, 12));
                REQUIRE_EQ(room.recent(k), model.recent(k));
            }
            REQUIRE_EQ(room.message_count(), model.log.size());
        }
    }
}

// Rebuilding the full history on each call costs 10^11 element copies here.
PERF(1, "recent costs O(k) however long the history is", 2000) {
    ChatRoom<int, int> room(viewer_ids(1'000));
    for (int message = 0; message < 500'000; ++message) {
        room.send(message % 1'000 + 1, message);
    }
    long long sum = 0;
    for (int query = 0; query < 300'000; ++query) {
        sum += static_cast<long long>(room.recent(5).size());
    }
    CHECK_EQ(sum, 1'500'000);
}

// Scanning for the oldest message on every send makes this quadratic.
PERF(3, "sending with a retention limit stays logarithmic", 1500) {
    ChatRoom<int, int> room(viewer_ids(100'000), 3);
    for (int i = 0; i < 3'000'000; ++i) {
        room.send(i % 100'000 + 1, i);
    }
    CHECK_EQ(room.message_count(), 300'000);
}
