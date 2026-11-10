#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

struct FakeClock {
    std::uint64_t now = 0;
    void advance(std::uint64_t ticks) { now += ticks; }
    Profiler::Clock clock() {
        return [this] { return now; };
    }
};

Profiler::Entry get(const std::vector<Profiler::Entry>& report, const std::string& name) {
    for (const auto& entry : report) {
        if (entry.name == name) {
            return entry;
        }
    }
    return Profiler::Entry{};
}

}  // namespace

TEST(1, "a single phase is all self time") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("load");
    t.advance(7);
    p.exit();
    const auto report = p.report();
    REQUIRE_EQ(report.size(), 1);
    CHECK_EQ(report[0].name, "load");
    CHECK_EQ(report[0].calls, 1);
    CHECK_EQ(report[0].inclusive, 7);
    CHECK_EQ(report[0].self, 7);
}

TEST(1, "nesting splits self and inclusive") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("a");
    t.advance(1);
    p.enter("b");
    t.advance(2);
    p.exit();
    t.advance(1);
    p.exit();
    const auto report = p.report();
    REQUIRE_EQ(report.size(), 2);
    const auto a = get(report, "a");
    CHECK_EQ(a.calls, 1);
    CHECK_EQ(a.self, 2);
    CHECK_EQ(a.inclusive, 4);
    const auto b = get(report, "b");
    CHECK_EQ(b.calls, 1);
    CHECK_EQ(b.self, 2);
    CHECK_EQ(b.inclusive, 2);
}

TEST(1, "repeated sibling phases accumulate") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("root");
    for (int i = 1; i <= 3; ++i) {
        p.enter("leaf");
        t.advance(static_cast<std::uint64_t>(i));
        p.exit();
    }
    p.exit();
    const auto leaf = get(p.report(), "leaf");
    CHECK_EQ(leaf.calls, 3);
    CHECK_EQ(leaf.self, 6);
    CHECK_EQ(leaf.inclusive, 6);
}

TEST(1, "direct recursion counts inclusive only at the outermost activation") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("rec");
    t.advance(1);
    p.enter("rec");
    t.advance(1);
    p.enter("rec");
    t.advance(1);
    p.exit();
    t.advance(1);
    p.exit();
    t.advance(1);
    p.exit();
    const auto report = p.report();
    REQUIRE_EQ(report.size(), 1);
    CHECK_EQ(report[0].calls, 3);
    CHECK_EQ(report[0].self, 5);
    CHECK_EQ(report[0].inclusive, 5);
}

TEST(1, "indirect recursion through another phase is also counted once") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("a");
    t.advance(1);
    p.enter("b");
    t.advance(1);
    p.enter("a");
    t.advance(1);
    p.exit();
    p.exit();
    p.exit();
    const auto report = p.report();
    const auto a = get(report, "a");
    CHECK_EQ(a.inclusive, 3);
    CHECK_EQ(a.self, 2);
    CHECK_EQ(a.calls, 2);
    const auto b = get(report, "b");
    CHECK_EQ(b.inclusive, 2);
    CHECK_EQ(b.self, 1);
}

TEST(1, "report is sorted by self descending, then name") {
    FakeClock t;
    Profiler p(t.clock());
    for (const char* name : {"zeta", "alpha", "mu"}) {
        p.enter(name);
        t.advance(5);
        p.exit();
    }
    p.enter("longest");
    t.advance(9);
    p.exit();
    const auto report = p.report();
    REQUIRE_EQ(report.size(), 4);
    CHECK_EQ(report[0].name, "longest");
    CHECK_EQ(report[1].name, "alpha");
    CHECK_EQ(report[2].name, "mu");
    CHECK_EQ(report[3].name, "zeta");
}

TEST(1, "unbalanced exit throws and leaves the profiler usable") {
    FakeClock t;
    Profiler p(t.clock());
    CHECK_THROWS_AS(p.exit(), std::logic_error);
    CHECK_EQ(p.depth(), 0);
    p.enter("x");
    CHECK_EQ(p.depth(), 1);
    t.advance(2);
    p.exit();
    CHECK_THROWS_AS(p.exit(), std::logic_error);
    CHECK_EQ(get(p.report(), "x").self, 2);
}

