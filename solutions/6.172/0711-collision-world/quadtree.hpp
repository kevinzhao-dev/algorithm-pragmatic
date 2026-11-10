// quadtree.hpp — broad phase: serial and fork-join detectors over an implicit quadtree.
//
// The tree is never built as nodes. Items live in one flat array; each recursive call partitions
// its range into "straddlers" (boxes crossing a split line, which stay at this node) and the four
// quadrants. A pair is tested exactly once: where the deeper of the two sits, against everything
// on the path above it.
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "collide.hpp"
#include "forkjoin.hpp"
#include "scale.hpp"
#include "world_types.hpp"

struct QuadtreeOptions {
    std::size_t leaf_capacity = 12;
    int max_depth = 16;
};

namespace broad_detail {

struct Aabb {
    std::int64_t x0, y0, x1, y1;
};

constexpr bool overlaps(const Aabb& a, const Aabb& b) {
    return a.x0 <= b.x1 && b.x0 <= a.x1 && a.y0 <= b.y1 && b.y0 <= a.y1;
}

// Everything the bar can occupy during the tick.
constexpr Aabb swept_box(const Segment& s) {
    const std::int64_t low_x = std::min(s.p.x, s.q.x);
    const std::int64_t high_x = std::max(s.p.x, s.q.x);
    const std::int64_t low_y = std::min(s.p.y, s.q.y);
    const std::int64_t high_y = std::max(s.p.y, s.q.y);
    return {low_x + std::min<std::int64_t>(0, s.v.x), low_y + std::min<std::int64_t>(0, s.v.y),
            high_x + std::max<std::int64_t>(0, s.v.x), high_y + std::max<std::int64_t>(0, s.v.y)};
}

struct Item {
    Aabb box;
    std::uint32_t index;
};

// The straddlers on the path from the root; immutable, so tasks can share them freely.
struct Path {
    const Item* first;
    const Item* last;
    const Path* up;
};

struct Cell {
    std::int64_t x;
    std::int64_t y;
    std::int64_t size;
};

constexpr int kStraddle = 4;

// A box goes left only if it ends strictly before the split: quadrants are half-open, boxes are
// closed, so a box whose edge lies exactly on the split line must not be sent to one side.
constexpr int classify(const Aabb& box, const Cell& cell, std::int64_t half) {
    const std::int64_t mid_x = cell.x + half;
    const std::int64_t mid_y = cell.y + half;
    const int column = box.x1 < mid_x ? 0 : (box.x0 >= mid_x ? 1 : -1);
    const int row = box.y1 < mid_y ? 0 : (box.y0 >= mid_y ? 1 : -1);
    return column < 0 || row < 0 ? kStraddle : row * 2 + column;
}

template <typename Pred>
class Solver {
public:
    Solver(std::span<const Segment> segments, Pred& pred, const QuadtreeOptions& options,
           std::size_t grain)
        : segments_(segments), pred_(pred), options_(options), grain_(grain) {}

    std::vector<Hit> run(bool parallel) {
        std::vector<Hit> all;
        if (segments_.size() >= 2) {
            buffers_.resize(parallel ? practice::workers() : 1);
            std::vector<Item> items = make_items();
            const Cell root = root_cell(items);
            solve({items.data(), items.size()}, root, 0, nullptr, parallel);
            for (Buffer& buffer : buffers_) {
                all.insert(all.end(), buffer.hits.begin(), buffer.hits.end());
            }
        }
        std::sort(all.begin(), all.end());
        return all;
    }

private:
    struct alignas(64) Buffer {
        std::vector<Hit> hits;
    };

    std::vector<Item> make_items() const {
        std::vector<Item> items;
        items.reserve(segments_.size());
        for (std::uint32_t i = 0; i < segments_.size(); ++i) {
            items.push_back({swept_box(segments_[i]), i});
        }
        return items;
    }

    static Cell root_cell(std::span<const Item> items) {
        Aabb all = items.front().box;
        for (const Item& item : items) {
            all.x0 = std::min(all.x0, item.box.x0);
            all.y0 = std::min(all.y0, item.box.y0);
            all.x1 = std::max(all.x1, item.box.x1);
            all.y1 = std::max(all.y1, item.box.y1);
        }
        const auto extent = static_cast<std::uint64_t>(std::max(all.x1 - all.x0, all.y1 - all.y0));
        return {all.x0, all.y0, static_cast<std::int64_t>(std::bit_ceil(extent + 1))};
    }

