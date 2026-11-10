#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;

using Heap = UpdatableHeap<int>;
using Handle = Heap::Handle;

namespace {

int ceil_lg(std::size_t n) {
    int bits = 0;
    while ((std::size_t{1} << bits) < n) {
        ++bits;
    }
    return bits;
}

}  // namespace

TEST(1, "a new heap is empty") {
    Heap heap;
    CHECK(heap.empty());
    CHECK_EQ(heap.size(), 0);
    CHECK_THROWS_AS(heap.top(), std::out_of_range);
    CHECK_THROWS_AS(heap.top_handle(), std::out_of_range);
    CHECK_THROWS_AS(heap.pop(), std::out_of_range);
}

TEST(1, "push and pop keep maximum-first order") {
    Heap heap;
    for (int value : {4, 9, 1, 7}) {
        heap.push(value);
    }
    CHECK_EQ(heap.size(), 4);
    CHECK_EQ(heap.top(), 9);
    CHECK_EQ(heap.pop(), 9);
    CHECK_EQ(heap.pop(), 7);
    CHECK_EQ(heap.pop(), 4);
    CHECK_EQ(heap.pop(), 1);
    CHECK(heap.empty());
}

TEST(1, "handles keep working as other elements move around") {
    Heap heap;
    std::vector<Handle> handles;
    for (int value = 0; value < 100; ++value) {
        handles.push_back(heap.push(value));
    }
    for (int popped = 0; popped < 50; ++popped) {
        heap.pop();
    }
    for (int value = 0; value < 50; ++value) {
        CHECK_EQ(heap.get(handles[static_cast<std::size_t>(value)]), value);
        CHECK(heap.contains(handles[static_cast<std::size_t>(value)]));
    }
}

TEST(1, "top_handle identifies the top element") {
    Heap heap;
    heap.push(1);
    const Handle big = heap.push(5);
    heap.push(3);
    CHECK(heap.top_handle() == big);
    CHECK_EQ(heap.get(heap.top_handle()), 5);
}

TEST(1, "a default handle is never valid") {
    Heap heap;
    heap.push(1);
    const Handle none;
    CHECK(!heap.contains(none));
    CHECK_THROWS_AS(heap.get(none), std::invalid_argument);
}

TEST(1, "popped handles become invalid") {
    Heap heap;
    const Handle a = heap.push(7);
    heap.pop();
    CHECK(!heap.contains(a));
    CHECK_THROWS_AS(heap.get(a), std::invalid_argument);
}

TEST(1, "a custom comparator gives a min-heap") {
    UpdatableHeap<int, std::greater<int>> heap;
    heap.push(5);
    heap.push(1);
    heap.push(3);
    CHECK_EQ(heap.pop(), 1);
    CHECK_EQ(heap.top(), 3);
}

