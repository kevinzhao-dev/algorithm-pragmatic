#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename Key>
class IndexedSkipList {
public:
    class Node {
    public:
        const Key& key() const { return *key_; }

    private:
        friend class IndexedSkipList;

        struct Link {
            Node* next = nullptr;
            Node* prev = nullptr;
            std::size_t span = 0;  // level-0 steps from this node to next
        };

        explicit Node(std::size_t height) : links_(height) {}
        Node(Key key, std::size_t height) : key_(std::move(key)), links_(height) {}

        std::size_t height() const { return links_.size(); }

        std::optional<Key> key_;
        std::vector<Link> links_;
    };

    explicit IndexedSkipList(std::uint64_t seed = 6046)
        : rng_(seed), head_(new Node(kMaxHeight)), tail_(new Node(kMaxHeight)) {
        for (std::size_t level = 0; level < kMaxHeight; ++level) {
            head_->links_[level].next = tail_;
            head_->links_[level].span = 1;
            tail_->links_[level].prev = head_;
        }
    }

    ~IndexedSkipList() {
        Node* node = head_;
        while (node != nullptr) {
            Node* following = node->links_[0].next;
            delete node;
            node = following;
        }
    }

    IndexedSkipList(const IndexedSkipList&) = delete;
    IndexedSkipList& operator=(const IndexedSkipList&) = delete;

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    Node* first() const { return size_ == 0 ? nullptr : head_->links_[0].next; }

    Node* next(const Node* node) const {
        Node* following = node->links_[0].next;
        return following == tail_ ? nullptr : following;
    }

    Node* prev(const Node* node) const {
        Node* preceding = node->links_[0].prev;
        return preceding == head_ ? nullptr : preceding;
    }

    Node* find(const Key& key) const {
        Node* z = predecessor_of(key);
        Node* candidate = z->links_[0].next;
        return candidate != tail_ && !(key < candidate->key()) ? candidate : nullptr;
    }

    Node* insert(Key key) {
        std::vector<Node*> update(kMaxHeight);
        std::vector<std::size_t> position(kMaxHeight);
        locate(key, update, position);
        Node* successor = update[0]->links_[0].next;
        if (successor != tail_ && !(key < successor->key())) {
            return nullptr;
        }

        const std::size_t height = random_height();
        for (std::size_t level = active_; level < height; ++level) {
            update[level] = head_;
            position[level] = 0;
            head_->links_[level].span = size_ + 1;
        }
        active_ = std::max(active_, height);

        Node* node = new Node(std::move(key), height);
        const std::size_t new_position = position[0] + 1;
        for (std::size_t level = 0; level < height; ++level) {
            Node* before = update[level];
            typename Node::Link& link = before->links_[level];
            Node* after = link.next;
            const std::size_t old_span = link.span;

            node->links_[level].next = after;
            node->links_[level].prev = before;
            after->links_[level].prev = node;
            link.next = node;
            link.span = new_position - position[level];
            node->links_[level].span = old_span + 1 - link.span;
        }
        for (std::size_t level = height; level < active_; ++level) {
            ++update[level]->links_[level].span;
        }
        ++size_;
        return node;
    }

    bool erase(const Key& key) {
        std::vector<Node*> update(kMaxHeight);
        std::vector<std::size_t> position(kMaxHeight);
        locate(key, update, position);
        Node* node = update[0]->links_[0].next;
        if (node == tail_ || key < node->key()) {
            return false;
        }
        for (std::size_t level = 0; level < active_; ++level) {
            typename Node::Link& link = update[level]->links_[level];
            if (link.next == node) {
                link.span += node->links_[level].span - 1;
                link.next = node->links_[level].next;
                link.next->links_[level].prev = update[level];
            } else {
                --link.span;
            }
        }
        delete node;
        --size_;
        while (active_ > 1 && head_->links_[active_ - 1].next == tail_) {
            --active_;
        }
        return true;
    }

    std::size_t rank(const Node* node) const {
        std::size_t steps_to_head = 0;
        const Node* z = node;
        std::size_t level = 0;
        while (z != head_) {
            ++steps_;
            if (level + 1 < z->height()) {
                ++level;
                continue;
            }
            const Node* before = z->links_[level].prev;
            steps_to_head += before->links_[level].span;
            z = before;
        }
        return steps_to_head - 1;
    }