TEST(1, "names are copied, not borrowed") {
    FakeClock t;
    Profiler p(t.clock());
    {
        std::string temporary = "a-rather-long-phase-name-allocated-on-the-heap";
        p.enter(temporary);
        temporary.assign(100, '#');
    }
    t.advance(3);
    p.exit();
    CHECK_EQ(p.report()[0].name, "a-rather-long-phase-name-allocated-on-the-heap");
}

TEST(1, "open phases do not appear in the report") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("done");
    t.advance(1);
    p.exit();
    p.enter("still-open");
    t.advance(100);
    const auto report = p.report();
    REQUIRE_EQ(report.size(), 1);
    CHECK_EQ(report[0].name, "done");
}

TEST(1, "random nesting matches a brute-force recomputation") {
    Rng rng;
    const std::vector<std::string> names = {"a", "b", "c", "d"};
    for (int round = 0; round < 60; ++round) {
        FakeClock t;
        Profiler p(t.clock());
        struct Activation {
            std::string name;
            std::uint64_t start;
            std::uint64_t nested;
        };
        std::vector<Activation> stack;
        std::map<std::string, Profiler::Entry> expected;
        const auto close_top = [&] {
            p.exit();
            const Activation done = stack.back();
            stack.pop_back();
            const std::uint64_t elapsed = t.now - done.start;
            Profiler::Entry& entry = expected[done.name];
            entry.name = done.name;
            entry.calls += 1;
            entry.self += elapsed - done.nested;
            const bool nested_same = std::any_of(stack.begin(), stack.end(), [&](const auto& a) {
                return a.name == done.name;
            });
            if (!nested_same) {
                entry.inclusive += elapsed;
            }
            if (!stack.empty()) {
                stack.back().nested += elapsed;
            }
        };
        for (int step = 0; step < 2000; ++step) {
            t.advance(static_cast<std::uint64_t>(rng.uniform(0, 4)));
            if (stack.empty() || (stack.size() < 12 && rng.coin(0.55))) {
                const std::string& name = names[static_cast<std::size_t>(rng.uniform(0, 3))];
                p.enter(name);
                stack.push_back({name, t.now, 0});
            } else {
                close_top();
            }
        }
        while (!stack.empty()) {
            close_top();
        }
        const auto report = p.report();
        INFO("round ", round);
        for (const auto& [name, want] : expected) {
            const auto got = get(report, name);
            CHECK_EQ(got.calls, want.calls);
            CHECK_EQ(got.self, want.self);
            CHECK_EQ(got.inclusive, want.inclusive);
        }
    }
}

PERF(1, "enter and exit are cheap on hot paths", 2000) {
    FakeClock t;
    Profiler p(t.clock());
    for (int i = 0; i < 2'000'000; ++i) {
        p.enter("outer");
        p.enter("inner");
        t.advance(1);
        p.exit();
        p.exit();
    }
    CHECK_EQ(get(p.report(), "inner").calls, 2'000'000);
}

TEST(2, "scoped phases balance automatically") {
    FakeClock t;
    Profiler p(t.clock());
    {
        ScopedPhase outer(p, "outer");
        t.advance(1);
        {
            ScopedPhase inner(p, "inner");
            t.advance(2);
        }
        t.advance(3);
    }
    CHECK_EQ(p.depth(), 0);
    const auto report = p.report();
    CHECK_EQ(get(report, "outer").self, 4);
    CHECK_EQ(get(report, "outer").inclusive, 6);
    CHECK_EQ(get(report, "inner").self, 2);
}

TEST(2, "an exception unwinds phases in order") {
    FakeClock t;
    Profiler p(t.clock());
    try {
        ScopedPhase outer(p, "outer");
        t.advance(1);
        ScopedPhase inner(p, "inner");
        t.advance(1);
        throw std::runtime_error("failed");
    } catch (const std::runtime_error&) {
    }
    CHECK_EQ(p.depth(), 0);
    CHECK_EQ(get(p.report(), "outer").inclusive, 2);
    CHECK_EQ(get(p.report(), "inner").inclusive, 1);
}

