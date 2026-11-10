#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

enum class Tilt { Up, Down, Left, Right };

using Board = std::vector<std::string>;

namespace detail {

inline constexpr std::array<Tilt, 4> kDirections = {Tilt::Up, Tilt::Down, Tilt::Left, Tilt::Right};

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

inline std::string flatten(const Board& board) {
    std::string cells;
    cells.reserve(board.size() * board.size());
    for (const auto& row : board) {
        cells += row;
    }
    return cells;
}

inline Board unflatten(const std::string& cells, int n) {
    Board board(idx(n));
    for (int r = 0; r < n; ++r) {
        board[idx(r)] = cells.substr(idx(r * n), idx(n));
    }
    return board;
}

// Each line is walked starting from the wall the pucks slide towards.
struct LineView {
    int start;
    int step;
};

inline LineView line_view(int n, int line, Tilt direction) {
    switch (direction) {
        case Tilt::Up:
            return {line, n};
        case Tilt::Down:
            return {(n - 1) * n + line, -n};
        case Tilt::Left:
            return {line * n, 1};
        case Tilt::Right:
            return {line * n + n - 1, -1};
    }
    return {0, 0};
}

inline void tilt_in_place(std::string& cells, int n, Tilt direction) {
    for (int line = 0; line < n; ++line) {
        const LineView view = line_view(n, line, direction);
        int write = 0;
        for (int position = 0; position < n; ++position) {
            char& cell = cells[idx(view.start + position * view.step)];
            if (cell == '#') {
                write = position + 1;
            } else if (cell == 'o') {
                cell = '.';
                cells[idx(view.start + write * view.step)] = 'o';
                ++write;
            }
        }
    }
}

// Configurations are created on demand; the map doubles as the visited set and the parent table.
struct Discovered {
    int parent;
    Tilt via;
};

template <typename Stop>
inline std::optional<std::vector<Tilt>> explore(const Board& start, Stop stop, std::size_t* count) {
    const int n = static_cast<int>(start.size());
    std::string initial = flatten(start);
    std::unordered_map<std::string, int> ids;
    std::vector<const std::string*> states;
    std::vector<Discovered> origin;

    const auto path_to = [&](int state) {
        std::vector<Tilt> moves;
        for (int s = state; origin[idx(s)].parent != -1; s = origin[idx(s)].parent) {
            moves.push_back(origin[idx(s)].via);
        }
        std::reverse(moves.begin(), moves.end());
        return moves;
    };

    ids.emplace(std::move(initial), 0);
    states.push_back(&ids.begin()->first);
    origin.push_back({-1, Tilt::Up});
    if (stop(*states[0])) {
        return std::vector<Tilt>{};
    }

    for (std::size_t head = 0; head < states.size(); ++head) {
        for (const Tilt direction : kDirections) {
            std::string next = *states[head];
            tilt_in_place(next, n, direction);
            const auto [position, inserted] = ids.try_emplace(std::move(next), static_cast<int>(states.size()));
            if (!inserted) {
                continue;
            }
            states.push_back(&position->first);
            origin.push_back({static_cast<int>(head), direction});
            if (stop(position->first)) {
                return path_to(position->second);
            }
        }
    }
    if (count != nullptr) {
        *count = states.size();
    }
    return std::nullopt;
}

}  // namespace detail

inline Board apply_tilt(const Board& board, Tilt direction) {
    std::string cells = detail::flatten(board);
    const int n = static_cast<int>(board.size());
    detail::tilt_in_place(cells, n, direction);
    return detail::unflatten(cells, n);
}

inline std::optional<std::vector<Tilt>> solve_board(const Board& start, int row, int col) {
    const int n = static_cast<int>(start.size());
    if (row < 0 || row >= n || col < 0 || col >= n) {
        throw std::out_of_range("target is off the board");
    }
    const auto target = detail::idx(row * n + col);
    return detail::explore(start, [&](const std::string& cells) { return cells[target] == 'o'; },
                           nullptr);
}

inline std::size_t reachable_configurations(const Board& start) {
    std::size_t count = 0;
    detail::explore(start, [](const std::string&) { return false; }, &count);
    return count;
}

inline std::uint64_t count_predecessors(const Board& board, Tilt direction) {
    const int n = static_cast<int>(board.size());
    std::string cells = detail::flatten(board);
    {
        std::string settled = cells;
        detail::tilt_in_place(settled, n, direction);
        if (settled != cells) {
            return 0;
        }
    }
    std::vector<std::vector<std::uint64_t>> pascal(detail::idx(n) + 1,
                                                   std::vector<std::uint64_t>(detail::idx(n) + 1, 0));
    for (std::size_t i = 0; i < pascal.size(); ++i) {
        pascal[i][0] = 1;
        for (std::size_t j = 1; j <= i; ++j) {
            pascal[i][j] = pascal[i - 1][j - 1] + pascal[i - 1][j];
        }
    }
    // A settled line splits at pillars into independent runs; a run of length L holding c pucks
    // could have started from any of C(L, c) arrangements.
    std::uint64_t total = 1;
    for (int line = 0; line < n; ++line) {
        const detail::LineView view = detail::line_view(n, line, direction);
        int run_length = 0;
        int pucks = 0;
        const auto close_run = [&] {
            total *= pascal[detail::idx(run_length)][detail::idx(pucks)];
            run_length = 0;
            pucks = 0;
        };
        for (int position = 0; position < n; ++position) {
            const char cell = cells[detail::idx(view.start + position * view.step)];
            if (cell == '#') {
                close_run();
            } else {
                ++run_length;
                pucks += cell == 'o' ? 1 : 0;
            }
        }
        close_run();
    }
    return total;
}
