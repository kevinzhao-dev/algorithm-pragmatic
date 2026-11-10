#include <memory>
#include <thread>
#include <vector>

#include "08_dsched.hpp"
#include "practice.hpp"
#include "solution.hpp"

namespace dsched = practice::dsched;

namespace {

// A dsched-visible critical-section probe: fails if two threads are ever inside at once.
struct Probe {
    dsched::Atomic<int> inside{0};
    int violations = 0;

    void enter_and_leave() {
        if (inside.fetch_add(1) != 0) {
            ++violations;
        }
        dsched::yield();
        inside.fetch_sub(1);
    }
};

template <typename Make, typename Lock, typename Unlock>
dsched::Report explore_exclusion(Make make_lock, Lock lock, Unlock unlock, long runs = 2500) {
    return dsched::explore_random(
        [=] {
            auto probe = std::make_shared<Probe>();
            auto shared_lock = make_lock();
            dsched::Scenario scenario;
            for (int id = 0; id < 2; ++id) {
                scenario.threads.push_back([=] {
                    for (int round = 0; round < 2; ++round) {
                        lock(*shared_lock, id);
                        probe->enter_and_leave();
                        unlock(*shared_lock, id);
                    }
                });
            }
            scenario.check = [probe] { return probe->violations == 0; };
            return scenario;
        },
        runs);
}

// Real threads incrementing a plain int: a lock without happens-before edges is a TSan report.
template <typename Lock, typename Unlock>
void hammer(int threads, int iterations, Lock lock, Unlock unlock) {
    int counter = 0;
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            for (int i = 0; i < iterations; ++i) {
                lock(t);
                ++counter;
                unlock(t);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    CHECK_EQ(counter, threads * iterations);
}

using TtasSched = TtasLock<dsched::Atomic>;
using CompetitiveSched = CompetitiveLock<dsched::Atomic>;
using PetersonSched = PetersonLock<dsched::Atomic>;

}  // namespace

TEST(1, "lock and unlock cycle repeatedly") {
    TtasLock<> lock;
    lock.lock();
    lock.unlock();
    lock.lock();
    lock.unlock();
}

TEST(1, "try_lock fails while held and succeeds after unlock") {
    TtasLock<> lock;
    CHECK(lock.try_lock());
    CHECK(!lock.try_lock());
    lock.unlock();
    CHECK(lock.try_lock());
    lock.unlock();
}

TEST(1, "no two threads are in the critical section under explored schedules") {
    const auto report = explore_exclusion(
        [] { return std::make_shared<TtasSched>(); },
        [](TtasSched& lock, int) { lock.lock(); }, [](TtasSched& lock, int) { lock.unlock(); });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(1, "real threads count without a data race") {
    TtasLock<> lock;
    hammer(
        4, 2000, [&](int) { lock.lock(); }, [&](int) { lock.unlock(); });
}

TEST(2, "an uncontended lock never yields") {
    int yields = 0;
    CompetitiveLock<> lock(4, [&] { ++yields; });
    for (int i = 0; i < 100; ++i) {
        lock.lock();
        lock.unlock();
    }
    CHECK_EQ(yields, 0);
    CHECK(lock.try_lock());
    CHECK(!lock.try_lock());
    lock.unlock();
}

TEST(2, "a contended lock yields after spinning the limit") {
    CompetitiveLock<>* lock_pointer = nullptr;
    int yields = 0;
    CompetitiveLock<> lock(8, [&] {
        ++yields;
        lock_pointer->unlock();  // the "other thread" lets go, so the next attempt succeeds
    });
    lock_pointer = &lock;
    lock.lock();
    lock.lock();  // held: must spin, then yield once
    CHECK_EQ(yields, 1);
    lock.unlock();
}

TEST(2, "a spin limit of zero yields on the first failure") {
    CompetitiveLock<>* lock_pointer = nullptr;
    int yields = 0;
    CompetitiveLock<> lock(0, [&] {
        ++yields;
        lock_pointer->unlock();
    });
    lock_pointer = &lock;
    lock.lock();
    lock.lock();
    CHECK_EQ(yields, 1);
    lock.unlock();
}

TEST(2, "yield is called repeatedly while the lock stays held") {
    CompetitiveLock<>* lock_pointer = nullptr;
    int yields = 0;
    CompetitiveLock<> lock(3, [&] {
        if (++yields == 5) {
            lock_pointer->unlock();
        }
    });
    lock_pointer = &lock;
    lock.lock();
    lock.lock();
    CHECK_EQ(yields, 5);
    lock.unlock();
}

TEST(2, "exclusion holds under explored schedules") {
    const auto report = explore_exclusion(
        [] {
            return std::make_shared<CompetitiveSched>(2, [] { dsched::yield(); });
        },
        [](CompetitiveSched& lock, int) { lock.lock(); },
        [](CompetitiveSched& lock, int) { lock.unlock(); });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(2, "real threads count without a data race") {
    CompetitiveLock<> lock(32);
    hammer(
        4, 2000, [&](int) { lock.lock(); }, [&](int) { lock.unlock(); });
}

TEST(3, "one thread alone is never blocked") {
    PetersonLock<> lock;
    for (int i = 0; i < 100; ++i) {
        lock.lock(0);
        lock.unlock(0);
        lock.lock(1);
        lock.unlock(1);
    }
}

TEST(3, "exclusion holds under explored schedules") {
    const auto report = explore_exclusion(
        [] { return std::make_shared<PetersonSched>(); },
        [](PetersonSched& lock, int id) { lock.lock(id); },
        [](PetersonSched& lock, int id) { lock.unlock(id); }, 4000);
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(3, "two real threads count without a data race") {
    PetersonLock<> lock;
    hammer(
        2, 3000, [&](int id) { lock.lock(id); }, [&](int id) { lock.unlock(id); });
}
