#include <algorithm>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

namespace {

// Known solution counts for 1..12 queens.
const std::vector<int> kCounts = {1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200};

// Validates one solution: each row has a column in range, and no two queens attack.
bool is_valid(Board board, int n) {
    std::vector<int> columns;
    for (int row = 0; row < n; ++row) {
        const int column = board_column(board, row);
        if (column < 0 || column >= n) {
            return false;
        }
        columns.push_back(column);
    }
    for (int a = 0; a < n; ++a) {
        for (int b = a + 1; b < n; ++b) {
            if (columns[a] == columns[b]) {
                return false;
            }
            if (columns[a] - columns[b] == a - b || columns[a] - columns[b] == b - a) {
                return false;
            }
        }
    }
    return true;
}

// Number of backtracking nodes visited (every call, including row == n leaves), computed
// independently of the solution.
long long node_count(int n, int row, unsigned down, unsigned left, unsigned right) {
    long long nodes = 1;
    if (row == n) {
        return nodes;
    }
    const unsigned all = (1U << n) - 1;
    unsigned available = ~(down | left | right) & all;
    while (available != 0) {
        const unsigned place = available & (~available + 1);
        available ^= place;
        nodes += node_count(n, row + 1, down | place, ((left | place) << 1) & all, (right | place) >> 1);
    }
    return nodes;
}

std::vector<Board> sorted_boards(const ResultList<Board>& list) {
    std::vector<Board> boards = list.to_vector();
    std::sort(boards.begin(), boards.end());
    return boards;
}

}  // namespace

TEST(1, "a new list is empty") {
    ResultList<int> list;
    CHECK(list.empty());
    CHECK_EQ(list.size(), 0);
    CHECK_EQ(list.to_vector(), std::vector<int>{});
}

TEST(1, "push_back appends in order") {
    ResultList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    CHECK(!list.empty());
    CHECK_EQ(list.size(), 3);
    CHECK_EQ(list.to_vector(), std::vector<int>({1, 2, 3}));
}

TEST(1, "merge concatenates and empties the source") {
    ResultList<int> a;
    a.push_back(1);
    ResultList<int> b;
    b.push_back(2);
    b.push_back(3);
    a.merge(b);
    CHECK_EQ(a.to_vector(), std::vector<int>({1, 2, 3}));
    CHECK_EQ(a.size(), 3);
    CHECK(b.empty());
    CHECK_EQ(b.size(), 0);
    a.push_back(4);
    b.push_back(9);
    CHECK_EQ(a.to_vector(), std::vector<int>({1, 2, 3, 4}));
    CHECK_EQ(b.to_vector(), std::vector<int>({9}));
}

TEST(1, "merging with empty lists and itself is safe") {
    ResultList<int> a;
    ResultList<int> empty;
    a.merge(empty);
    CHECK(a.empty());
    empty.merge(a);
    CHECK(empty.empty());
    ResultList<int> b;
    b.push_back(1);
    ResultList<int> c;
    b.merge(c);
    CHECK_EQ(b.to_vector(), std::vector<int>{1});
    c.merge(b);
    CHECK_EQ(c.to_vector(), std::vector<int>{1});
    CHECK(b.empty());
    c.merge(c);
    CHECK_EQ(c.to_vector(), std::vector<int>{1});
    CHECK_EQ(c.size(), 1);
}

TEST(1, "destruction releases every node without recursion") {
    const auto before = Tracked::live();
    {
        ResultList<Tracked> list;
        for (int i = 0; i < 1000; ++i) {
            list.push_back(Tracked(i));
        }
        ResultList<Tracked> other;
        other.push_back(Tracked(-1));
        list.merge(other);
    }
    CHECK_EQ(Tracked::live(), before);
    ResultList<int> huge;
    for (int i = 0; i < 1'000'000; ++i) {
        huge.push_back(i);
    }
}

TEST(1, "moves transfer ownership") {
    const auto before = Tracked::live();
    {
        ResultList<Tracked> a;
        a.push_back(Tracked(1));
        ResultList<Tracked> b(std::move(a));
        CHECK(a.empty());  // NOLINT
        CHECK_EQ(b.size(), 1);
        ResultList<Tracked> c;
        c.push_back(Tracked(5));
        c = std::move(b);
        CHECK(b.empty());  // NOLINT
        CHECK_EQ(c.to_vector()[0].value(), 1);
    }
    CHECK_EQ(Tracked::live(), before);
}

