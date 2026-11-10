#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

inline constexpr std::uint64_t kHashPrime = (std::uint64_t{1} << 61) - 1;

class DotProductHash {
public:
    DotProductHash(std::uint64_t slots, std::mt19937_64& rng) : slots_(slots) {
        if (!is_prime(slots) || slots >= (std::uint64_t{1} << 31)) {
            throw std::invalid_argument("DotProductHash needs a prime below 2^31");
        }
        unsigned __int128 covered = 1;
        while (covered >> 64 == 0) {
            covered *= slots;
            weights_.push_back(std::uniform_int_distribution<std::uint64_t>(0, slots - 1)(rng));
        }
    }

    std::uint64_t slots() const { return slots_; }

    std::uint64_t operator()(std::uint64_t key) const {
        std::uint64_t sum = 0;
        for (std::uint64_t weight : weights_) {
            sum = (sum + weight * (key % slots_)) % slots_;
            key /= slots_;
        }
        return sum;
    }

private:
    static bool is_prime(std::uint64_t n) {
        if (n < 2) {
            return false;
        }
        for (std::uint64_t d = 2; d * d <= n; ++d) {
            if (n % d == 0) {
                return false;
            }
        }
        return true;
    }

    std::uint64_t slots_;
    std::vector<std::uint64_t> weights_;
};

class LinearHash {
public:
    LinearHash(std::uint64_t slots, std::mt19937_64& rng)
        : slots_(slots),
          a_(std::uniform_int_distribution<std::uint64_t>(1, kHashPrime - 1)(rng)),
          b_(std::uniform_int_distribution<std::uint64_t>(0, kHashPrime - 1)(rng)) {
        if (slots == 0) {
            throw std::invalid_argument("LinearHash needs at least one slot");
        }
    }

    std::uint64_t slots() const { return slots_; }

    std::uint64_t operator()(std::uint64_t key) const {
        const unsigned __int128 product = static_cast<unsigned __int128>(a_) * key + b_;
        // A prime of the form 2^61 - 1 reduces with a shift instead of a division.
        std::uint64_t reduced = (static_cast<std::uint64_t>(product) & kHashPrime) +
                                static_cast<std::uint64_t>(product >> 61);
        if (reduced >= kHashPrime) {
            reduced -= kHashPrime;
        }
        return reduced % slots_;
    }

private:
    std::uint64_t slots_;
    std::uint64_t a_;
    std::uint64_t b_;
};

template <typename V>
class UniversalHashMap {
public:
    explicit UniversalHashMap(std::uint64_t seed = 6046)
        : rng_(seed), hash_(kInitialBuckets, rng_), buckets_(kInitialBuckets) {}

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    std::size_t bucket_count() const { return buckets_.size(); }

    std::size_t longest_chain() const {
        std::size_t longest = 0;
        for (const Bucket& bucket : buckets_) {
            longest = std::max(longest, bucket.size());
        }
        return longest;
    }

    bool insert(std::uint64_t key, V value) {
        check_key(key);
        if (V* existing = locate(key)) {
            *existing = std::move(value);
            return false;
        }
        if (size_ == buckets_.size()) {
            rehash(buckets_.size() * 2);
        }
        buckets_[hash_(key)].push_back({key, std::move(value)});
        ++size_;
        return true;
    }

    V* find(std::uint64_t key) {
        check_key(key);
        return locate(key);
    }

    const V* find(std::uint64_t key) const {
        check_key(key);
        return const_cast<UniversalHashMap*>(this)->locate(key);
    }

    bool erase(std::uint64_t key) {
        check_key(key);
        Bucket& bucket = buckets_[hash_(key)];
        const auto entry = std::ranges::find(bucket, key, &Entry::key);
        if (entry == bucket.end()) {
            return false;
        }
        bucket.erase(entry);
        --size_;
        return true;
    }

private:
    struct Entry {
        std::uint64_t key;
        V value;
    };
    using Bucket = std::vector<Entry>;

    static constexpr std::size_t kInitialBuckets = 8;

    static void check_key(std::uint64_t key) {
        if (key >= kHashPrime) {
            throw std::invalid_argument("key must be below 2^61 - 1");
        }
    }

    V* locate(std::uint64_t key) {
        Bucket& bucket = buckets_[hash_(key)];
        const auto entry = std::ranges::find(bucket, key, &Entry::key);
        return entry == bucket.end() ? nullptr : &entry->value;
    }

    // A fresh function on every rebuild, so an adversary can never learn a stable one.
    void rehash(std::size_t bucket_count) {
        LinearHash next(bucket_count, rng_);
        std::vector<Bucket> grown(bucket_count);
        for (Bucket& bucket : buckets_) {
            for (Entry& entry : bucket) {
                grown[next(entry.key)].push_back(std::move(entry));
            }
        }
        buckets_ = std::move(grown);
        hash_ = next;
    }

    std::mt19937_64 rng_;
    LinearHash hash_;
    std::vector<Bucket> buckets_;
    std::size_t size_ = 0;
};

class RateLimiter {
public:
    RateLimiter(std::uint64_t limit, std::int64_t window, std::uint64_t seed = 6046)
        : limit_(limit), window_(window), clients_(seed) {
        if (limit == 0 || window <= 0) {
            throw std::invalid_argument("limit and window must be positive");
        }
    }

    bool allow(std::uint64_t client, std::int64_t now) {
        const std::int64_t bucket = now >= 0 ? now / window_ : (now - window_ + 1) / window_;
        if (Slot* slot = clients_.find(client)) {
            if (slot->bucket != bucket) {
                *slot = Slot{bucket, 0};
            }
            if (slot->count >= limit_) {
                return false;
            }
            ++slot->count;
            return true;
        }
        clients_.insert(client, Slot{bucket, 1});
        return true;
    }

    std::size_t client_count() const { return clients_.size(); }
    std::size_t longest_chain() const { return clients_.longest_chain(); }

private:
    struct Slot {
        std::int64_t bucket;
        std::uint64_t count;
    };

    std::uint64_t limit_;
    std::int64_t window_;
    UniversalHashMap<Slot> clients_;
};
