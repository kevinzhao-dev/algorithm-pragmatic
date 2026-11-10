#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <list>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename Viewer, typename Message>
class ChatRoom {
public:
    explicit ChatRoom(std::vector<Viewer> viewers,
                      std::size_t per_viewer_limit = std::numeric_limits<std::size_t>::max())
        : limit_(per_viewer_limit) {
        if (per_viewer_limit == 0) {
            throw std::invalid_argument("per-viewer limit must be at least 1");
        }
        std::sort(viewers.begin(), viewers.end());
        if (std::adjacent_find(viewers.begin(), viewers.end()) != viewers.end()) {
            throw std::invalid_argument("duplicate viewer");
        }
        accounts_.reserve(viewers.size());
        for (Viewer& viewer : viewers) {
            accounts_.push_back(Account{std::move(viewer), {}, false});
        }
    }

    bool send(const Viewer& viewer, Message message) {
        Account* account = find(viewer);
        if (account == nullptr || account->banned) {
            return false;
        }
        timeline_.push_front(std::move(message));
        account->sent.push_back(timeline_.begin());
        if (account->sent.size() > limit_) {
            timeline_.erase(account->sent.front());
            account->sent.pop_front();
        }
        return true;
    }

    std::vector<Message> recent(std::size_t k) const {
        std::vector<Message> messages;
        messages.reserve(std::min(k, timeline_.size()));
        for (auto it = timeline_.begin(); it != timeline_.end() && messages.size() < k; ++it) {
            messages.push_back(*it);
        }
        return messages;
    }

    std::size_t message_count() const { return timeline_.size(); }

    bool ban(const Viewer& viewer) {
        Account* account = find(viewer);
        if (account == nullptr || account->banned) {
            return false;
        }
        for (const auto& message : account->sent) {
            timeline_.erase(message);
        }
        account->sent.clear();
        account->banned = true;
        return true;
    }

    bool is_banned(const Viewer& viewer) const {
        const Account* account = find(viewer);
        return account != nullptr && account->banned;
    }

private:
    using Timeline = std::list<Message>;

    struct Account {
        Viewer viewer;
        // Each viewer's own messages, oldest first, as handles into the timeline.
        std::list<typename Timeline::iterator> sent;
        bool banned = false;
    };

    const Account* find(const Viewer& viewer) const {
        std::size_t low = 0;
        std::size_t high = accounts_.size();
        while (low < high) {
            const std::size_t middle = low + (high - low) / 2;
            if (accounts_[middle].viewer < viewer) {
                low = middle + 1;
            } else {
                high = middle;
            }
        }
        if (low < accounts_.size() && !(viewer < accounts_[low].viewer)) {
            return &accounts_[low];
        }
        return nullptr;
    }

    Account* find(const Viewer& viewer) {
        return const_cast<Account*>(std::as_const(*this).find(viewer));
    }

    std::size_t limit_;
    std::vector<Account> accounts_;  // sorted by viewer
    Timeline timeline_;              // newest first
};