PERF(1, "merge takes constant time", 1000) {
    ResultList<int> sink;
    ResultList<int> source;
    for (int i = 0; i < 5'000'000; ++i) {
        source.push_back(i);
    }
    for (int round = 0; round < 2'000'000; ++round) {
        sink.merge(source);
        source.merge(sink);
    }
    CHECK_EQ(source.size(), 5'000'000);
}

TEST(2, "board packing stores four bits per row") {
    Board board = 0;
    board = board_place(board, 0, 3);
    board = board_place(board, 5, 15);
    board = board_place(board, 15, 1);
    CHECK_EQ(board_column(board, 0), 3);
    CHECK_EQ(board_column(board, 5), 15);
    CHECK_EQ(board_column(board, 15), 1);
    CHECK_EQ(board_column(board, 7), 0);
}

TEST(2, "queens_serial finds the known number of solutions") {
    for (int n = 1; n <= 10; ++n) {
        ResultList<Board> found = queens_serial(n);
        INFO("n ", n);
        CHECK_EQ(found.size(), kCounts[static_cast<std::size_t>(n - 1)]);
    }
}

TEST(2, "every solution is a legal placement and all are distinct") {
    for (int n = 1; n <= 8; ++n) {
        const std::vector<Board> boards = queens_serial(n).to_vector();
        INFO("n ", n);
        for (const Board board : boards) {
            CHECK(is_valid(board, n));
        }
        CHECK_EQ(std::set<Board>(boards.begin(), boards.end()).size(), boards.size());
    }
}

TEST(3, "the serial and parallel versions agree under the meter") {
    for (int n = 1; n <= 9; ++n) {
        exec::Meter meter;
        const ResultList<Board> found = queens_parallel(meter, n, 2);
        INFO("n ", n);
        CHECK_EQ(sorted_boards(found), sorted_boards(queens_serial(n)));
    }
}

TEST(3, "work equals the number of search nodes plus spawn charges") {
    for (int n = 2; n <= 8; ++n) {
        exec::Meter meter;
        queens_parallel(meter, n, 3);
        const long long nodes = node_count(n, 0, 0, 0, 0);
        INFO("n ", n);
        CHECK_EQ(meter.work(), nodes + meter.spawns());
    }
}

TEST(3, "no race and correct results on the real pool") {
    exec::ForkJoin pool;
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        for (int n = 1; n <= 9; ++n) {
            const ResultList<Board> found = queens_parallel(pool, n, 3);
            INFO("workers ", workers, " n ", n);
            CHECK_EQ(found.size(), kCounts[static_cast<std::size_t>(n - 1)]);
        }
    }
    const ResultList<Board> found = queens_parallel(pool, 10, 4);
    CHECK_EQ(sorted_boards(found), sorted_boards(queens_serial(10)));
}

TEST(4, "serial depth zero spawns nothing and depth one spawns once per placement") {
    {
        exec::Meter meter;
        const ResultList<Board> found = queens_parallel(meter, 8, 0);
        CHECK_EQ(meter.spawns(), 0);
        CHECK_EQ(found.size(), 92);
    }
    {
        exec::Meter meter;
        queens_parallel(meter, 8, 1);
        CHECK_EQ(meter.spawns(), 7);  // first row: 8 candidates, 7 forks in a binary split
    }
}

TEST(4, "every depth finds the same solutions") {
    const std::vector<Board> expected = sorted_boards(queens_serial(8));
    for (int depth = 0; depth <= 8; ++depth) {
        exec::Meter meter;
        const ResultList<Board> found = queens_parallel(meter, 8, depth);
        INFO("depth ", depth);
        CHECK_EQ(sorted_boards(found), expected);
    }
}

TEST(4, "deeper forking means more spawns") {
    long long previous = -1;
    for (int depth = 0; depth <= 4; ++depth) {
        exec::Meter meter;
        queens_parallel(meter, 8, depth);
        INFO("depth ", depth);
        CHECK_GT(meter.spawns(), previous);
        previous = meter.spawns();
    }
}