    Node* at(std::size_t index) const {
        if (index >= size_) {
            return nullptr;
        }
        std::size_t remaining = index + 1;
        Node* z = head_;
        for (std::size_t level = active_; level-- > 0;) {
            while (z->links_[level].span <= remaining) {
                ++steps_;
                remaining -= z->links_[level].span;
                z = z->links_[level].next;
            }
            ++steps_;
            if (remaining == 0) {
                break;
            }
        }
        return z;
    }

    Node* finger_search(Node* from, const Key& key) const {
        if (key < from->key()) {
            return finger_left(from, key);
        }
        if (from->key() < key) {
            return finger_right(from, key);
        }
        return from;
    }

    Node* rank_search(Node* from, std::ptrdiff_t delta) const {
        if (delta > 0) {
            return rank_right(from, static_cast<std::size_t>(delta));
        }
        if (delta < 0) {
            return rank_left(from, static_cast<std::size_t>(-delta));
        }
        return from;
    }

    std::uint64_t steps() const { return steps_; }

private:
    static constexpr std::size_t kMaxHeight = 40;

    std::size_t random_height() {
        std::size_t height = 1;
        while (height < kMaxHeight && (rng_() & 1) != 0) {
            ++height;
        }
        return height;
    }

    Node* predecessor_of(const Key& key) const {
        Node* z = head_;
        for (std::size_t level = active_; level-- > 0;) {
            while (z->links_[level].next != tail_ && z->links_[level].next->key() < key) {
                z = z->links_[level].next;
            }
        }
        return z;
    }

    void locate(const Key& key, std::vector<Node*>& update, std::vector<std::size_t>& position) const {
        Node* z = head_;
        std::size_t place = 0;
        for (std::size_t level = active_; level-- > 0;) {
            while (z->links_[level].next != tail_ && z->links_[level].next->key() < key) {
                place += z->links_[level].span;
                z = z->links_[level].next;
            }
            update[level] = z;
            position[level] = place;
        }
    }

    // Climbs while it can still see at or before the target, then walks right and descends.
    Node* finger_right(Node* from, const Key& key) const {
        Node* z = from;
        std::size_t level = 0;
        for (;;) {
            ++steps_;
            if (level + 1 < z->height()) {
                Node* above = z->links_[level + 1].next;
                if (above != tail_ && !(key < above->key())) {
                    ++level;
                    continue;
                }
            }
            Node* forward = z->links_[level].next;
            if (forward == tail_) {
                break;
            }
            Node* beyond = forward->links_[level].next;
            if (beyond != tail_ && !(key < beyond->key())) {
                z = forward;
                continue;
            }
            break;
        }
        for (;;) {
            ++steps_;
            Node* forward = z->links_[level].next;
            if (forward != tail_ && !(key < forward->key())) {
                z = forward;
            } else if (level > 0) {
                --level;
            } else {
                break;
            }
        }
        return z->key() < key ? nullptr : z;
    }

    Node* finger_left(Node* from, const Key& key) const {
        Node* z = from;
        std::size_t level = 0;
        for (;;) {
            ++steps_;
            if (level + 1 < z->height()) {
                Node* above = z->links_[level + 1].prev;
                if (above != head_ && !(above->key() < key)) {
                    ++level;
                    continue;
                }
            }
            Node* backward = z->links_[level].prev;
            if (backward == head_) {
                break;
            }
            Node* beyond = backward->links_[level].prev;
            if (beyond != head_ && !(beyond->key() < key)) {
                z = backward;
                continue;
            }
            break;
        }
        for (;;) {
            ++steps_;
            Node* backward = z->links_[level].prev;
            if (backward != head_ && !(backward->key() < key)) {
                z = backward;
            } else if (level > 0) {
                --level;
            } else {
                break;
            }
        }
        return key < z->key() ? nullptr : z;
    }

