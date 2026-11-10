#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

inline constexpr std::uint64_t kHashPrime = (std::uint64_t{1} << 61) - 1;

namespace perfect_detail {

class LinearHash {
public:
    LinearHash() = default;
    LinearHash(std::uint64_t slots, std::mt19937_64& rng)
        : slots_(slots),
          a_(std::uniform_int_distribution<std::uint64_t>(1, kHashPrime - 1)(rng)),
          b_(std::uniform_int_distribution<std::uint64_t>(0, kHashPrime - 1)(rng)) {}

    std::uint64_t operator()(std::uint64_t key) const {
        const unsigned __int128 product = static_cast<unsigned __int128>(a_) * key + b_;
        std::uint64_t reduced = (static_cast<std::uint64_t>(product) & kHashPrime) +
                                static_cast<std::uint64_t>(product >> 61);
        if (reduced >= kHashPrime) {
            reduced -= kHashPrime;
        }
        return reduced % slots_;
    }

private:
    std::uint64_t slots_ = 1;
    std::uint64_t a_ = 1;
    std::uint64_t b_ = 0;
};

}  // namespace perfect_detail

template <typename V>
class PerfectHashMap {
public:
    using Item = std::pair<std::uint64_t, V>;

    explicit PerfectHashMap(std::vector<Item> items, std::uint64_t seed = 6046) : size_(items.size()) {
        std::vector<std::uint64_t> keys;
        keys.reserve(items.size());
        for (const Item& item : items) {
            if (item.first >= kHashPrime) {
                throw std::invalid_argument("key must be below 2^61 - 1");
            }
            keys.push_back(item.first);
        }
        std::ranges::sort(keys);
        if (std::ranges::adjacent_find(keys) != keys.end()) {
            throw std::invalid_argument("keys must be distinct");
        }
        if (items.empty()) {
            return;
        }

        std::mt19937_64 rng(seed);
        // Level one: retry until the sum of squared bucket sizes stays linear.
        std::vector<std::size_t> sizes(size_);
        const auto n = static_cast<std::uint64_t>(size_);
        while (true) {
            top_ = perfect_detail::LinearHash(n, rng);
            std::ranges::fill(sizes, std::size_t{0});
            for (const Item& item : items) {
                ++sizes[top_(item.first)];
            }
            std::uint64_t squares = 0;
            for (const std::size_t count : sizes) {
                squares += count * count;
            }
            if (squares <= 4 * n) {
                total_slots_ = static_cast<std::size_t>(squares);
                break;
            }
        }

        std::vector<std::vector<Item>> grouped(size_);
        for (Item& item : items) {
            grouped[top_(item.first)].push_back(std::move(item));
        }

        buckets_.resize(size_);
        for (std::size_t j = 0; j < size_; ++j) {
            if (!grouped[j].empty()) {
                build_bucket(buckets_[j], grouped[j], rng);
            }
        }
    }

    std::size_t size() const { return size_; }
    std::size_t total_slots() const { return total_slots_; }
    std::uint64_t probes() const { return probes_; }

    const V* find(std::uint64_t key) const {
        if (key >= kHashPrime || size_ == 0) {
            return nullptr;
        }
        const Bucket& bucket = buckets_[top_(key)];
        ++probes_;
        if (bucket.slots.empty()) {
            return nullptr;
        }
        const std::optional<Item>& slot = bucket.slots[bucket.hash(key)];
        ++probes_;
        return slot.has_value() && slot->first == key ? &slot->second : nullptr;
    }

    bool contains(std::uint64_t key) const { return find(key) != nullptr; }

private:
    struct Bucket {
        perfect_detail::LinearHash hash;
        std::vector<std::optional<Item>> slots;
    };

    static void build_bucket(Bucket& bucket, std::vector<Item>& members, std::mt19937_64& rng) {
        const std::uint64_t slot_count = static_cast<std::uint64_t>(members.size()) * members.size();
        std::vector<std::size_t> placed(slot_count, 0);
        std::size_t generation = 0;
        while (true) {
            ++generation;
            bucket.hash = perfect_detail::LinearHash(slot_count, rng);
            bool collision_free = true;
            for (const Item& item : members) {
                const std::uint64_t position = bucket.hash(item.first);
                if (placed[position] == generation) {
                    collision_free = false;
                    break;
                }
                placed[position] = generation;
            }
            if (collision_free) {
                break;
            }
        }
        bucket.slots.resize(slot_count);
        for (Item& item : members) {
            bucket.slots[bucket.hash(item.first)] = std::move(item);
        }
    }

    std::size_t size_;
    std::size_t total_slots_ = 0;
    perfect_detail::LinearHash top_;
    std::vector<Bucket> buckets_;
    mutable std::uint64_t probes_ = 0;
};

template <typename V>
class StaticStringMap {
public:
    using Item = std::pair<std::string, V>;

    explicit StaticStringMap(std::vector<Item> items, std::uint64_t seed = 6046) : rng_(seed) {
        std::vector<std::string_view> names;
        names.reserve(items.size());
        for (const Item& item : items) {
            names.push_back(item.first);
        }
        std::ranges::sort(names);
        if (std::ranges::adjacent_find(names) != names.end()) {
            throw std::invalid_argument("keys must be distinct");
        }

        // Two distinct strings can share a 61-bit fingerprint, so a collision forces a new base.
        std::vector<std::uint64_t> fingerprints(items.size());
        while (true) {
            base_ = std::uniform_int_distribution<std::uint64_t>(1'000'000, kHashPrime - 2)(rng_);
            for (std::size_t i = 0; i < items.size(); ++i) {
                fingerprints[i] = fingerprint(items[i].first);
            }
            auto sorted = fingerprints;
            std::ranges::sort(sorted);
            if (std::ranges::adjacent_find(sorted) == sorted.end()) {
                break;
            }
        }

        std::vector<std::pair<std::uint64_t, Entry>> keyed;
        keyed.reserve(items.size());
        for (std::size_t i = 0; i < items.size(); ++i) {
            keyed.emplace_back(fingerprints[i], Entry{std::move(items[i].first), std::move(items[i].second)});
        }
        table_.emplace(std::move(keyed), rng_());
    }

    std::size_t size() const { return table_->size(); }

    const V* find(std::string_view key) const {
        const Entry* entry = table_->find(fingerprint(key));
        return entry != nullptr && entry->name == key ? &entry->value : nullptr;
    }

private:
    struct Entry {
        std::string name;
        V value;
    };

    std::uint64_t fingerprint(std::string_view text) const {
        std::uint64_t hash = 1;
        for (const char c : text) {
            const unsigned __int128 product =
                static_cast<unsigned __int128>(hash) * base_ + static_cast<unsigned char>(c) + 1;
            hash = static_cast<std::uint64_t>(product % kHashPrime);
        }
        return hash;
    }

    std::mt19937_64 rng_;
    std::uint64_t base_ = 0;
    std::optional<PerfectHashMap<Entry>> table_;
};
