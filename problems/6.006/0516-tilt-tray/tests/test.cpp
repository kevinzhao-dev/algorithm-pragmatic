#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::array<Tilt, 4> kAllTilts = {Tilt::Up, Tilt::Down, Tilt::Left, Tilt::Right};

std::pair<int, int> direction_vector(Tilt tilt) {
    switch (tilt) {
        case Tilt::Up:
            return {-1, 0};
        case Tilt::Down:
            return {1, 0};
        case Tilt::Left:
            return {0, -1};
        case Tilt::Right:
            return {0, 1};
    }
    return {0, 0};
}

// Moves pucks one cell at a time until nothing can move: slow, obviously matches the statement.
Board model_tilt(Board board, Tilt tilt) {
    const int n = static_cast<int>(board.size());
    const auto [dr, dc] = direction_vector(tilt);
    bool moved = true;
    while (moved) {
        moved = false;
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                if (board[idx(r)][idx(c)] != 'o') {
                    continue;
                }
                const int nr = r + dr;
                const int nc = c + dc;
                if (nr >= 0 && nr < n && nc >= 0 && nc < n && board[idx(nr)][idx(nc)] == '.') {
                    board[idx(r)][idx(c)] = '.';
                    board[idx(nr)][idx(nc)] = 'o';
                    moved = true;
                }
            }
        }
    }
    return board;
}

Board random_board(Rng& rng, int n, double pillars, double pucks) {
    Board board(idx(n), std::string(idx(n), '.'));
    for (auto& row : board) {
        for (char& cell : row) {
            const double roll = rng.real();
            cell = roll < pillars ? '#' : (roll < pillars + pucks ? 'o' : '.');
        }
    }
    return board;
}

// Breadth-first over board values using the slow model; the ground truth for small boards.
struct ModelResult {
    std::map<Board, int> depth;
};

std::optional<int> model_solve(const Board& start, int row, int col) {
    std::map<Board, int> depth{{start, 0}};
    std::vector<Board> frontier{start};
    if (start[idx(row)][idx(col)] == 'o') {
        return 0;
    }
    for (std::size_t head = 0; head < frontier.size(); ++head) {
        const Board current = frontier[head];
        const int d = depth[current];
        for (const Tilt tilt : kAllTilts) {
            Board next = model_tilt(current, tilt);
            if (depth.emplace(next, d + 1).second) {
                if (next[idx(row)][idx(col)] == 'o') {
                    return d + 1;
                }
                frontier.push_back(std::move(next));
            }
        }
    }
    return std::nullopt;
}

std::size_t model_reachable(const Board& start) {
    std::set<Board> seen{start};
    std::vector<Board> frontier{start};
    for (std::size_t head = 0; head < frontier.size(); ++head) {
        const Board current = frontier[head];
        for (const Tilt tilt : kAllTilts) {
            Board next = model_tilt(current, tilt);
            if (seen.insert(next).second) {
                frontier.push_back(std::move(next));
            }
        }
    }
    return seen.size();
}

// Tries every placement of the same number of pucks and counts those that tilt onto `board`.
std::uint64_t model_predecessors(const Board& board, Tilt tilt) {
    const int n = static_cast<int>(board.size());
    std::vector<std::pair<int, int>> free_cells;
    int pucks = 0;
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (board[idx(r)][idx(c)] != '#') {
                free_cells.emplace_back(r, c);
            }
            pucks += board[idx(r)][idx(c)] == 'o' ? 1 : 0;
        }
    }
    std::uint64_t count = 0;
    const std::uint32_t limit = 1u << free_cells.size();
    for (std::uint32_t mask = 0; mask < limit; ++mask) {
        if (std::popcount(mask) != pucks) {
            continue;
        }
        Board candidate = board;
        for (std::size_t i = 0; i < free_cells.size(); ++i) {
            candidate[idx(free_cells[i].first)][idx(free_cells[i].second)] =
                ((mask >> i) & 1u) != 0 ? 'o' : '.';
        }
        if (model_tilt(candidate, tilt) == board) {
            ++count;
        }
    }
    return count;
}

