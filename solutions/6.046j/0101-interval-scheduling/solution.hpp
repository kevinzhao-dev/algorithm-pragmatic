#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

struct Request {
    std::int64_t start;
    std::int64_t finish;
    std::int64_t value;
};

struct Schedule {
    std::int64_t total = 0;
    std::vector<int> chosen;
};

namespace interval_detail {

inline void validate(std::span<const Request> requests) {
    for (const Request& r : requests) {
        if (r.start >= r.finish || r.value < 0) {
            throw std::invalid_argument("request needs start < finish and value >= 0");
        }
    }
}

inline std::vector<int> order_by_finish(std::span<const Request> requests) {
    std::vector<int> order(requests.size());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::sort(order, {}, [&](int i) { return requests[static_cast<std::size_t>(i)].finish; });
    return order;
}

}  // namespace interval_detail

inline std::vector<int> max_compatible(std::span<const Request> requests) {
    interval_detail::validate(requests);
    std::vector<int> chosen;
    bool any = false;
    std::int64_t last_finish = 0;
    for (int index : interval_detail::order_by_finish(requests)) {
        const Request& r = requests[static_cast<std::size_t>(index)];
        if (!any || r.start >= last_finish) {
            chosen.push_back(index);
            last_finish = r.finish;
            any = true;
        }
    }
    return chosen;
}

inline Schedule max_value_schedule(std::span<const Request> requests) {
    interval_detail::validate(requests);
    const std::vector<int> order = interval_detail::order_by_finish(requests);
    const std::size_t n = order.size();

    std::vector<std::int64_t> finishes(n);
    for (std::size_t i = 0; i < n; ++i) {
        finishes[i] = requests[static_cast<std::size_t>(order[i])].finish;
    }

    // best[k] is the optimum over the k earliest-finishing requests.
    std::vector<std::int64_t> best(n + 1, 0);
    std::vector<std::size_t> previous(n);
    for (std::size_t i = 0; i < n; ++i) {
        const Request& r = requests[static_cast<std::size_t>(order[i])];
        // start < finish, so everything finishing by r.start already sorts before r.
        previous[i] = static_cast<std::size_t>(
            std::ranges::upper_bound(finishes, r.start) - finishes.begin());
        best[i + 1] = std::max(best[i], r.value + best[previous[i]]);
    }

    Schedule schedule;
    schedule.total = best[n];
    for (std::size_t k = n; k > 0;) {
        const Request& r = requests[static_cast<std::size_t>(order[k - 1])];
        if (r.value + best[previous[k - 1]] > best[k - 1]) {
            schedule.chosen.push_back(order[k - 1]);
            k = previous[k - 1];
        } else {
            --k;
        }
    }
    std::ranges::reverse(schedule.chosen);
    return schedule;
}

class WhatIfPlanner {
public:
    explicit WhatIfPlanner(std::span<const Request> requests)
        : requests_(requests.begin(), requests.end()) {
        interval_detail::validate(requests_);
        const std::size_t n = requests_.size();

        std::vector<int> by_finish = interval_detail::order_by_finish(requests_);
        finishes_.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            finishes_[i] = requests_[static_cast<std::size_t>(by_finish[i])].finish;
        }
        best_prefix_.assign(n + 1, 0);
        for (std::size_t i = 0; i < n; ++i) {
            const Request& r = requests_[static_cast<std::size_t>(by_finish[i])];
            const auto before = std::ranges::upper_bound(finishes_, r.start) - finishes_.begin();
            best_prefix_[i + 1] =
                std::max(best_prefix_[i], r.value + best_prefix_[static_cast<std::size_t>(before)]);
        }

        std::vector<int> by_start(n);
        std::iota(by_start.begin(), by_start.end(), 0);
        std::ranges::sort(by_start, {},
                          [&](int i) { return requests_[static_cast<std::size_t>(i)].start; });
        starts_.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            starts_[i] = requests_[static_cast<std::size_t>(by_start[i])].start;
        }
        best_suffix_.assign(n + 1, 0);
        for (std::size_t i = n; i-- > 0;) {
            const Request& r = requests_[static_cast<std::size_t>(by_start[i])];
            const auto after = std::ranges::lower_bound(starts_, r.finish) - starts_.begin();
            best_suffix_[i] =
                std::max(best_suffix_[i + 1], r.value + best_suffix_[static_cast<std::size_t>(after)]);
        }
    }

    std::int64_t best_total() const { return best_prefix_.back(); }

    std::int64_t best_with(std::size_t index) const {
        const Request& r = requests_.at(index);
        const auto before = std::ranges::upper_bound(finishes_, r.start) - finishes_.begin();
        const auto after = std::ranges::lower_bound(starts_, r.finish) - starts_.begin();
        return best_prefix_[static_cast<std::size_t>(before)] + r.value +
               best_suffix_[static_cast<std::size_t>(after)];
    }

    std::int64_t regret(std::size_t index) const { return best_total() - best_with(index); }

private:
    std::vector<Request> requests_;
    std::vector<std::int64_t> finishes_;
    std::vector<std::int64_t> starts_;
    std::vector<std::int64_t> best_prefix_;
    std::vector<std::int64_t> best_suffix_;
};