TEST(1, "elements are released") {
    const auto before = Tracked::live();
    {
        struct Less {
            bool operator()(const Tracked& a, const Tracked& b) const { return a.value() < b.value(); }
        };
        UpdatableHeap<Tracked, Less> heap;
        for (int i = 0; i < 300; ++i) {
            heap.push(Tracked(i));
        }
        for (int i = 0; i < 100; ++i) {
            CHECK_EQ(heap.pop().value(), 299 - i);
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "raising a value moves it toward the top") {
    Heap heap;
    heap.push(10);
    const Handle low = heap.push(1);
    heap.push(5);
    heap.update(low, 100);
    CHECK_EQ(heap.top(), 100);
    CHECK_EQ(heap.get(low), 100);
    CHECK(heap.top_handle() == low);
}

TEST(2, "lowering the top element moves it down") {
    Heap heap;
    const Handle top = heap.push(10);
    heap.push(8);
    heap.push(5);
    heap.update(top, 1);
    CHECK_EQ(heap.top(), 8);
    CHECK_EQ(heap.pop(), 8);
    CHECK_EQ(heap.pop(), 5);
    CHECK_EQ(heap.pop(), 1);
}

TEST(2, "updating a stale handle throws") {
    Heap heap;
    const Handle a = heap.push(1);
    heap.pop();
    CHECK_THROWS_AS(heap.update(a, 5), std::invalid_argument);
    CHECK_THROWS_AS(heap.update(Handle{}, 5), std::invalid_argument);
}

TEST(2, "random pushes, pops and updates match a model") {
    Rng rng;
    Heap heap;
    std::vector<Handle> handles;
    std::map<int, int> values;  // id -> value
    std::multiset<int> ordered;
    int next_id = 0;
    std::map<int, std::size_t> slot;
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        const auto action = rng.uniform(0, 3);
        if (ordered.empty() || action == 0) {
            const int value = static_cast<int>(rng.uniform(0, 1000));
            handles.push_back(heap.push(value));
            values[next_id] = value;
            slot[next_id] = handles.size() - 1;
            ordered.insert(value);
            ++next_id;
        } else if (action == 1) {
            REQUIRE_EQ(heap.top(), *ordered.rbegin());
            const int value = heap.pop();
            REQUIRE_EQ(value, *ordered.rbegin());
            ordered.erase(std::prev(ordered.end()));
            // Forget one model entry that holds this value; which one does not matter.
            const auto it = std::find_if(values.begin(), values.end(),
                                         [&](const auto& kv) { return kv.second == value && !heap.contains(handles[slot[kv.first]]); });
            REQUIRE(it != values.end());
            values.erase(it);
        } else if (!values.empty()) {
            auto it = values.begin();
            std::advance(it, static_cast<std::ptrdiff_t>(rng.uniform(0, static_cast<std::int64_t>(values.size()) - 1)));
            const Handle handle = handles[slot[it->first]];
            REQUIRE_EQ(heap.get(handle), it->second);
            const int value = static_cast<int>(rng.uniform(0, 1000));
            ordered.erase(ordered.find(it->second));
            ordered.insert(value);
            it->second = value;
            heap.update(handle, value);
        }
        REQUIRE_EQ(heap.size(), ordered.size());
    }
}

TEST(2, "updates use O(log n) comparisons") {
    Rng rng;
    const std::size_t n = 1 << 17;
    UpdatableHeap<Counted<int>> heap;
    std::vector<UpdatableHeap<Counted<int>>::Handle> handles;
    for (std::size_t i = 0; i < n; ++i) {
        handles.push_back(heap.push(Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000'000)))));
    }
    Counted<int>::comparisons() = 0;
    const int rounds = 20'000;
    for (int i = 0; i < rounds; ++i) {
        heap.update(handles[static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1))],
                    Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000'000))));
    }
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(rounds * (3 * ceil_lg(n) + 8)));
}

PERF(2, "updates on a huge heap cannot scan for the element", 2500) {
    Rng rng;
    Heap heap;
    const int n = 1'000'000;
    std::vector<Handle> handles;
    handles.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        handles.push_back(heap.push(static_cast<int>(rng.uniform(0, 2'000'000'000))));
    }
    for (int i = 0; i < 2'000'000; ++i) {
        heap.update(handles[static_cast<std::size_t>(rng.uniform(0, n - 1))], static_cast<int>(rng.uniform(0, 2'000'000'000)));
    }
    long long sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += heap.pop();
    }
    CHECK_GT(sum, 0);
}

TEST(3, "erase removes exactly that element") {
    Heap heap;
    heap.push(5);
    const Handle target = heap.push(3);
    heap.push(8);
    heap.push(1);
    CHECK_EQ(heap.erase(target), 3);
    CHECK_EQ(heap.size(), 3);
    CHECK(!heap.contains(target));
    CHECK_THROWS_AS(heap.erase(target), std::invalid_argument);
    CHECK_EQ(heap.pop(), 8);
    CHECK_EQ(heap.pop(), 5);
    CHECK_EQ(heap.pop(), 1);
}

TEST(3, "erasing the only element and the top element") {
    Heap heap;
    const Handle lone = heap.push(1);
    CHECK_EQ(heap.erase(lone), 1);
    CHECK(heap.empty());
    heap.push(2);
    const Handle top = heap.push(9);
    heap.push(4);
    CHECK_EQ(heap.erase(top), 9);
    CHECK_EQ(heap.top(), 4);
}

TEST(3, "a stale handle stays stale after its slot is reused") {
    Heap heap;
    const Handle old_handle = heap.push(1);
    heap.pop();
    const Handle new_handle = heap.push(2);
    CHECK(!heap.contains(old_handle));
    CHECK(heap.contains(new_handle));
    CHECK_EQ(heap.get(new_handle), 2);
    CHECK_THROWS_AS(heap.get(old_handle), std::invalid_argument);
    CHECK_THROWS_AS(heap.update(old_handle, 5), std::invalid_argument);
    CHECK_THROWS_AS(heap.erase(old_handle), std::invalid_argument);
    CHECK_EQ(heap.get(new_handle), 2);  // the live element was not touched by the stale calls
    CHECK(old_handle != new_handle);
}

TEST(3, "build replaces the contents with fresh handles in input order") {
    Heap heap;
    heap.push(100);
    const std::vector<int> items{3, 1, 4, 1, 5};
    const auto handles = heap.build(items);
    REQUIRE_EQ(handles.size(), items.size());
    CHECK_EQ(heap.size(), items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        CHECK_EQ(heap.get(handles[i]), items[i]);
    }
    CHECK_EQ(heap.top(), 5);
    CHECK_EQ(heap.pop(), 5);
}

TEST(3, "build of nothing leaves an empty heap") {
    Heap heap;
    heap.push(1);
    CHECK(heap.build({}).empty());
    CHECK(heap.empty());
}

TEST(3, "build costs a linear number of comparisons") {
    Rng rng;
    const std::size_t n = 100'000;
    UpdatableHeap<Counted<int>> heap;
    std::vector<Counted<int>> items;
    for (std::size_t i = 0; i < n; ++i) {
        items.emplace_back(static_cast<int>(rng.uniform(0, 1'000'000'000)));
    }
    Counted<int>::comparisons() = 0;
    heap.build(std::move(items));
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(3 * n));
    CHECK_EQ(heap.size(), n);
}

TEST(3, "random mix of every operation matches a model") {
    Rng rng;
    Heap heap;
    struct Entry {
        Handle handle;
        int value;
        bool alive;
    };
    std::vector<Entry> entries;
    std::multiset<int> ordered;
    std::vector<std::size_t> alive;

    const auto kill = [&](std::size_t alive_index) {
        const std::size_t index = alive[alive_index];
        entries[index].alive = false;
        alive[alive_index] = alive.back();
        alive.pop_back();
        return index;
    };

    for (int step = 0; step < 150'000; ++step) {
        INFO("step ", step);
        const auto action = rng.uniform(0, 4);
        if (ordered.empty() || action == 0) {
            const int value = static_cast<int>(rng.uniform(0, 1000));
            entries.push_back({heap.push(value), value, true});
            alive.push_back(entries.size() - 1);
            ordered.insert(value);
        } else if (action == 1) {
            REQUIRE_EQ(heap.top(), *ordered.rbegin());
            REQUIRE_EQ(heap.get(heap.top_handle()), *ordered.rbegin());
            const Handle handle = heap.top_handle();
            const int value = heap.pop();
            ordered.erase(ordered.find(value));
            const auto it = std::find_if(alive.begin(), alive.end(),
                                         [&](std::size_t index) { return entries[index].handle == handle; });
            REQUIRE(it != alive.end());
            kill(static_cast<std::size_t>(it - alive.begin()));
            REQUIRE(!heap.contains(handle));
        } else if (action == 2) {
            const auto alive_index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(alive.size()) - 1));
            Entry& entry = entries[alive[alive_index]];
            REQUIRE(heap.contains(entry.handle));
            const int value = static_cast<int>(rng.uniform(0, 1000));
            ordered.erase(ordered.find(entry.value));
            ordered.insert(value);
            entry.value = value;
            heap.update(entry.handle, value);
        } else if (action == 3) {
            const auto alive_index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(alive.size()) - 1));
            const std::size_t index = alive[alive_index];
            REQUIRE_EQ(heap.erase(entries[index].handle), entries[index].value);
            ordered.erase(ordered.find(entries[index].value));
            kill(alive_index);
            REQUIRE(!heap.contains(entries[index].handle));
        } else if (!entries.empty()) {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(entries.size()) - 1));
            REQUIRE_EQ(heap.contains(entries[index].handle), entries[index].alive);
        }
        REQUIRE_EQ(heap.size(), ordered.size());
    }
}

TEST(4, "earlier deadlines run first") {
    DeadlineScheduler scheduler;
    CHECK(scheduler.submit("b", 20));
    CHECK(scheduler.submit("a", 10));
    CHECK(scheduler.submit("c", 30));
    CHECK_EQ(scheduler.size(), 3);
    CHECK_EQ(scheduler.peek(), std::optional<std::string>("a"));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("a"));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("b"));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("c"));
    CHECK_EQ(scheduler.next(), std::optional<std::string>());
    CHECK_EQ(scheduler.peek(), std::optional<std::string>());
}