int count_pucks(const Board& board) {
    int total = 0;
    for (const auto& row : board) {
        total += static_cast<int>(std::count(row.begin(), row.end(), 'o'));
    }
    return total;
}

void check_solution(const Board& start, int row, int col, const std::vector<Tilt>& moves,
                    int expected) {
    REQUIRE_EQ(moves.size(), expected);
    Board current = start;
    for (const Tilt tilt : moves) {
        current = model_tilt(current, tilt);
    }
    CHECK_EQ(current[idx(row)][idx(col)], 'o');
}

// Deterministic generator, independent of the standard library's distributions.
struct Lcg {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<std::uint32_t>(state >> 33);
    }
};

Board generated_board(std::uint64_t seed, int n, std::uint32_t pillars_permille, int pucks) {
    Lcg lcg{seed};
    Board board(idx(n), std::string(idx(n), '.'));
    std::vector<std::pair<int, int>> open;
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (lcg.next() % 1000 < pillars_permille) {
                board[idx(r)][idx(c)] = '#';
            } else {
                open.emplace_back(r, c);
            }
        }
    }
    for (int i = static_cast<int>(open.size()) - 1; i > 0; --i) {
        std::swap(open[idx(i)], open[lcg.next() % (static_cast<std::uint32_t>(i) + 1)]);
    }
    for (int i = 0; i < pucks; ++i) {
        board[idx(open[idx(i)].first)][idx(open[idx(i)].second)] = 'o';
    }
    return board;
}

// 12x12 with 322,904 reachable configurations; its deepest target needs 79 tilts.
const Board& deep_board() {
    static const Board board = {
        "......#...##", "#..o#..#...#", "o........o#o", "..#.o..o.##o",
        "#..#....#.#.", "#...###...#.", "........oo..", "o##....#.##o",
        "....#...#...", ".........#.o", "#....#..#..#", "#.......#oo.",
    };
    return board;
}

}  // namespace

TEST(1, "tilting left packs a row against the wall") {
    const Board board = {"o.o", "...", "..o"};
    CHECK_EQ(apply_tilt(board, Tilt::Left), Board{"oo.", "...", "o.."});
}

TEST(1, "tilting right packs against the other wall") {
    const Board board = {"o.o", "...", "o.."};
    CHECK_EQ(apply_tilt(board, Tilt::Right), Board{".oo", "...", "..o"});
}

TEST(1, "tilting up and down move along columns") {
    const Board board = {"o..", "..o", "o.."};
    CHECK_EQ(apply_tilt(board, Tilt::Up), Board{"o.o", "o..", "..."});
    CHECK_EQ(apply_tilt(board, Tilt::Down), Board{"...", "o..", "o.o"});
}

TEST(1, "pillars stop pucks and split the line") {
    const Board board = {"...", ".o.", "#.o"};
    CHECK_EQ(apply_tilt(board, Tilt::Left), Board{"...", "o..", "#o."});
    const Board row = {"o#o", "...", "..."};
    CHECK_EQ(apply_tilt(row, Tilt::Right), Board{"o#o", "...", "..."});
}

TEST(1, "a one-cell board never changes") {
    const Board puck = {"o"};
    CHECK_EQ(apply_tilt(puck, Tilt::Left), puck);
    const Board pillar = {"#"};
    CHECK_EQ(apply_tilt(pillar, Tilt::Up), pillar);
    const Board empty = {"."};
    CHECK_EQ(apply_tilt(empty, Tilt::Down), empty);
}

TEST(1, "tilting twice in the same direction changes nothing more") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 8));
        const Board board = random_board(rng, n, 0.15, 0.3);
        INFO("round ", round);
        for (const Tilt tilt : kAllTilts) {
            const Board once = apply_tilt(board, tilt);
            REQUIRE_EQ(apply_tilt(once, tilt), once);
            REQUIRE_EQ(count_pucks(once), count_pucks(board));
        }
    }
}

