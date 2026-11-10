#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename K, typename Hash = std::hash<K>>
class OpenAddressingSet {
public:
    explicit OpenAddressingSet(Hash hash = Hash{}) : hash_(std::move(hash)), slots_(kMinCapacity) {}

    bool insert(K key) {
        // Tombstones count against the load: every probe has to walk over them too.
        if ((size_ + tombstones_ + 1) * 4 > slots_.size() * 3) {
            rehash();
        }
        std::size_t index = home_of(key);
        std::size_t first_tombstone = npos;
        while (slots_[index].key.has_value() || slots_[index].tombstone) {
            if (slots_[index].key.has_value()) {
                if (*slots_[index].key == key) {
                    return false;  // Also catches a duplicate sitting past a tombstone.
                }
            } else if (first_tombstone == npos) {
                first_tombstone = index;
            }
            index = (index + 1) & mask();
        }
        if (first_tombstone != npos) {
            index = first_tombstone;
            slots_[index].tombstone = false;
            --tombstones_;
        }
        slots_[index].key.emplace(std::move(key));
        ++size_;
        return true;
    }

    bool contains(const K& key) const { return locate(key) != npos; }

    bool erase(const K& key) {
        const std::size_t index = locate(key);
        if (index == npos) {
            return false;
        }
        slots_[index].key.reset();
        slots_[index].tombstone = true;
        ++tombstones_;
        --size_;
        if (slots_.size() > kMinCapacity && size_ * 8 < slots_.size()) {
            rehash();
        }
        return true;
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return slots_.size(); }

private:
    struct Slot {
        std::optional<K> key;
        bool tombstone = false;
    };

    static constexpr std::size_t kMinCapacity = 16;
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    static std::uint64_t mix(std::uint64_t value) {
        value ^= value >> 33;
        value *= 0xFF51AFD7ED558CCDULL;
        value ^= value >> 33;
        value *= 0xC4CEB9FE1A85EC53ULL;
        value ^= value >> 33;
        return value;
    }

    std::size_t mask() const { return slots_.size() - 1; }

    std::size_t home_of(const K& key) const {
        return static_cast<std::size_t>(mix(static_cast<std::uint64_t>(hash_(key)))) & mask();
    }

    std::size_t locate(const K& key) const {
        std::size_t index = home_of(key);
        while (slots_[index].key.has_value() || slots_[index].tombstone) {
            if (slots_[index].key.has_value() && *slots_[index].key == key) {
                return index;
            }
            index = (index + 1) & mask();
        }
        return npos;
    }

    // Rebuilds at twice the live size, which grows, shrinks and drops tombstones all at once.
    void rehash() {
        const std::size_t target = std::max<std::size_t>(kMinCapacity, std::bit_ceil((size_ + 1) * 2));
        std::vector<Slot> old(target);
        old.swap(slots_);
        tombstones_ = 0;
        for (Slot& slot : old) {
            if (slot.key.has_value()) {
                std::size_t index = home_of(*slot.key);
                while (slots_[index].key.has_value()) {
                    index = (index + 1) & mask();
                }
                slots_[index].key = std::move(slot.key);
            }
        }
    }

    Hash hash_;
    std::vector<Slot> slots_;
    std::size_t size_ = 0;
    std::size_t tombstones_ = 0;
};

// Remembers keys for `ttl` ticks after their first accepted sighting.
template <typename K, typename Hash = std::hash<K>>
class DedupWindow {
public:
    explicit DedupWindow(std::int64_t ttl, Hash hash = Hash{}) : ttl_(ttl), seen_(std::move(hash)) {
        if (ttl <= 0) {
            throw std::invalid_argument("ttl must be positive");
        }
    }

    // True when the event is new and accepted, false when it is a duplicate inside the window.
    bool offer(const K& key, std::int64_t now) {
        while (!expiry_.empty() && expiry_.front().first <= now) {
            seen_.erase(expiry_.front().second);
            expiry_.pop_front();
        }
        if (seen_.contains(key)) {
            return false;
        }
        seen_.insert(key);
        expiry_.emplace_back(now + ttl_, key);
        return true;
    }

    std::size_t size() const { return seen_.size(); }

private:
    std::int64_t ttl_;
    OpenAddressingSet<K, Hash> seen_;
    std::deque<std::pair<std::int64_t, K>> expiry_;
};
