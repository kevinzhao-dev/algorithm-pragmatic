#include <algorithm>
#include <atomic>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include "forkjoin.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::atomic<std::int64_t> identity_calls{0};
std::atomic<std::int64_t> reduce_calls{0};

struct IntSum {
    using value_type = long long;
    static value_type identity() {
        identity_calls.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }
    static void reduce(value_type& left, value_type&& right) {
        reduce_calls.fetch_add(1, std::memory_order_relaxed);
        left += right;
    }
};

struct Concat {
    using value_type = std::string;
    static value_type identity() {
        identity_calls.fetch_add(1, std::memory_order_relaxed);
        return std::string();
    }
    static void reduce(value_type& left, value_type&& right) {
        reduce_calls.fetch_add(1, std::memory_order_relaxed);
        left += right;
    }
};

struct IntLog {
    using value_type = std::vector<int>;
    static value_type identity() { return {}; }
    static void reduce(value_type& left, value_type&& right) {
        left.insert(left.end(), right.begin(), right.end());
    }
};

void reset_counters() {
    identity_calls.store(0);
    reduce_calls.store(0);
}

std::string serial_text(std::size_t n) {
    std::string text;
    for (std::size_t i = 0; i < n; ++i) {
        text += static_cast<char>('a' + (i % 26));
    }
    return text;
}

}  // namespace

TEST(1, "a reducer without forks behaves like a plain variable") {
    Reducer<IntSum> reducer(5);
    reducer.view() += 3;
    reducer.view() += 4;
    CHECK_EQ(reducer.take(), 12);
    Reducer<Concat> empty;
    CHECK_EQ(empty.take(), "");
}

TEST(1, "both branches of fork2 contribute") {
    practice::set_workers(4);
    Reducer<IntSum> reducer(100);
    reducer.fork2([&] { reducer.view() += 1; }, [&] { reducer.view() += 2; });
    CHECK_EQ(reducer.take(), 103);
}

TEST(1, "fork2 preserves left-to-right order of a non-commutative monoid") {
    practice::set_workers(4);
    Reducer<Concat> reducer(std::string("<"));
    reducer.fork2(
        [&] {
            reducer.view() += "a";
            reducer.fork2([&] { reducer.view() += "b"; }, [&] { reducer.view() += "c"; });
            reducer.view() += "d";
        },
        [&] {
            reducer.view() += "e";
            reducer.fork2([&] { reducer.view() += "f"; }, [&] { reducer.view() += "g"; });
        });
    reducer.view() += ">";
    CHECK_EQ(reducer.take(), "<abcdefg>");
}