TEST(1, "random boards match cell-by-cell sliding") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const Board board = random_board(rng, n, rng.real() * 0.3, rng.real() * 0.5);
        INFO("round ", round, " board=", board);
        for (const Tilt tilt : kAllTilts) {
            REQUIRE_EQ(apply_tilt(board, tilt), model_tilt(board, tilt));
        }
    }
}

// Sliding each puck one cell at a time costs O(n^3) on boards like this.
PERF(1, "tilting a large board is quadratic in n", 2500) {
    const int n = 3000;
    Board board(idx(n), std::string(idx(n), '.'));
    for (auto& row : board) {
        for (std::size_t c = 0; c < row.size(); c += 3) {
            row[c] = 'o';
        }
    }
    for (int round = 0; round < 8; ++round) {
        board = apply_tilt(board, kAllTilts[static_cast<std::size_t>(round) % 4]);
    }
    CHECK_EQ(count_pucks(board), n * (n / 3));
}

TEST(2, "a puck already on the target needs no moves") {
    const Board board = {".o.", "...", "..."};
    CHECK_EQ(solve_board(board, 0, 1), std::optional(std::vector<Tilt>{}));
}

TEST(2, "a pillar cell can never hold a puck") {
    const Board board = {"#o.", "...", "..."};
    CHECK_EQ(solve_board(board, 0, 0), std::optional<std::vector<Tilt>>());
}

TEST(2, "a target off the board throws out_of_range") {
    const Board board = {"..", ".."};
    CHECK_THROWS_AS(solve_board(board, 2, 0), std::out_of_range);
    CHECK_THROWS_AS(solve_board(board, 0, -1), std::out_of_range);
}

TEST(2, "no pucks means no solution") {
    const Board board = {"...", "...", "..."};
    CHECK_EQ(solve_board(board, 1, 1), std::optional<std::vector<Tilt>>());
}

TEST(2, "a two-move solution around a pillar") {
    const Board board = {"....", ".#o.", "....", "...."};
    const auto moves = solve_board(board, 3, 0);
    REQUIRE(moves.has_value());
    check_solution(board, 3, 0, *moves, 2);
}

TEST(2, "pucks only rest where they can stop") {
    // On an empty row the target in the middle is unreachable: a puck can only stop at the ends.
    const Board board = {"o..", "...", "..."};
    CHECK_EQ(solve_board(board, 1, 1), std::optional<std::vector<Tilt>>());
}

TEST(2, "random boards match a breadth-first search over board values") {
    Rng rng;
    int solved = 0;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 5));
        const Board board = random_board(rng, n, rng.real() * 0.3, 0.1 + rng.real() * 0.2);
        const int row = static_cast<int>(rng.uniform(0, n - 1));
        const int col = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " board=", board, " target=", row, ",", col);
        const auto expected = model_solve(board, row, col);
        const auto moves = solve_board(board, row, col);
        REQUIRE_EQ(moves.has_value(), expected.has_value());
        if (moves) {
            check_solution(board, row, col, *moves, *expected);
            ++solved;
        }
    }
    CHECK_GT(solved, 100);
}

// Enumerating the whole reachable set first never finishes on a board this large.
PERF(2, "a shallow target does not pay for the whole state space", 400) {
    const Board board = generated_board(3, 30, 200, 120);
    // Find a cell first occupied after exactly two tilts.
    std::vector<std::vector<char>> early(idx(30), std::vector<char>(idx(30), 0));
    const auto mark = [&](const Board& b) {
        for (int r = 0; r < 30; ++r) {
            for (int c = 0; c < 30; ++c) {
                early[idx(r)][idx(c)] |= b[idx(r)][idx(c)] == 'o' ? 1 : 0;
            }
        }
    };
    mark(board);
    for (const Tilt tilt : kAllTilts) {
        mark(model_tilt(board, tilt));
    }
    const Board target_board = model_tilt(model_tilt(board, Tilt::Right), Tilt::Down);
    int row = -1;
    int col = -1;
    for (int r = 0; r < 30 && row == -1; ++r) {
        for (int c = 0; c < 30; ++c) {
            if (target_board[idx(r)][idx(c)] == 'o' && early[idx(r)][idx(c)] == 0) {
                row = r;
                col = c;
                break;
            }
        }
    }
    REQUIRE(row != -1);
    const auto moves = solve_board(board, row, col);
    REQUIRE(moves.has_value());
    CHECK_EQ(moves->size(), 2);
}