TEST(3, "each distinct path gets one line with its self time") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("main");
    p.enter("parse");
    p.enter("scan");
    t.advance(120);
    p.exit();
    t.advance(30);
    p.exit();
    p.enter("eval");
    t.advance(50);
    p.exit();
    p.exit();
    CHECK_EQ(p.folded(), std::vector<std::string>{"main;eval 50", "main;parse 30", "main;parse;scan 120"});
}

TEST(3, "the same name on different paths stays separate") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("a");
    p.enter("c");
    t.advance(1);
    p.exit();
    p.exit();
    p.enter("b");
    p.enter("c");
    t.advance(2);
    p.exit();
    p.exit();
    CHECK_EQ(p.folded(), std::vector<std::string>{"a;c 1", "b;c 2"});
}

TEST(3, "paths with no self time are omitted") {
    FakeClock t;
    Profiler p(t.clock());
    p.enter("wrapper");
    p.enter("work");
    t.advance(5);
    p.exit();
    p.exit();
    CHECK_EQ(p.folded(), std::vector<std::string>{"wrapper;work 5"});
}

TEST(4, "self goes to the leaf and inclusive to every distinct frame") {
    const std::vector<std::vector<std::string>> stacks = {
        {"main", "parse", "scan"},
        {"main", "parse"},
        {"main", "eval"},
        {"main", "parse", "scan"},
    };
    const auto report = report_from_samples(stacks);
    CHECK_EQ(get(report, "main").inclusive, 4);
    CHECK_EQ(get(report, "main").self, 0);
    CHECK_EQ(get(report, "parse").inclusive, 3);
    CHECK_EQ(get(report, "parse").self, 1);
    CHECK_EQ(get(report, "scan").self, 2);
    CHECK_EQ(get(report, "eval").self, 1);
    CHECK_EQ(get(report, "scan").calls, 0);
}

TEST(4, "recursion in a sample is counted once for inclusive") {
    const std::vector<std::vector<std::string>> stacks = {{"a", "b", "a"}, {"a"}};
    const auto report = report_from_samples(stacks);
    CHECK_EQ(get(report, "a").inclusive, 2);
    CHECK_EQ(get(report, "a").self, 2);
    CHECK_EQ(get(report, "b").inclusive, 1);
    CHECK_EQ(get(report, "b").self, 0);
}

TEST(4, "empty samples are ignored") {
    const std::vector<std::vector<std::string>> stacks = {{}, {"x"}, {}};
    const auto report = report_from_samples(stacks);
    REQUIRE_EQ(report.size(), 1);
    CHECK_EQ(report[0].self, 1);
}

TEST(4, "samples taken every tick reproduce the instrumented numbers") {
    Rng rng;
    const std::vector<std::string> names = {"f", "g", "h"};
    FakeClock t;
    Profiler p(t.clock());
    std::vector<std::string> live;
    std::vector<std::vector<std::string>> samples;
    for (int step = 0; step < 4000; ++step) {
        if (live.empty() || (live.size() < 8 && rng.coin(0.5))) {
            const std::string& name = names[static_cast<std::size_t>(rng.uniform(0, 2))];
            p.enter(name);
            live.push_back(name);
        } else {
            p.exit();
            live.pop_back();
        }
        if (!live.empty()) {
            // Sample at the end of every tick, with the stack currently active.
            samples.push_back(live);
            t.advance(1);
        }
    }
    while (!live.empty()) {
        p.exit();
        live.pop_back();
    }
    const auto instrumented = p.report();
    const auto sampled = report_from_samples(samples);
    for (const std::string& name : names) {
        INFO("name ", name);
        CHECK_EQ(get(sampled, name).self, get(instrumented, name).self);
        CHECK_EQ(get(sampled, name).inclusive, get(instrumented, name).inclusive);
    }
}
