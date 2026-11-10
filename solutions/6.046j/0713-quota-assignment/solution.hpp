#pragma once

#include <algorithm>
#include <span>
#include <vector>

namespace quota_detail {

// Augmenting-path engine over resource slots: every request sits on one resource at most,
// holders[j] lists the requests currently using resource j.
class Matcher {
public:
    explicit Matcher(std::vector<int> quota)
        : quota_(std::move(quota)),
          holders_(quota_.size()),
          accepting_(quota_.size()),
          stamp_(quota_.size(), 0),
          round_(0) {}

    int size() const { return static_cast<int>(accepts_.size()); }
    int choice_of(int id) const { return choice_[id]; }

    void add(std::vector<int> accepts) {
        const int id = size();
        for (int resource : accepts) {
            accepting_[resource].push_back(id);
        }
        accepts_.push_back(std::move(accepts));
        choice_.push_back(-1);
        cancelled_.push_back(0);
    }

    bool active(int id) const { return !cancelled_[id]; }

    // Tries to assign `id` an accepted resource, displacing others along an alternating path.
    bool try_assign(int id) {
        ++round_;
        return dfs(id);
    }

    // Frees id's seat, then lets a rejected request reach that seat by one alternating chain.
    void cancel(int id) {
        cancelled_[id] = 1;
        const int resource = choice_[id];
        if (resource != -1) {
            auto& list = holders_[resource];
            list.erase(std::find(list.begin(), list.end(), id));
            choice_[id] = -1;
        }
        for (int r : accepts_[id]) {
            auto& list = accepting_[r];
            list.erase(std::find(list.begin(), list.end(), id));
        }
        accepts_[id].clear();
        if (resource != -1) {
            ++round_;
            fill_spare_slot(resource);
        }
    }

    int rejected() const {
        int count = 0;
        for (int id = 0; id < size(); ++id) {
            count += active(id) && choice_[id] == -1 ? 1 : 0;
        }
        return count;
    }

private:
    // `resource` has (or is about to have) a free seat: find a chain ending at a rejected
    // request, shifting the requests along it towards this seat.
    bool fill_spare_slot(int resource) {
        stamp_[resource] = round_;
        for (int q : accepting_[resource]) {
            if (choice_[q] == resource) {
                continue;
            }
            if (choice_[q] == -1) {
                place(q, resource);
                return true;
            }
            const int other = choice_[q];
            if (stamp_[other] != round_ && fill_spare_slot(other)) {
                place(q, resource);
                return true;
            }
        }
        return false;
    }

    bool dfs(int id) {
        for (int resource : accepts_[id]) {
            if (stamp_[resource] == round_) {
                continue;
            }
            stamp_[resource] = round_;
            if (static_cast<int>(holders_[resource].size()) < quota_[resource]) {
                place(id, resource);
                return true;
            }
            for (int holder : holders_[resource]) {
                if (holder != id && dfs(holder)) {
                    // The holder moved elsewhere (place() removed it from this list).
                    place(id, resource);
                    return true;
                }
            }
        }
        return false;
    }

    void place(int id, int resource) {
        if (choice_[id] != -1 && choice_[id] != resource) {
            auto& old = holders_[choice_[id]];
            old.erase(std::find(old.begin(), old.end(), id));
        }
        if (choice_[id] != resource) {
            holders_[resource].push_back(id);
        }
        choice_[id] = resource;
    }

    std::vector<int> quota_;
    std::vector<std::vector<int>> accepts_;
    std::vector<int> choice_;
    std::vector<char> cancelled_;
    std::vector<std::vector<int>> holders_;
    std::vector<std::vector<int>> accepting_;
    std::vector<int> stamp_;
    int round_;
};

}  // namespace quota_detail

struct Plan {
    std::vector<int> choice;
    int rejected = 0;
};

inline Plan best_plan(std::span<const std::vector<int>> accepts, std::span<const int> quota) {
    quota_detail::Matcher matcher(std::vector<int>(quota.begin(), quota.end()));
    const int n = static_cast<int>(accepts.size());
    for (int i = 0; i < n; ++i) {
        matcher.add(accepts[i]);
    }
    for (int i = 0; i < n; ++i) {
        matcher.try_assign(i);
    }
    Plan plan;
    plan.choice.resize(n);
    for (int i = 0; i < n; ++i) {
        plan.choice[i] = matcher.choice_of(i);
        plan.rejected += plan.choice[i] == -1 ? 1 : 0;
    }
    return plan;
}

struct Shortage {
    std::vector<int> requests;
    std::vector<int> resources;
};

inline Shortage shortage_report(std::span<const std::vector<int>> accepts,
                                std::span<const int> quota) {
    const Plan plan = best_plan(accepts, quota);
    if (plan.rejected == 0) {
        return {};
    }
    const int n = static_cast<int>(accepts.size());
    const int m = static_cast<int>(quota.size());
    // Alternating reachability from every rejected request.
    std::vector<std::vector<int>> holders(m);
    for (int i = 0; i < n; ++i) {
        if (plan.choice[i] != -1) {
            holders[plan.choice[i]].push_back(i);
        }
    }
    std::vector<char> seen_request(n, 0), seen_resource(m, 0);
    std::vector<int> stack;
    for (int i = 0; i < n; ++i) {
        if (plan.choice[i] == -1) {
            seen_request[i] = 1;
            stack.push_back(i);
        }
    }
    while (!stack.empty()) {
        const int i = stack.back();
        stack.pop_back();
        for (int r : accepts[i]) {
            if (!seen_resource[r]) {
                seen_resource[r] = 1;
                for (int h : holders[r]) {
                    if (!seen_request[h]) {
                        seen_request[h] = 1;
                        stack.push_back(h);
                    }
                }
            }
        }
    }
    Shortage out;
    for (int i = 0; i < n; ++i) {
        if (seen_request[i]) {
            out.requests.push_back(i);
        }
    }
    for (int r = 0; r < m; ++r) {
        if (seen_resource[r]) {
            out.resources.push_back(r);
        }
    }
    return out;
}

class Allocator {
public:
    explicit Allocator(std::vector<int> quota) : matcher_(std::move(quota)) {}

    int add_request(std::vector<int> accepts) {
        const int id = matcher_.size();
        matcher_.add(std::move(accepts));
        matcher_.try_assign(id);
        return id;
    }

    void cancel(int id) { matcher_.cancel(id); }

    int choice_of(int id) const { return matcher_.choice_of(id); }
    int rejected() const { return matcher_.rejected(); }

private:
    quota_detail::Matcher matcher_;
};