TEST(1, "parallel_for covers every index once") {
    practice::set_workers(4);
    Reducer<IntSum> reducer;
    reducer.parallel_for(0, 100'000, [&](std::size_t i) { reducer.view() += static_cast<long long>(i); }, 8);
    CHECK_EQ(reducer.take(), 99'999LL * 100'000LL / 2);
}

TEST(1, "empty parallel_for changes nothing") {
    Reducer<IntSum> reducer(9);
    reducer.parallel_for(5, 5, [&](std::size_t) { reducer.view() += 1; });
    CHECK_EQ(reducer.take(), 9);
}

TEST(1, "serial order survives heavy stealing for any worker count") {
    for (const unsigned workers : {1U, 2U, 4U, 8U}) {
        practice::set_workers(workers);
        const std::size_t n = 20'000;
        Reducer<Concat> reducer;
        reducer.parallel_for(0, n, [&](std::size_t i) {
            reducer.view() += static_cast<char>('a' + (i % 26));
        });
        INFO("workers ", workers);
        CHECK_EQ(reducer.take(), serial_text(n));
    }
}

TEST(1, "nested recursion keeps serial order") {
    practice::set_workers(8);
    Reducer<IntLog> reducer;
    const auto emit = [&](int lo, int hi, const auto& self) -> void {
        if (hi - lo == 1) {
            reducer.view().push_back(lo);
            return;
        }
        const int mid = lo + (hi - lo) / 2;
        reducer.fork2([&] { self(lo, mid, self); }, [&] { self(mid, hi, self); });
    };
    emit(0, 50'000, emit);
    std::vector<int> expected(50'000);
    std::iota(expected.begin(), expected.end(), 0);
    CHECK_EQ(reducer.take(), expected);
}

TEST(1, "repeated independent reducers do not interfere") {
    practice::set_workers(4);
    for (int round = 0; round < 50; ++round) {
        Reducer<IntSum> reducer;
        reducer.parallel_for(0, 1000, [&](std::size_t) { reducer.view() += 1; });
        CHECK_EQ(reducer.take(), 1000);
    }
}

TEST(2, "a single worker creates no views and performs no reduces") {
    practice::set_workers(1);
    reset_counters();
    Reducer<Concat> reducer(std::string("x"));
    reducer.parallel_for(0, 5000, [&](std::size_t i) {
        reducer.view() += static_cast<char>('a' + (i % 26));
    });
    CHECK_EQ(identity_calls.load(), 0);
    CHECK_EQ(reduce_calls.load(), 0);
    CHECK_EQ(reducer.take(), "x" + serial_text(5000));
}

TEST(2, "nested forks on one worker also avoid new views") {
    practice::set_workers(1);
    reset_counters();
    Reducer<IntSum> reducer(0);
    const auto recurse = [&](int depth, const auto& self) -> void {
        if (depth == 0) {
            reducer.view() += 1;
            return;
        }
        reducer.fork2([&] { self(depth - 1, self); }, [&] { self(depth - 1, self); });
    };
    recurse(10, recurse);
    CHECK_EQ(reducer.take(), 1024);
    CHECK_EQ(identity_calls.load(), 0);
    CHECK_EQ(reduce_calls.load(), 0);
}

TEST(2, "laziness does not break order under contention") {
    for (const unsigned workers : {2U, 4U, 8U}) {
        practice::set_workers(workers);
        for (int round = 0; round < 20; ++round) {
            const std::size_t n = 3000;
            Reducer<Concat> reducer;
            reducer.parallel_for(0, n, [&](std::size_t i) {
                reducer.view() += static_cast<char>('a' + (i % 26));
            });
            INFO("workers ", workers, " round ", round);
            REQUIRE_EQ(reducer.take(), serial_text(n));
        }
    }
}

PERF(2, "a single-worker reducer is nearly free", 3000) {
    practice::set_workers(1);
    Reducer<IntSum> reducer(0);
    const auto recurse = [&](int depth, const auto& self) -> void {
        if (depth == 0) {
            reducer.view() += 1;
            return;
        }
        reducer.fork2([&] { self(depth - 1, self); }, [&] { self(depth - 1, self); });
    };
    recurse(21, recurse);  // 2M leaves, 2M forks
    CHECK_EQ(reducer.take(), 1LL << 21);
}

namespace {

const std::vector<int> kQueenCounts = {1, 0, 0, 2, 10, 4, 40, 92, 352, 724};

bool valid_board(std::uint64_t board, int n) {
    std::vector<int> columns;
    for (int row = 0; row < n; ++row) {
        columns.push_back(static_cast<int>((board >> (4 * row)) & 0xF));
    }
    for (int a = 0; a < n; ++a) {
        if (columns[static_cast<std::size_t>(a)] >= n) {
            return false;
        }
        for (int b = a + 1; b < n; ++b) {
            const int da = columns[static_cast<std::size_t>(a)];
            const int db = columns[static_cast<std::size_t>(b)];
            if (da == db || da - db == a - b || da - db == b - a) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

TEST(3, "serial enumeration finds every solution in order") {
    practice::set_workers(1);
    for (int n = 1; n <= 8; ++n) {
        const std::vector<std::uint64_t> boards = queens_ordered(n, 0);
        INFO("n ", n);
        CHECK_EQ(boards.size(), kQueenCounts[static_cast<std::size_t>(n - 1)]);
        for (const std::uint64_t board : boards) {
            CHECK(valid_board(board, n));
        }
        CHECK(std::is_sorted(boards.begin(), boards.end(),
                             [&](std::uint64_t a, std::uint64_t b) {
                                 for (int r = 0; r < n; ++r) {
                                     const int ca = static_cast<int>((a >> (4 * r)) & 0xF);
                                     const int cb = static_cast<int>((b >> (4 * r)) & 0xF);
                                     if (ca != cb) {
                                         return ca < cb;
                                     }
                                 }
                                 return false;
                             }));
    }
}

TEST(3, "the parallel result matches the serial one element for element") {
    const std::vector<std::uint64_t> expected = queens_ordered(9, 0);
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        for (const int depth : {1, 2, 3}) {
            const std::vector<std::uint64_t> got = queens_ordered(9, depth);
            INFO("workers ", workers, " depth ", depth);
            REQUIRE_EQ(got.size(), expected.size());
            CHECK(got == expected);
        }
    }
}