// Without a visited set the same boards are expanded 4^79 times; a linear scan is also hopeless.
PERF(2, "a target that needs seventy-nine tilts", 4000) {
    const auto moves = solve_board(deep_board(), 8, 6);
    REQUIRE(moves.has_value());
    CHECK_EQ(moves->size(), 79);
}

PERF(2, "an unreachable target exhausts the state space and stops", 4000) {
    CHECK_EQ(solve_board(deep_board(), 5, 2), std::optional<std::vector<Tilt>>());
}

TEST(3, "a settled board with one puck is its own closure") {
    const Board board = {"o#", "#."};
    CHECK_EQ(reachable_configurations(board), 1);
}

TEST(3, "a puck in the open reaches every rim cell and its start") {
    const Board board = {"...", ".o.", "..."};
    CHECK_EQ(reachable_configurations(board), 9);
}

TEST(3, "random boards match a closure over board values") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 5));
        const Board board = random_board(rng, n, rng.real() * 0.3, 0.1 + rng.real() * 0.2);
        INFO("round ", round, " board=", board);
        const std::size_t expected = model_reachable(board);
        REQUIRE_EQ(reachable_configurations(board), expected);
    }
}

TEST(3, "the reachable count never exceeds the binomial bound") {
    Rng rng;
    std::vector<std::vector<std::uint64_t>> binom(37, std::vector<std::uint64_t>(37, 0));
    for (std::size_t i = 0; i < binom.size(); ++i) {
        binom[i][0] = 1;
        for (std::size_t j = 1; j <= i; ++j) {
            binom[i][j] = binom[i - 1][j - 1] + binom[i - 1][j];
        }
    }
    for (int round = 0; round < 150; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 5));
        const Board board = random_board(rng, n, 0.2, 0.2);
        int pillars = 0;
        for (const auto& row : board) {
            pillars += static_cast<int>(std::count(row.begin(), row.end(), '#'));
        }
        const std::uint64_t limit = binom[idx(n * n - pillars)][idx(count_pucks(board))];
        INFO("round ", round, " board=", board);
        REQUIRE(reachable_configurations(board) <= limit);
    }
}

PERF(3, "counting over three hundred thousand configurations", 4000) {
    CHECK_EQ(reachable_configurations(deep_board()), 322'904);
}

TEST(3, "an unsettled board has no predecessors") {
    const Board board = {".#.", ".o.", "..."};
    CHECK_EQ(count_predecessors(board, Tilt::Down), 0);
}

TEST(3, "each puck on the floor could have come from anywhere in its column") {
    const Board board = {"...", "...", "o.o"};
    CHECK_EQ(count_predecessors(board, Tilt::Down), 9);
}

TEST(3, "pillars split a line into independent runs") {
    const Board board = {".#o", ".o.", "..."};
    CHECK_EQ(count_predecessors(board, Tilt::Up), 6);
}

TEST(3, "an empty board has exactly one predecessor") {
    const Board board = {"..", ".."};
    CHECK_EQ(count_predecessors(board, Tilt::Left), 1);
}

TEST(3, "predecessor counts match trying every placement") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 4));
        Board board = random_board(rng, n, rng.real() * 0.3, 0.1 + rng.real() * 0.3);
        const Tilt tilt = kAllTilts[static_cast<std::size_t>(rng.uniform(0, 3))];
        if (rng.coin(0.7)) {
            board = model_tilt(board, tilt);
        }
        INFO("round ", round, " board=", board);
        REQUIRE_EQ(count_predecessors(board, tilt), model_predecessors(board, tilt));
    }
}