    void emit(const Item& a, const Item& b) {
        if (!overlaps(a.box, b.box)) {
            return;
        }
        const Segment& s = segments_[a.index];
        const Segment& t = segments_[b.index];
        if (pred_(s, t)) {
            buffers_[practice::worker_index()].hits.push_back(
                {std::min(s.id, t.id), std::max(s.id, t.id)});
        }
    }

    void pairs_within(std::span<Item> items) {
        for (std::size_t i = 0; i < items.size(); ++i) {
            for (std::size_t j = i + 1; j < items.size(); ++j) {
                emit(items[i], items[j]);
            }
        }
    }

    void against_path(std::span<Item> items, const Path* path) {
        for (; path != nullptr; path = path->up) {
            for (const Item* other = path->first; other != path->last; ++other) {
                for (const Item& item : items) {
                    emit(item, *other);
                }
            }
        }
    }

    void solve(std::span<Item> items, Cell cell, int depth, const Path* path, bool parallel) {
        if (items.size() <= options_.leaf_capacity || depth >= options_.max_depth ||
            cell.size < 2) {
            pairs_within(items);
            against_path(items, path);
            return;
        }

        const std::int64_t half = cell.size / 2;
        std::array<std::size_t, kStraddle + 2> bounds{};
        Item* cursor = items.data();
        Item* const end = items.data() + items.size();
        const auto partition_group = [&](int group) {
            cursor = std::partition(cursor, end, [&](const Item& item) {
                return classify(item.box, cell, half) == group;
            });
            return static_cast<std::size_t>(cursor - items.data());
        };
        bounds[0] = partition_group(kStraddle);
        for (int quadrant = 0; quadrant < kStraddle; ++quadrant) {
            bounds[static_cast<std::size_t>(quadrant) + 1] = partition_group(quadrant);
        }

        const std::span<Item> straddlers = items.first(bounds[0]);
        pairs_within(straddlers);
        against_path(straddlers, path);

        const Path extended{straddlers.data(), straddlers.data() + straddlers.size(), path};
        const Path* const below = straddlers.empty() ? path : &extended;
        const std::array<Cell, 4> cells{Cell{cell.x, cell.y, half},
                                        Cell{cell.x + half, cell.y, half},
                                        Cell{cell.x, cell.y + half, half},
                                        Cell{cell.x + half, cell.y + half, half}};
        const auto child = [&](std::size_t q) {
            solve(items.subspan(bounds[q], bounds[q + 1] - bounds[q]), cells[q], depth + 1, below,
                  parallel);
        };
        if (parallel && items.size() > grain_) {
            scale::fork2([&] { scale::fork2([&] { child(0); }, [&] { child(1); }); },
                         [&] { scale::fork2([&] { child(2); }, [&] { child(3); }); });
        } else {
            for (std::size_t q = 0; q < 4; ++q) {
                child(q);
            }
        }
    }

    std::span<const Segment> segments_;
    Pred& pred_;
    QuadtreeOptions options_;
    std::size_t grain_;
    std::vector<Buffer> buffers_;
};

}  // namespace broad_detail

template <typename Pred = ExactPredicate>
std::vector<Hit> detect_brute(std::span<const Segment> segments, Pred pred = {}) {
    std::vector<Hit> hits;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        for (std::size_t j = i + 1; j < segments.size(); ++j) {
            if (pred(segments[i], segments[j])) {
                hits.push_back({std::min(segments[i].id, segments[j].id),
                                std::max(segments[i].id, segments[j].id)});
            }
        }
    }
    std::sort(hits.begin(), hits.end());
    return hits;
}

template <typename Pred = ExactPredicate>
std::vector<Hit> detect_quadtree(std::span<const Segment> segments, Pred pred = {},
                                 const QuadtreeOptions& options = {}) {
    return broad_detail::Solver<Pred>(segments, pred, options, 0).run(false);
}

template <typename Pred = ExactPredicate>
std::vector<Hit> detect_parallel(std::span<const Segment> segments, Pred pred = {},
                                 const QuadtreeOptions& options = {}, std::size_t grain = 256) {
    return broad_detail::Solver<Pred>(segments, pred, options, grain).run(true);
}