TEST(4, "ties go to the earlier submission") {
    DeadlineScheduler scheduler;
    scheduler.submit("first", 5);
    scheduler.submit("second", 5);
    scheduler.submit("third", 5);
    CHECK_EQ(scheduler.next(), std::optional<std::string>("first"));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("second"));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("third"));
}

TEST(4, "rejects duplicates and unknown names") {
    DeadlineScheduler scheduler;
    CHECK(scheduler.submit("a", 1));
    CHECK(!scheduler.submit("a", 2));
    CHECK_EQ(scheduler.size(), 1);
    CHECK(!scheduler.reschedule("zzz", 3));
    CHECK(!scheduler.cancel("zzz"));
}

TEST(4, "reschedule moves a task but keeps its submission order") {
    DeadlineScheduler scheduler;
    scheduler.submit("a", 1);
    scheduler.submit("b", 2);
    scheduler.submit("c", 3);
    CHECK(scheduler.reschedule("a", 3));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("b"));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("a"));  // submitted before c
    CHECK_EQ(scheduler.next(), std::optional<std::string>("c"));
}

TEST(4, "cancel removes a task and frees the name") {
    DeadlineScheduler scheduler;
    scheduler.submit("a", 1);
    scheduler.submit("b", 2);
    CHECK(scheduler.cancel("a"));
    CHECK_EQ(scheduler.size(), 1);
    CHECK_EQ(scheduler.next(), std::optional<std::string>("b"));
    CHECK(scheduler.submit("a", 7));
    CHECK_EQ(scheduler.next(), std::optional<std::string>("a"));
}

