#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Key>
class SkipList {
public:
    struct Node {
        Node(Key k, std::size_t height) : key(std::move(k)), next(height, nullptr) {}

        Key key;
        std::vector<Node*> next;
    };

    explicit SkipList(std::uint64_t seed = 6046) : rng_(seed) {}

    ~SkipList() { release_all(); }

    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    SkipList(SkipList&& other) noexcept
        : head_(std::move(other.head_)), size_(std::exchange(other.size_, 0)), rng_(other.rng_) {
        other.head_.clear();
    }

    SkipList& operator=(SkipList&& other) noexcept {
        if (this != &other) {
            release_all();
            head_ = std::move(other.head_);
            other.head_.clear();
            size_ = std::exchange(other.size_, 0);
            rng_ = other.rng_;
        }
        return *this;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    int height() const { return static_cast<int>(head_.size()); }

    Node* first_at(int level) const {
        return static_cast<std::size_t>(level) < head_.size() ? head_[static_cast<std::size_t>(level)]
                                                              : nullptr;
    }
    Node* first() const { return first_at(0); }

    const Node* find(const Key& key) const {
        const Node* node = nullptr;  // nullptr stands for the head tower
        for (std::size_t level = head_.size(); level-- > 0;) {
            const Node* next = node == nullptr ? head_[level] : node->next[level];
            while (next != nullptr && next->key < key) {
                node = next;
                next = node->next[level];
            }
            if (next != nullptr && !(key < next->key)) {
                return next;
            }
        }
        return nullptr;
    }

    bool contains(const Key& key) const { return find(key) != nullptr; }

    bool insert(Key key) {
        const auto path = search_path(key);
        Node* successor = path.empty() ? nullptr : follow(path[0], 0);
        if (successor != nullptr && !(key < successor->key)) {
            return false;
        }

        std::size_t height = 1;
        while (height < kMaxHeight && (rng_() & 1) != 0) {
            ++height;
        }
        Node* node = new Node(std::move(key), height);
        if (height > head_.size()) {
            head_.resize(height, nullptr);
        }
        for (std::size_t level = 0; level < height; ++level) {
            Node* before = level < path.size() ? path[level] : nullptr;
            link_after(before, level, node);
        }
        ++size_;
        return true;
    }

    bool erase(const Key& key) {
        const auto path = search_path(key);
        Node* node = path.empty() ? nullptr : follow(path[0], 0);
        if (node == nullptr || key < node->key) {
            return false;
        }
        for (std::size_t level = 0; level < node->next.size(); ++level) {
            Node* before = path[level];
            (before == nullptr ? head_[level] : before->next[level]) = node->next[level];
        }
        delete node;
        --size_;
        while (!head_.empty() && head_.back() == nullptr) {
            head_.pop_back();
        }
        return true;
    }

private:
    static constexpr std::size_t kMaxHeight = 48;

    // path[level] is the last node with key < target at that level; nullptr means the head.
    std::vector<Node*> search_path(const Key& key) const {
        std::vector<Node*> path(head_.size(), nullptr);
        Node* node = nullptr;
        for (std::size_t level = head_.size(); level-- > 0;) {
            Node* next = follow(node, level);
            while (next != nullptr && next->key < key) {
                node = next;
                next = follow(node, level);
            }
            path[level] = node;
        }
        return path;
    }

    Node* follow(Node* node, std::size_t level) const {
        return node == nullptr ? head_[level] : node->next[level];
    }

    void link_after(Node* before, std::size_t level, Node* node) {
        Node*& slot = before == nullptr ? head_[level] : before->next[level];
        node->next[level] = slot;
        slot = node;
    }

    void release_all() {
        Node* node = head_.empty() ? nullptr : head_[0];
        while (node != nullptr) {
            Node* following = node->next[0];
            delete node;
            node = following;
        }
        head_.clear();
        size_ = 0;
    }

    std::vector<Node*> head_;
    std::size_t size_ = 0;
    std::mt19937_64 rng_;
};

class JobScheduler {
public:
    explicit JobScheduler(std::uint64_t seed = 6046) : queue_(seed) {}

    int schedule(std::int64_t run_at) {
        const int job = next_job_++;
        run_at_[job] = run_at;
        queue_.insert({run_at, job});
        return job;
    }

    bool cancel(int job) {
        const auto found = run_at_.find(job);
        if (found == run_at_.end()) {
            return false;
        }
        queue_.erase({found->second, job});
        run_at_.erase(found);
        return true;
    }

    std::vector<int> pop_due(std::int64_t now) {
        std::vector<int> due;
        while (const auto* node = queue_.first()) {
            if (node->key.first > now) {
                break;
            }
            const auto job = node->key;
            due.push_back(job.second);
            run_at_.erase(job.second);
            queue_.erase(job);
        }
        return due;
    }

    std::size_t pending() const { return run_at_.size(); }

private:
    SkipList<std::pair<std::int64_t, int>> queue_;
    std::unordered_map<int, std::int64_t> run_at_;
    int next_job_ = 0;
};
