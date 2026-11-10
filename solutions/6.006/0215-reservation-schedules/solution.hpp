#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

struct JobRequest {
    std::int64_t start;
    std::int64_t end;
};

struct Reservation {
    std::int64_t machines;
    std::int64_t start;
    std::int64_t end;
    friend bool operator==(const Reservation&, const Reservation&) = default;
    friend std::ostream& operator<<(std::ostream& os, const Reservation& r) {
        return os << '(' << r.machines << ", [" << r.start << ',' << r.end << "))";
    }
};

using Schedule = std::vector<Reservation>;

namespace detail {

// Appends a piece, coalescing it with the previous one when possible.
inline void append_piece(Schedule& out, std::int64_t machines, std::int64_t start, std::int64_t end) {
    if (!out.empty() && out.back().end == start && out.back().machines == machines) {
        out.back().end = end;
    } else {
        out.push_back({machines, start, end});
    }
}

}  // namespace detail

inline Schedule merge_schedules(const Schedule& a, const Schedule& b) {
    Schedule out;
    out.reserve(a.size() + b.size());
    std::size_t i = 0;
    std::size_t j = 0;
    // Before the loop, t is the earliest instant any reservation could cover.
    std::int64_t t = std::numeric_limits<std::int64_t>::min();
    if (!a.empty() && !b.empty()) {
        t = std::min(a.front().start, b.front().start);
    } else if (!a.empty()) {
        t = a.front().start;
    } else if (!b.empty()) {
        t = b.front().start;
    }

    for (;;) {
        // Drop finished reservations: their contribution ended at or before t.
        while (i < a.size() && a[i].end <= t) {
            ++i;
        }
        while (j < b.size() && b[j].end <= t) {
            ++j;
        }
        if (i == a.size() && j == b.size()) {
            return out;
        }
        const bool in_a = i < a.size() && a[i].start <= t;
        const bool in_b = j < b.size() && b[j].start <= t;
        // The next event: the nearest start or end among the two head reservations.
        std::int64_t next = std::numeric_limits<std::int64_t>::max();
        if (i < a.size()) {
            next = std::min(next, in_a ? a[i].end : a[i].start);
        }
        if (j < b.size()) {
            next = std::min(next, in_b ? b[j].end : b[j].start);
        }
        if (in_a || in_b) {
            const std::int64_t machines = (in_a ? a[i].machines : 0) + (in_b ? b[j].machines : 0);
            detail::append_piece(out, machines, t, next);
        }
        t = next;
    }
}

inline Schedule satisfying_schedule(std::span<const JobRequest> jobs) {
    if (jobs.empty()) {
        return {};
    }
    if (jobs.size() == 1) {
        if (jobs[0].start >= jobs[0].end) {
            throw std::invalid_argument("job must have start < end");
        }
        return {Reservation{1, jobs[0].start, jobs[0].end}};
    }
    const std::size_t middle = jobs.size() / 2;
    const Schedule left = satisfying_schedule(jobs.first(middle));
    const Schedule right = satisfying_schedule(jobs.subspan(middle));
    return merge_schedules(left, right);
}

class IncrementalPlanner {
public:
    void add(JobRequest job) {
        if (job.start >= job.end) {
            throw std::invalid_argument("job must have start < end");
        }
        Schedule carry = {Reservation{1, job.start, job.end}};
        std::size_t level = 0;
        // Merge equal-sized runs, like carrying in a binary counter.
        while (level < runs_.size() && !runs_[level].empty()) {
            carry = merge_schedules(carry, runs_[level]);
            runs_[level].clear();
            ++level;
        }
        if (level == runs_.size()) {
            runs_.emplace_back();
        }
        runs_[level] = std::move(carry);
        ++size_;
    }

    Schedule schedule() const {
        // Smallest runs first, so the accumulated schedule stays small during merging.
        Schedule combined;
        for (const Schedule& run : runs_) {
            if (!run.empty()) {
                combined = merge_schedules(combined, run);
            }
        }
        return combined;
    }

    std::int64_t peak() const {
        std::int64_t best = 0;
        for (const Reservation& piece : schedule()) {
            best = std::max(best, piece.machines);
        }
        return best;
    }

    std::size_t size() const { return size_; }

private:
    std::vector<Schedule> runs_;  // runs_[i] holds 2^i jobs, or is empty
    std::size_t size_ = 0;
};
