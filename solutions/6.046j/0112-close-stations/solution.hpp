#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <span>
#include <utility>
#include <vector>

struct Station {
    std::int64_t x;
    std::int64_t y;
};

namespace close_detail {

inline std::int64_t distance_sq(const Station& a, const Station& b) {
    const std::int64_t dx = a.x - b.x;
    const std::int64_t dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Sorts by x once; every recursive level hands its ids back sorted by y, so the merge below is
// the only per-level sorting work.
class Searcher {
public:
    Searcher(std::span<const Station> stations, std::int64_t radius_sq)
        : stations_(stations), radius_sq_(radius_sq), by_x_(stations.size()), buffer_(stations.size()) {
        std::iota(by_x_.begin(), by_x_.end(), 0);
        std::ranges::sort(by_x_, {}, [&](int i) { return at(i).x; });
    }

    std::optional<std::pair<int, int>> find_pair() {
        std::optional<std::pair<int, int>> found;
        pair_search(0, by_x_.size(), found);
        return found;
    }

    std::optional<std::array<int, 3>> find_triple() {
        std::optional<std::array<int, 3>> found;
        triple_search(0, by_x_.size(), found);
        return found;
    }

private:
    const Station& at(int i) const { return stations_[static_cast<std::size_t>(i)]; }

    bool close(int a, int b) const { return distance_sq(at(a), at(b)) <= radius_sq_; }

    bool within_y(int low, int high) const {
        const std::int64_t dy = at(high).y - at(low).y;
        return dy * dy <= radius_sq_;
    }

    void merge_by_y(std::size_t first, std::size_t middle, std::size_t last) {
        std::ranges::merge(by_x_.begin() + static_cast<std::ptrdiff_t>(first),
                           by_x_.begin() + static_cast<std::ptrdiff_t>(middle),
                           by_x_.begin() + static_cast<std::ptrdiff_t>(middle),
                           by_x_.begin() + static_cast<std::ptrdiff_t>(last), buffer_.begin(), {},
                           [&](int i) { return at(i).y; }, [&](int i) { return at(i).y; });
        std::copy(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(last - first),
                  by_x_.begin() + static_cast<std::ptrdiff_t>(first));
    }

    std::vector<int> strip(std::size_t first, std::size_t last, std::int64_t split_x) const {
        std::vector<int> ids;
        for (std::size_t k = first; k < last; ++k) {
            const std::int64_t dx = at(by_x_[k]).x - split_x;
            if (dx * dx <= radius_sq_) {
                ids.push_back(by_x_[k]);
            }
        }
        return ids;
    }

    void pair_search(std::size_t first, std::size_t last, std::optional<std::pair<int, int>>& found) {
        if (last - first <= 1) {
            return;
        }
        const std::size_t middle = first + (last - first) / 2;
        const std::int64_t split_x = at(by_x_[middle]).x;
        pair_search(first, middle, found);
        if (found) {
            return;
        }
        pair_search(middle, last, found);
        if (found) {
            return;
        }
        merge_by_y(first, middle, last);

        // No close pair inside either half, so the geometry bounds this loop by a constant.
        const std::vector<int> ids = strip(first, last, split_x);
        for (std::size_t i = 0; i < ids.size(); ++i) {
            for (std::size_t j = i + 1; j < ids.size() && within_y(ids[i], ids[j]); ++j) {
                if (close(ids[i], ids[j])) {
                    found = std::minmax(ids[i], ids[j]);
                    return;
                }
            }
        }
    }

    void triple_search(std::size_t first, std::size_t last, std::optional<std::array<int, 3>>& found) {
        if (last - first <= 2) {
            merge_small(first, last);
            return;
        }
        const std::size_t middle = first + (last - first) / 2;
        const std::int64_t split_x = at(by_x_[middle]).x;
        triple_search(first, middle, found);
        if (found) {
            return;
        }
        triple_search(middle, last, found);
        if (found) {
            return;
        }
        merge_by_y(first, middle, last);

        const std::vector<int> ids = strip(first, last, split_x);
        for (std::size_t i = 0; i < ids.size(); ++i) {
            for (std::size_t j = i + 1; j < ids.size() && within_y(ids[i], ids[j]); ++j) {
                if (!close(ids[i], ids[j])) {
                    continue;
                }
                for (std::size_t k = j + 1; k < ids.size() && within_y(ids[i], ids[k]); ++k) {
                    if (close(ids[i], ids[k]) && close(ids[j], ids[k])) {
                        std::array<int, 3> triple{ids[i], ids[j], ids[k]};
                        std::ranges::sort(triple);
                        found = triple;
                        return;
                    }
                }
            }
        }
    }

    void merge_small(std::size_t first, std::size_t last) {
        if (last - first == 2 && at(by_x_[first + 1]).y < at(by_x_[first]).y) {
            std::swap(by_x_[first], by_x_[first + 1]);
        }
    }

    std::span<const Station> stations_;
    std::int64_t radius_sq_;
    std::vector<int> by_x_;
    std::vector<int> buffer_;
};

}  // namespace close_detail

inline std::optional<std::pair<int, int>> find_close_pair(std::span<const Station> stations,
                                                          std::int64_t radius_sq) {
    return close_detail::Searcher(stations, radius_sq).find_pair();
}

inline std::optional<std::array<int, 3>> find_close_triple(std::span<const Station> stations,
                                                           std::int64_t radius_sq) {
    return close_detail::Searcher(stations, radius_sq).find_triple();
}