    Node* rank_right(Node* from, std::size_t remaining) const {
        Node* z = from;
        std::size_t level = 0;
        for (;;) {
            ++steps_;
            if (level + 1 < z->height() && z->links_[level + 1].next != tail_ &&
                z->links_[level + 1].span <= remaining) {
                ++level;
                continue;
            }
            Node* forward = z->links_[level].next;
            if (forward != tail_ && forward->links_[level].next != tail_ &&
                z->links_[level].span + forward->links_[level].span <= remaining) {
                remaining -= z->links_[level].span;
                z = forward;
                continue;
            }
            break;
        }
        for (;;) {
            ++steps_;
            Node* forward = z->links_[level].next;
            if (remaining == 0) {
                return z;
            }
            if (forward != tail_ && z->links_[level].span <= remaining) {
                remaining -= z->links_[level].span;
                z = forward;
            } else if (level > 0) {
                --level;
            } else {
                return nullptr;
            }
        }
    }

    Node* rank_left(Node* from, std::size_t remaining) const {
        Node* z = from;
        std::size_t level = 0;
        const auto span_back = [](const Node* node, std::size_t lvl) {
            return node->links_[lvl].prev->links_[lvl].span;
        };
        for (;;) {
            ++steps_;
            if (level + 1 < z->height() && z->links_[level + 1].prev != head_ &&
                span_back(z, level + 1) <= remaining) {
                ++level;
                continue;
            }
            Node* backward = z->links_[level].prev;
            if (backward != head_ && backward->links_[level].prev != head_ &&
                span_back(z, level) + span_back(backward, level) <= remaining) {
                remaining -= span_back(z, level);
                z = backward;
                continue;
            }
            break;
        }
        for (;;) {
            ++steps_;
            Node* backward = z->links_[level].prev;
            if (remaining == 0) {
                return z;
            }
            if (backward != head_ && span_back(z, level) <= remaining) {
                remaining -= span_back(z, level);
                z = backward;
            } else if (level > 0) {
                --level;
            } else {
                return nullptr;
            }
        }
    }

    std::mt19937_64 rng_;
    Node* head_;
    Node* tail_;
    std::size_t size_ = 0;
    std::size_t active_ = 1;
    mutable std::uint64_t steps_ = 0;
};

class SlidingMedian {
public:
    explicit SlidingMedian(std::uint64_t seed = 6046) : list_(seed) {}

    std::size_t size() const { return window_.size(); }
    std::uint64_t steps() const { return list_.steps(); }

    std::optional<std::int64_t> median() const {
        if (median_ == nullptr) {
            return std::nullopt;
        }
        return median_->key().first;
    }

    void push(std::int64_t value) {
        const Item item{value, next_seq_++};
        window_.push_back(item);
        const std::size_t before = window_.size() - 1;
        Node* node = list_.insert(item);
        if (before == 0) {
            median_ = node;
            return;
        }
        const bool went_left = item < median_->key();
        std::ptrdiff_t delta = 0;
        if (before % 2 == 0) {
            delta = went_left ? 0 : 1;
        } else {
            delta = went_left ? -1 : 0;
        }
        median_ = list_.rank_search(median_, delta);
    }

    void pop() {
        if (window_.empty()) {
            throw std::out_of_range("the window is empty");
        }
        const Item item = window_.front();
        window_.pop_front();
        const std::size_t before = window_.size() + 1;
        if (before == 1) {
            list_.erase(item);
            median_ = nullptr;
            return;
        }
        if (item == median_->key()) {
            Node* replacement = before % 2 == 0 ? list_.next(median_) : list_.prev(median_);
            list_.erase(item);
            median_ = replacement;
            return;
        }
        const bool went_left = item < median_->key();
        list_.erase(item);
        std::ptrdiff_t delta = 0;
        if (before % 2 == 0) {
            delta = went_left ? 1 : 0;
        } else {
            delta = went_left ? 0 : -1;
        }
        median_ = list_.rank_search(median_, delta);
    }

private:
    using Item = std::pair<std::int64_t, std::uint64_t>;
    using Node = IndexedSkipList<Item>::Node;

    IndexedSkipList<Item> list_;
    std::deque<Item> window_;
    Node* median_ = nullptr;
    std::uint64_t next_seq_ = 0;
};
