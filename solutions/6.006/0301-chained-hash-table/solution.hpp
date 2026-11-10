#pragma once

#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// One member of the universal family h_ab(k) = ((a*k + b) mod p) mod m with p = 2^61 - 1.
class UniversalHash {
public:
    static constexpr std::uint64_t kPrime = (std::uint64_t{1} << 61) - 1;

    UniversalHash(std::uint64_t a, std::uint64_t b) : a_(a), b_(b) {
        if (a == 0 || a >= kPrime || b >= kPrime) {
            throw std::invalid_argument("UniversalHash needs a in [1, p) and b in [0, p)");
        }
    }

    std::size_t operator()(std::uint64_t key, std::size_t buckets) const {
        const unsigned __int128 value = static_cast<unsigned __int128>(a_) * key + b_;
        return static_cast<std::size_t>(reduce(value) % buckets);
    }

private:
    // value mod (2^61 - 1), using 2^61 == 1 (mod p). value < 2^125 here.
    static std::uint64_t reduce(unsigned __int128 value) {
        value = (value & kPrime) + (value >> 61);
        value = (value & kPrime) + (value >> 61);
        return value >= kPrime ? static_cast<std::uint64_t>(value - kPrime)
                               : static_cast<std::uint64_t>(value);
    }

    std::uint64_t a_;
    std::uint64_t b_;
};

template <typename K, typename V, typename Prehash = std::hash<K>>
class ChainedHashMap {
public:
    explicit ChainedHashMap(std::uint64_t seed = 0x9E3779B97F4A7C15ULL, Prehash prehash = Prehash{})
        : hash_(derive(seed)), prehash_(std::move(prehash)), buckets_(kMinBuckets) {}

    ChainedHashMap(const ChainedHashMap&) = delete;
    ChainedHashMap& operator=(const ChainedHashMap&) = delete;

    ChainedHashMap(ChainedHashMap&& other) noexcept
        : hash_(other.hash_),
          prehash_(std::move(other.prehash_)),
          buckets_(std::move(other.buckets_)),
          size_(std::exchange(other.size_, 0)) {
        other.buckets_.clear();
    }

    ChainedHashMap& operator=(ChainedHashMap&& other) noexcept {
        if (this != &other) {
            hash_ = other.hash_;
            prehash_ = std::move(other.prehash_);
            buckets_ = std::move(other.buckets_);
            size_ = std::exchange(other.size_, 0);
            other.buckets_.clear();
        }
        return *this;
    }

    // Inserts or overwrites; true when the key was not present before.
    bool insert(K key, V value) {
        if (buckets_.empty()) {
            buckets_.resize(kMinBuckets);
        }
        auto& chain = buckets_[bucket_of(key)];
        for (Entry& entry : chain) {
            if (entry.key == key) {
                entry.value = std::move(value);
                return false;
            }
        }
        chain.push_front(Entry{std::move(key), std::move(value)});
        ++size_;
        if (size_ > buckets_.size()) {
            rehash(buckets_.size() * 2);
        }
        return true;
    }

    V* find(const K& key) { return locate(key); }
    const V* find(const K& key) const { return locate(key); }

    std::optional<V> erase(const K& key) {
        if (buckets_.empty()) {
            return std::nullopt;
        }
        auto& chain = buckets_[bucket_of(key)];
        for (auto before = chain.before_begin(), it = chain.begin(); it != chain.end();
             before = it, ++it) {
            if (it->key == key) {
                std::optional<V> value(std::move(it->value));
                chain.erase_after(before);
                --size_;
                if (buckets_.size() > kMinBuckets && size_ < buckets_.size() / 4) {
                    rehash(buckets_.size() / 2);
                }
                return value;
            }
        }
        return std::nullopt;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    std::size_t bucket_count() const { return buckets_.size(); }
    double load_factor() const {
        return buckets_.empty() ? 0.0
                                : static_cast<double>(size_) / static_cast<double>(buckets_.size());
    }

    std::size_t bucket_of(const K& key) const {
        return hash_(static_cast<std::uint64_t>(prehash_(key)), buckets_.size());
    }

    std::size_t max_chain_length() const {
        std::size_t longest = 0;
        for (const auto& chain : buckets_) {
            std::size_t length = 0;
            for (auto it = chain.begin(); it != chain.end(); ++it) {
                ++length;
            }
            longest = std::max(longest, length);
        }
        return longest;
    }

private:
    struct Entry {
        K key;
        V value;
    };

    static constexpr std::size_t kMinBuckets = 8;

    static UniversalHash derive(std::uint64_t seed) {
        const std::uint64_t mixed = splitmix(seed);
        const std::uint64_t a = mixed % (UniversalHash::kPrime - 1) + 1;
        const std::uint64_t b = splitmix(seed + 1) % UniversalHash::kPrime;
        return UniversalHash(a, b);
    }

    static std::uint64_t splitmix(std::uint64_t value) {
        value += 0x9E3779B97F4A7C15ULL;
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    }

    V* locate(const K& key) const {
        if (buckets_.empty()) {
            return nullptr;
        }
        auto& chain = const_cast<std::forward_list<Entry>&>(buckets_[bucket_of(key)]);
        for (Entry& entry : chain) {
            if (entry.key == key) {
                return &entry.value;
            }
        }
        return nullptr;
    }

    void rehash(std::size_t new_count) {
        std::vector<std::forward_list<Entry>> fresh(new_count);
        for (auto& chain : buckets_) {
            while (!chain.empty()) {
                const std::size_t target = hash_(static_cast<std::uint64_t>(prehash_(chain.front().key)), new_count);
                fresh[target].splice_after(fresh[target].before_begin(), chain, chain.before_begin());
            }
        }
        buckets_ = std::move(fresh);
    }

    UniversalHash hash_;
    Prehash prehash_;
    std::vector<std::forward_list<Entry>> buckets_;
    std::size_t size_ = 0;
};