TEST(4, "random workload matches a brute-force scheduler") {
    Rng rng;
    DeadlineScheduler scheduler;
    struct Task {
        std::int64_t deadline;
        std::uint64_t sequence;
    };
    std::map<std::string, Task> model;
    std::uint64_t sequence = 0;
    const auto expected_next = [&]() -> std::optional<std::string> {
        const auto best = std::min_element(model.begin(), model.end(), [](const auto& a, const auto& b) {
            return std::tie(a.second.deadline, a.second.sequence) < std::tie(b.second.deadline, b.second.sequence);
        });
        return best == model.end() ? std::optional<std::string>() : std::optional<std::string>(best->first);
    };
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        const std::string name = "t" + std::to_string(rng.uniform(0, 100));
        const std::int64_t deadline = rng.uniform(0, 500);
        switch (rng.uniform(0, 4)) {
            case 0: {
                const bool fresh = !model.contains(name);
                REQUIRE_EQ(scheduler.submit(name, deadline), fresh);
                if (fresh) {
                    model[name] = {deadline, sequence++};
                }
                break;
            }
            case 1: {
                const auto it = model.find(name);
                REQUIRE_EQ(scheduler.reschedule(name, deadline), it != model.end());
                if (it != model.end()) {
                    it->second.deadline = deadline;
                }
                break;
            }
            case 2:
                REQUIRE_EQ(scheduler.cancel(name), model.erase(name) > 0);
                break;
            case 3:
                REQUIRE_EQ(scheduler.peek(), expected_next());
                break;
            default: {
                const auto expected = expected_next();
                REQUIRE_EQ(scheduler.next(), expected);
                if (expected) {
                    model.erase(*expected);
                }
                break;
            }
        }
        REQUIRE_EQ(scheduler.size(), model.size());
    }
}

PERF(4, "a large schedule under heavy rescheduling", 4000) {
    Rng rng;
    DeadlineScheduler scheduler;
    const int tasks = 200'000;
    for (int i = 0; i < tasks; ++i) {
        scheduler.submit("task" + std::to_string(i), rng.uniform(0, 1'000'000'000));
    }
    for (int i = 0; i < 800'000; ++i) {
        scheduler.reschedule("task" + std::to_string(rng.uniform(0, tasks - 1)), rng.uniform(0, 1'000'000'000));
    }
    std::size_t drained = 0;
    while (scheduler.next().has_value()) {
        ++drained;
    }
    CHECK_EQ(drained, tasks);
}
