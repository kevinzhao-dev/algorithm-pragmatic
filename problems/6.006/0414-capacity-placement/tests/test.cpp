#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Machine = CapacityPlanner::Machine;
using MachineId = CapacityPlanner::MachineId;

namespace {

std::vector<std::string> sorted_tasks(const CapacityPlanner& planner, MachineId machine) {
    auto tasks = planner.tasks_on(machine);
    std::sort(tasks.begin(), tasks.end());
    return tasks;
}

}  // namespace

TEST(1, "places a task on a machine with room and accounts for it") {
    const std::vector<Machine> machines{{10, 5}, {20, 3}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    const auto machine = planner.place("web", 4);
    REQUIRE(machine.has_value());
    CHECK_EQ(*machine, MachineId{10});  // only machine 10 has room for 4
    CHECK_EQ(planner.available(10), 1);
    CHECK_EQ(planner.available(20), 3);
}

TEST(1, "an exact fit succeeds") {
    const std::vector<Machine> machines{{1, 7}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    CHECK_EQ(planner.place("task", 7), std::optional<MachineId>(1));
    CHECK_EQ(planner.available(1), 0);
}

TEST(1, "reports failure when nothing has room and changes nothing") {
    const std::vector<Machine> machines{{1, 3}, {2, 3}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    CHECK_EQ(planner.place("big", 4), std::optional<MachineId>());
    CHECK_EQ(planner.available(1), 3);
    CHECK_EQ(planner.available(2), 3);
    CHECK(planner.place("small", 3).has_value());
}

TEST(1, "an empty cluster places nothing") {
    const std::vector<Machine> machines;
    CapacityPlanner planner{std::span<const Machine>(machines)};
    CHECK_EQ(planner.place("task", 1), std::optional<MachineId>());
}

TEST(1, "placing the same task twice throws") {
    const std::vector<Machine> machines{{1, 10}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("task", 1);
    CHECK_THROWS_AS(planner.place("task", 1), std::invalid_argument);
    CHECK_EQ(planner.available(1), 9);
}

TEST(1, "unknown machines throw") {
    const std::vector<Machine> machines{{1, 10}};
    const CapacityPlanner planner{std::span<const Machine>(machines)};
    CHECK_THROWS_AS(planner.available(2), std::out_of_range);
}

TEST(2, "release returns the capacity") {
    const std::vector<Machine> machines{{1, 5}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("a", 4);
    planner.place("b", 1);
    planner.release("a");
    CHECK_EQ(planner.available(1), 4);
    CHECK(planner.place("c", 4).has_value());
    CHECK_THROWS_AS(planner.release("nope"), std::out_of_range);
}

TEST(2, "a released name can be used again") {
    const std::vector<Machine> machines{{1, 2}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("a", 2);
    planner.release("a");
    CHECK(planner.place("a", 2).has_value());
    CHECK_THROWS_AS(planner.release("b"), std::out_of_range);
}

TEST(2, "released capacity is visible to later placements on any machine") {
    const std::vector<Machine> machines{{1, 4}, {2, 4}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("x", 4);
    planner.place("y", 4);
    CHECK_EQ(planner.place("z", 2), std::optional<MachineId>());
    planner.release("x");
    CHECK(planner.place("z", 2).has_value());
}

TEST(2, "random traffic keeps capacity accounting correct") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        INFO("round ", round);
        const auto machine_count = static_cast<std::size_t>(rng.uniform(1, 12));
        std::vector<Machine> machines;
        std::map<MachineId, std::uint64_t> available;
        for (std::size_t i = 0; i < machine_count; ++i) {
            const auto capacity = static_cast<std::uint64_t>(rng.uniform(1, 30));
            machines.push_back({i * 7 + 1, capacity});
            available[i * 7 + 1] = capacity;
        }
        CapacityPlanner planner{std::span<const Machine>(machines)};
        std::map<std::string, std::pair<MachineId, std::uint64_t>> placed;
        int counter = 0;
        for (int step = 0; step < 400; ++step) {
            INFO("step ", step);
            if (placed.empty() || rng.coin(0.6)) {
                const std::string name = "t" + std::to_string(counter++);
                const auto demand = static_cast<std::uint64_t>(rng.uniform(1, 20));
                const auto got = planner.place(name, demand);
                std::uint64_t best = 0;
                for (const auto& [id, free] : available) {
                    best = std::max(best, free);
                }
                REQUIRE_EQ(got.has_value(), best >= demand);
                if (got) {
                    REQUIRE(available.contains(*got));
                    REQUIRE(available[*got] >= demand);
                    available[*got] -= demand;
                    placed[name] = {*got, demand};
                }
            } else {
                auto it = placed.begin();
                std::advance(it, static_cast<std::ptrdiff_t>(rng.uniform(0, static_cast<std::int64_t>(placed.size()) - 1)));
                planner.release(it->first);
                available[it->second.first] += it->second.second;
                placed.erase(it);
            }
            for (const auto& [id, free] : available) {
                REQUIRE_EQ(planner.available(id), free);
            }
        }
    }
}

PERF(2, "a million placements on a big cluster", 4000) {
    Rng rng;
    const std::size_t machine_count = 200'000;
    std::vector<Machine> machines;
    machines.reserve(machine_count);
    for (std::size_t i = 0; i < machine_count; ++i) {
        machines.push_back({i + 1, static_cast<std::uint64_t>(rng.uniform(100, 10'000))});
    }
    CapacityPlanner planner{std::span<const Machine>(machines)};
    std::vector<std::string> live;
    int counter = 0;
    for (int step = 0; step < 1'000'000; ++step) {
        if (live.empty() || rng.coin(0.55)) {
            const std::string name = "task" + std::to_string(counter++);
            if (planner.place(name, static_cast<std::uint64_t>(rng.uniform(1, 50))).has_value()) {
                live.push_back(name);
            }
        } else {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            planner.release(live[index]);
            live[index] = live.back();
            live.pop_back();
        }
    }
    CHECK_GT(live.size(), 0);
}

TEST(3, "tasks_on lists exactly the tasks of a machine") {
    const std::vector<Machine> machines{{1, 100}, {2, 100}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("a", 90);
    planner.place("b", 50);
    planner.place("c", 40);
    const auto on_one = sorted_tasks(planner, 1);
    const auto on_two = sorted_tasks(planner, 2);
    std::vector<std::string> combined = on_one;
    combined.insert(combined.end(), on_two.begin(), on_two.end());
    std::sort(combined.begin(), combined.end());
    CHECK_EQ(combined, std::vector<std::string>{"a", "b", "c"});
    CHECK(on_one.size() + on_two.size() == 3);
    planner.release("b");
    const auto after = sorted_tasks(planner, planner.tasks_on(1).empty() ? 2 : 1);
    CHECK(std::find(after.begin(), after.end(), "b") == after.end());
}

TEST(3, "a machine with no tasks returns nothing") {
    const std::vector<Machine> machines{{1, 5}};
    const CapacityPlanner planner{std::span<const Machine>(machines)};
    CHECK(planner.tasks_on(1).empty());
    CHECK_THROWS_AS(planner.tasks_on(9), std::out_of_range);
}

TEST(3, "tasks_on tracks placements under random traffic") {
    Rng rng;
    const std::vector<Machine> machines{{1, 1000}, {2, 1000}, {3, 1000}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    std::map<std::string, std::pair<MachineId, std::uint64_t>> placed;
    std::map<MachineId, std::set<std::string>> on_machine;
    int counter = 0;
    for (int step = 0; step < 2000; ++step) {
        INFO("step ", step);
        if (placed.empty() || rng.coin(0.7)) {
            const std::string name = "t" + std::to_string(counter++);
            const auto demand = static_cast<std::uint64_t>(rng.uniform(1, 5));
            if (const auto got = planner.place(name, demand)) {
                placed[name] = {*got, demand};
                on_machine[*got].insert(name);
            }
        } else {
            auto it = placed.begin();
            std::advance(it, static_cast<std::ptrdiff_t>(rng.uniform(0, static_cast<std::int64_t>(placed.size()) - 1)));
            on_machine[it->second.first].erase(it->first);
            planner.release(it->first);
            placed.erase(it);
        }
        if (step % 50 == 0) {
            for (MachineId id : {MachineId{1}, MachineId{2}, MachineId{3}}) {
                REQUIRE_EQ(sorted_tasks(planner, id),
                           std::vector<std::string>(on_machine[id].begin(), on_machine[id].end()));
            }
        }
    }
}

// Capacity 1 and demand 1 force exactly one task per machine, whatever the placement policy.
// Filtering a global task list per call would cost about 1e10 steps here.
PERF(3, "tasks_on is proportional to the answer size", 2000) {
    const std::size_t count = 100'000;
    std::vector<Machine> machines;
    machines.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        machines.push_back({i + 1, 1});
    }
    CapacityPlanner planner{std::span<const Machine>(machines)};
    for (std::size_t i = 0; i < count; ++i) {
        REQUIRE(planner.place("task" + std::to_string(i), 1).has_value());
    }
    std::size_t total = 0;
    for (std::size_t i = 0; i < count; ++i) {
        total += planner.tasks_on(machines[i].id).size();
    }
    CHECK_EQ(total, count);
}

TEST(4, "drain moves every task elsewhere") {
    const std::vector<Machine> machines{{1, 10}, {2, 10}, {3, 10}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("a", 3);
    planner.place("b", 3);
    planner.place("c", 3);
    // Machine 1 is whichever holds tasks; find one that does.
    MachineId loaded = 0;
    for (MachineId id : {MachineId{1}, MachineId{2}, MachineId{3}}) {
        if (!planner.tasks_on(id).empty()) {
            loaded = id;
            break;
        }
    }
    REQUIRE(loaded != MachineId{0});
    const auto moving = planner.tasks_on(loaded);
    CHECK(planner.drain(loaded));
    CHECK(planner.tasks_on(loaded).empty());
    CHECK_EQ(planner.available(loaded), 10);
    std::size_t elsewhere = 0;
    for (MachineId id : {MachineId{1}, MachineId{2}, MachineId{3}}) {
        if (id != loaded) {
            elsewhere += planner.tasks_on(id).size();
        }
    }
    CHECK_EQ(elsewhere, 3);
    CHECK(planner.available(1) + planner.available(2) + planner.available(3) == 21);
}

TEST(4, "drain never counts the drained machine itself as a destination") {
    // After "t" lands on machine 1 it still has the most free space, so a re-place that did not
    // exclude machine 1 would put the task straight back and report success.
    const std::vector<Machine> machines{{1, 20}, {2, 8}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    CHECK_EQ(planner.place("t", 9), std::optional<MachineId>(1));  // only machine 1 can hold 9
    CHECK_EQ(planner.available(1), 11);
    CHECK(!planner.drain(1));
    CHECK_EQ(planner.tasks_on(1), std::vector<std::string>{"t"});
    CHECK_EQ(planner.available(1), 11);
    CHECK_EQ(planner.available(2), 8);
}

// Every placement here has exactly one feasible machine, so the layout holds for any policy.
// Machine 2 ends up with room for one of a and b but not both: whichever is tried first moves,
// the other cannot, and the drain has to undo the first move.
TEST(4, "a failed drain rolls back tasks that had already been moved") {
    const std::vector<Machine> machines{{1, 8}, {2, 9}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    REQUIRE_EQ(planner.place("blocker", 9), std::optional<MachineId>(2));
    REQUIRE_EQ(planner.place("a", 4), std::optional<MachineId>(1));
    REQUIRE_EQ(planner.place("b", 4), std::optional<MachineId>(1));
    planner.release("blocker");
    REQUIRE_EQ(planner.place("c", 5), std::optional<MachineId>(2));

    CHECK(!planner.drain(1));

    CHECK_EQ(sorted_tasks(planner, 1), std::vector<std::string>{"a", "b"});
    CHECK_EQ(sorted_tasks(planner, 2), std::vector<std::string>{"c"});
    CHECK_EQ(planner.available(1), 0);
    CHECK_EQ(planner.available(2), 4);
    // Where each task lives must be restored too, or this release corrupts the books.
    planner.release("a");
    CHECK_EQ(planner.available(1), 4);
    CHECK_EQ(planner.available(2), 4);
    planner.release("b");
    planner.release("c");
    CHECK_EQ(planner.available(1), 8);
    CHECK_EQ(planner.available(2), 9);
}

TEST(4, "a failed drain restores the exact previous state") {
    const std::vector<Machine> machines{{1, 6}, {2, 7}, {3, 2}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("a", 7);  // only machine 2 fits
    planner.place("b", 4);  // machine 1
    planner.place("c", 2);
    // Force a known layout: b on 1, c on 1 or 3 depending on heap choice.
    const auto before1 = sorted_tasks(planner, 1);
    const auto before2 = sorted_tasks(planner, 2);
    const auto before3 = sorted_tasks(planner, 3);
    const auto free1 = planner.available(1);
    const auto free2 = planner.available(2);
    const auto free3 = planner.available(3);

    CHECK(!planner.drain(2));  // task of size 7 cannot go anywhere else

    CHECK_EQ(sorted_tasks(planner, 1), before1);
    CHECK_EQ(sorted_tasks(planner, 2), before2);
    CHECK_EQ(sorted_tasks(planner, 3), before3);
    CHECK_EQ(planner.available(1), free1);
    CHECK_EQ(planner.available(2), free2);
    CHECK_EQ(planner.available(3), free3);
    // The planner must still be fully consistent afterwards.
    planner.release("a");
    CHECK_EQ(planner.available(2), 7);
}

TEST(4, "draining an empty machine succeeds and machines stay usable") {
    const std::vector<Machine> machines{{1, 4}, {2, 4}};
    CapacityPlanner planner{std::span<const Machine>(machines)};
    planner.place("a", 1);
    MachineId empty_machine = planner.tasks_on(1).empty() ? 1 : 2;
    CHECK(planner.drain(empty_machine));
    CHECK(planner.place("b", 4).has_value() || planner.place("b", 3).has_value());
    CHECK_THROWS_AS(planner.drain(99), std::out_of_range);
}

TEST(4, "random drains either finish completely or change nothing") {
    Rng rng;
    const std::size_t count = 16;
    std::vector<Machine> machines;
    std::uint64_t total_capacity = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const auto capacity = static_cast<std::uint64_t>(rng.uniform(8, 40));
        machines.push_back({i + 1, capacity});
        total_capacity += capacity;
    }
    CapacityPlanner planner{std::span<const Machine>(machines)};

    // Fill the cluster until it is nearly full, so that drains regularly fail partway through.
    std::map<std::string, std::uint64_t> placed;
    int counter = 0;
    const auto try_place = [&](std::uint64_t demand) {
        const std::string name = "t" + std::to_string(counter++);
        if (planner.place(name, demand).has_value()) {
            placed[name] = demand;
        }
    };
    for (int i = 0; i < 500; ++i) {
        try_place(static_cast<std::uint64_t>(rng.uniform(1, 9)));
    }

    const auto capacity_of = [&](MachineId id) { return machines[static_cast<std::size_t>(id - 1)].capacity; };
    const auto snapshot = [&] {
        std::vector<std::pair<std::uint64_t, std::vector<std::string>>> state;
        for (const Machine& machine : machines) {
            state.emplace_back(planner.available(machine.id), sorted_tasks(planner, machine.id));
        }
        return state;
    };

    int failed = 0;
    for (int round = 0; round < 1000; ++round) {
        INFO("round ", round);
        const auto machine = static_cast<MachineId>(rng.uniform(1, static_cast<std::int64_t>(count)));
        const auto before = snapshot();
        if (planner.drain(machine)) {
            REQUIRE(planner.tasks_on(machine).empty());
            REQUIRE_EQ(planner.available(machine), capacity_of(machine));
        } else {
            ++failed;
            REQUIRE(snapshot() == before);
        }

        std::uint64_t free_total = 0;
        std::size_t task_total = 0;
        for (const Machine& m : machines) {
            free_total += planner.available(m.id);
            task_total += planner.tasks_on(m.id).size();
        }
        std::uint64_t used = 0;
        for (const auto& [name, demand] : placed) {
            used += demand;
        }
        REQUIRE_EQ(free_total, total_capacity - used);
        REQUIRE_EQ(task_total, placed.size());

        if (rng.coin(0.5) && !placed.empty()) {
            auto it = placed.begin();
            std::advance(it, static_cast<std::ptrdiff_t>(rng.uniform(0, static_cast<std::int64_t>(placed.size()) - 1)));
            planner.release(it->first);
            placed.erase(it);
        } else {
            try_place(static_cast<std::uint64_t>(rng.uniform(1, 9)));
        }
    }
    CHECK_GT(failed, 0);
}
